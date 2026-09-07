/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "gamepad.h"

#include "_keyboar.h"
#include "_map.h"
#include "dbgprint.h"
#include "globals.h"
#include "goptions.h"
#include "init.h"
#include "options.h"
#include "win.h"

#include <Xinput.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

typedef DWORD (WINAPI * XInputGetStateType)(DWORD index, XINPUT_STATE * state);

enum {
	STICK_DEADZONE = 16000,
	POINTER_DEADZONE = 7000,
	TRIGGER_THRESHOLD = 64,
	PAD_COUNT = 4,
};


// XInput is loaded at first use so a machine without it still runs the game.
static XInputGetStateType Get_State_Function(void)
{
	static XInputGetStateType _function = NULL;
	static bool _tried = false;

	if (!_tried) {
		_tried = true;
		static char const * const _names[] = {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"};
		for (char const * name : _names) {
			HMODULE module = LoadLibraryA(name);
			if (module != NULL) {
				_function = (XInputGetStateType)GetProcAddress(module, "XInputGetState");
				if (_function != NULL) {
					break;
				}
				FreeLibrary(module);
			}
		}
	}
	return(_function);
}


enum { VENDOR_SONY = 0x054C, VENDOR_MICROSOFT = 0x045E };

// Under Proton the Linux kernel's own device list is at hand through drive Z, and it still
// names the real pad after Steam Input has hidden it from the Windows side. Returns false
// where there is no such list.
static bool Kind_From_Linux_Devices(GamepadKindType & kind, bool log)
{
	FILE * file = fopen("Z:\\proc\\bus\\input\\devices", "r");
	if (file == NULL) return(false);
	kind = GAMEPAD_KIND_UNKNOWN;
	char line[512];
	unsigned vendor = 0;
	unsigned product = 0;
	while (fgets(line, sizeof(line), file) != NULL) {
		if (line[0] == 'I') {
			vendor = product = 0;
			char const * v = strstr(line, "Vendor=");
			char const * p = strstr(line, "Product=");
			if (v != NULL) vendor = strtoul(v + 7, NULL, 16);
			if (p != NULL) product = strtoul(p + 8, NULL, 16);
		} else if (line[0] == 'N') {
			// Only game controllers count, not the pad's touchpad or motion sensors.
			bool pad = strstr(line, "Controller") != NULL || strstr(line, "pad") != NULL || strstr(line, "Gamepad") != NULL || strstr(line, "Joystick") != NULL;
			bool extra = strstr(line, "Touchpad") != NULL || strstr(line, "Motion") != NULL;
			if (!pad || extra) continue;
			if (log) DebugString("Game controller (host): vendor %04X product %04X %s", vendor, product, line + 3);
			if (vendor == VENDOR_SONY) {
				kind = GAMEPAD_KIND_PLAYSTATION;
			} else if (vendor == VENDOR_MICROSOFT && kind == GAMEPAD_KIND_UNKNOWN) {
				kind = GAMEPAD_KIND_XBOX;
			}
		}
	}
	fclose(file);
	return(true);
}


// A stick axis as -1 to 1 with the dead zone removed and the rest rescaled to fill the range.
static float Stick_Axis(SHORT raw)
{
	float value = raw < 0 ? -float(-int(raw)) : float(raw);
	float size = value < 0 ? -value : value;
	if (size <= POINTER_DEADZONE) return(0.0f);
	float scaled = (size - POINTER_DEADZONE) / (32767.0f - POINTER_DEADZONE);
	if (scaled > 1.0f) scaled = 1.0f;
	return(value < 0 ? -scaled : scaled);
}


GamepadKindType Gamepad_Kind(void)
{
	enum { RECHECK_MS = 3000, USAGE_PAGE_DESKTOP = 1, USAGE_JOYSTICK = 4, USAGE_GAMEPAD = 5 };
	static GamepadKindType _kind = GAMEPAD_KIND_UNKNOWN;
	static unsigned long _checked = 0;
	static bool _logged = false;

	unsigned long now = timeGetTime();
	if (_checked != 0 && now - _checked < RECHECK_MS) {
		return(_kind);
	}
	_checked = now;

	if (Kind_From_Linux_Devices(_kind, !_logged)) {
		_logged = true;
		return(_kind);
	}

	UINT count = 0;
	if (GetRawInputDeviceList(NULL, &count, sizeof(RAWINPUTDEVICELIST)) != 0 || count == 0) {
		return(_kind);
	}
	RAWINPUTDEVICELIST * list = new RAWINPUTDEVICELIST[count];
	count = GetRawInputDeviceList(list, &count, sizeof(RAWINPUTDEVICELIST));
	GamepadKindType found = GAMEPAD_KIND_UNKNOWN;
	for (UINT index = 0; index < count && count != (UINT)-1; index++) {
		if (list[index].dwType != RIM_TYPEHID) continue;
		RID_DEVICE_INFO info;
		info.cbSize = sizeof(info);
		UINT size = sizeof(info);
		if (GetRawInputDeviceInfoA(list[index].hDevice, RIDI_DEVICEINFO, &info, &size) == (UINT)-1) continue;
		bool pad = info.hid.usUsagePage == USAGE_PAGE_DESKTOP && (info.hid.usUsage == USAGE_JOYSTICK || info.hid.usUsage == USAGE_GAMEPAD);
		if (!pad) continue;
		if (!_logged) {
			DebugString("Game controller: vendor %04lX product %04lX usage %u\n", info.hid.dwVendorId, info.hid.dwProductId, info.hid.usUsage);
		}
		if (info.hid.dwVendorId == VENDOR_SONY) {
			found = GAMEPAD_KIND_PLAYSTATION;
		} else if (info.hid.dwVendorId == VENDOR_MICROSOFT && found == GAMEPAD_KIND_UNKNOWN) {
			found = GAMEPAD_KIND_XBOX;
		}
	}
	_logged = true;
	delete [] list;
	_kind = found;
	return(_kind);
}


GamepadStateType Gamepad_Read(void)
{
	GamepadStateType result = {};

	XInputGetStateType get_state = Get_State_Function();
	if (get_state == NULL) {
		return(result);
	}

	for (DWORD index = 0; index < PAD_COUNT; index++) {
		XINPUT_STATE state = {};
		if (get_state(index, &state) != ERROR_SUCCESS) {
			continue;
		}
		XINPUT_GAMEPAD const & pad = state.Gamepad;
		result.Connected = true;
		result.PadUp = (pad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
		result.PadDown = (pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
		result.PadLeft = (pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
		result.PadRight = (pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
		result.Up = result.PadUp || pad.sThumbLY > STICK_DEADZONE;
		result.Down = result.PadDown || pad.sThumbLY < -STICK_DEADZONE;
		result.Left = result.PadLeft || pad.sThumbLX < -STICK_DEADZONE;
		result.Right = result.PadRight || pad.sThumbLX > STICK_DEADZONE;
		result.Accept = (pad.wButtons & XINPUT_GAMEPAD_A) != 0;
		result.Back = (pad.wButtons & XINPUT_GAMEPAD_B) != 0;
		result.Third = (pad.wButtons & XINPUT_GAMEPAD_X) != 0;
		result.Fourth = (pad.wButtons & XINPUT_GAMEPAD_Y) != 0;
		result.LeftShoulder = (pad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
		result.RightShoulder = (pad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
		result.Fast = result.LeftShoulder || result.RightShoulder;
		result.LeftTrigger = pad.bLeftTrigger > TRIGGER_THRESHOLD;
		result.RightTrigger = pad.bRightTrigger > TRIGGER_THRESHOLD;
		result.LeftThumb = (pad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
		result.RightThumb = (pad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
		result.Menu = (pad.wButtons & XINPUT_GAMEPAD_START) != 0;
		result.View = (pad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
		result.StickX = Stick_Axis(pad.sThumbLX);
		result.StickY = Stick_Axis(pad.sThumbLY);
		result.RightStickX = Stick_Axis(pad.sThumbRX);
		result.RightStickY = Stick_Axis(pad.sThumbRY);
		break;
	}
	return(result);
}


// The menu button is Escape everywhere: it skips a movie, opens the in-game menu, and
// backs out of a screen. Polling is held to once per frame because asking XInput about
// a controller that is not there is slow.
static bool _KeyboardMouseSeen = false;
static bool _MenuStarts = false;
static bool _AutoSettled = false;
static int _SyntheticClicks = 0;

bool Gamepad_Claim_Synthetic_Click(void)
{
	if (_SyntheticClicks <= 0) return(false);
	_SyntheticClicks--;
	return(true);
}


// The pad in play: the left stick and the d-pad move the pointer, a shoulder button speeds
// it, and cross and circle are the mouse buttons, posted as the messages a mouse would send
// so every tactical behaviour follows.
static void Play_Input(GamepadStateType const & pad, GamepadStateType const & previous, unsigned long now)
{
	enum { STEP_CAP_MS = 50 };
	const float POINTER_RATE = 0.9f;		// Screen heights per second at full stick.
	const float PAD_RATE = 0.6f;			// The d-pad's steady rate.
	const float FAST_FACTOR = 2.2f;
	static unsigned long _last = 0;
	static float _carry_x = 0.0f;
	static float _carry_y = 0.0f;

	float dt = (_last == 0 ? 16 : std::min<unsigned long>(now - _last, STEP_CAP_MS)) / 1000.0f;
	_last = now;

	// The stick's response is squared for fine control near the centre.
	float vx = pad.StickX * (pad.StickX < 0 ? -pad.StickX : pad.StickX) * POINTER_RATE;
	float vy = -pad.StickY * (pad.StickY < 0 ? -pad.StickY : pad.StickY) * POINTER_RATE;
	if (pad.PadLeft) vx -= PAD_RATE;
	if (pad.PadRight) vx += PAD_RATE;
	if (pad.PadUp) vy -= PAD_RATE;
	if (pad.PadDown) vy += PAD_RATE;
	if (pad.Fast) {
		vx *= FAST_FACTOR;
		vy *= FAST_FACTOR;
	}

	RECT client;
	GetClientRect(MainWindow, &client);
	POINT origin = {client.left, client.top};
	POINT corner = {client.right, client.bottom};
	ClientToScreen(MainWindow, &origin);
	ClientToScreen(MainWindow, &corner);
	float height = float(corner.y - origin.y);

	_carry_x += vx * dt * height;
	_carry_y += vy * dt * height;
	int dx = int(_carry_x);
	int dy = int(_carry_y);
	_carry_x -= dx;
	_carry_y -= dy;
	if (dx != 0 || dy != 0) {
		POINT at;
		GetCursorPos(&at);
		at.x = std::clamp<long>(at.x + dx, origin.x, corner.x - 1);
		at.y = std::clamp<long>(at.y + dy, origin.y, corner.y - 1);
		SetCursorPos(at.x, at.y);
	}

	auto post = [&](UINT message, WPARAM flags, bool down) {
		POINT at;
		GetCursorPos(&at);
		ScreenToClient(MainWindow, &at);
		if (down) _SyntheticClicks++;
		PostMessage(MainWindow, message, flags, MAKELPARAM(at.x, at.y));
	};
	auto pressed = [&](bool now_down, bool was_down) { return(now_down && !was_down); };
	static bool _right_posted = false;

	// Cross is the left button. Circle is the right button, unless R1 holds it for the
	// rebuild command.
	if (pressed(pad.Accept, previous.Accept)) post(WM_LBUTTONDOWN, MK_LBUTTON, true);
	if (!pad.Accept && previous.Accept) post(WM_LBUTTONUP, 0, false);
	if (pressed(pad.Back, previous.Back)) {
		if (pad.RightShoulder) {
			Execute_Command("RepeatLastBuilding");
		} else {
			post(WM_RBUTTONDOWN, MK_RBUTTON, true);
			_right_posted = true;
		}
	}
	if (!pad.Back && previous.Back && _right_posted) {
		post(WM_RBUTTONUP, 0, false);
		_right_posted = false;
	}

	// The shapes with L2 make teams and with L1 select and centre on them; square alone
	// cycles the sidebar modes.
	static char const * const _make[3] = {"TeamCreate_1", "TeamCreate_2", "TeamCreate_3"};
	static char const * const _pick[3] = {"TeamCenter_1", "TeamCenter_2", "TeamCenter_3"};
	bool shapes[3] = {pressed(pad.Third, previous.Third), pressed(pad.Fourth, previous.Fourth), pressed(pad.Back, previous.Back)};
	for (int index = 0; index < 3; index++) {
		if (!shapes[index]) continue;
		if (pad.LeftTrigger) {
			Execute_Command(_make[index]);
		} else if (pad.LeftShoulder) {
			Execute_Command(_pick[index]);
		}
	}
	if (pressed(pad.Third, previous.Third) && !pad.LeftTrigger && !pad.LeftShoulder) {
		if (Map.IsRepairMode) {
			Map.Repair_Mode_Control(0);
			Map.Sell_Mode_Control(1);
		} else if (Map.IsSellMode) {
			Map.Sell_Mode_Control(0);
			Map.Power_Mode_Control(1);
		} else if (Map.IsPowerMode) {
			Map.Power_Mode_Control(0);
			Map.Waypoint_Mode_Control(1);
		} else if (Map.IsWaypointMode) {
			Map.Waypoint_Mode_Control(0);
		} else {
			Map.Repair_Mode_Control(1);
		}
	}

	// R1 with L1 holds the force fire key and R1 with L2 the force move key, so the next
	// cross press orders as a Ctrl or Alt click would.
	static bool _force_fire = false;
	static bool _force_move = false;
	auto hold_key = [&](bool & held, bool want, WORD vk) {
		if (want == held) return;
		held = want;
		if (want) _SyntheticClicks++;
		PostMessage(MainWindow, want ? WM_KEYDOWN : WM_KEYUP, vk, want ? 0 : 0xC0000000);
	};
	hold_key(_force_fire, pad.RightShoulder && pad.LeftShoulder, VK_CONTROL);
	hold_key(_force_move, pad.RightShoulder && pad.LeftTrigger, VK_MENU);

	if (pressed(pad.LeftThumb, previous.LeftThumb)) Execute_Command("DeployObject");
	if (pressed(pad.RightThumb, previous.RightThumb)) Execute_Command("CenterBase");
	if (pressed(pad.RightTrigger, previous.RightTrigger)) Execute_Command(pad.RightShoulder ? "GuardObject" : "ScatterObject");
	if (pressed(pad.View, previous.View)) Execute_Command("ToggleAlliance");
}

void Gamepad_Settle_Auto_Scheme(unsigned wait_ms)
{
	if (_AutoSettled) return;
	if (Options.ControlSchemeAuto && Options.ControlScheme != CONTROL_CONTROLLER) {
		unsigned long until = timeGetTime() + wait_ms;
		while (timeGetTime() < until) {
			if (Gamepad_Read().Connected) {
				DebugString("ControlScheme is Controller (auto, pad found before the shell)\n");
				Options.ControlScheme = CONTROL_CONTROLLER;
				break;
			}
			Sleep(50);
		}
	}
	_AutoSettled = true;
}

void Gamepad_Menu_Starts(bool on)
{
	_MenuStarts = on;
}


bool Keyboard_Mouse_Seen(void)
{
	return(_KeyboardMouseSeen);
}


void Note_Keyboard_Mouse_Use(void)
{
	_KeyboardMouseSeen = true;
}


void Note_Keyboard_Mouse_Reset(void)
{
	_KeyboardMouseSeen = false;
}


void Gamepad_Pump(void * dialog)
{
	enum {
		POLL_MS = 16,
		SLOW_POLL_MS = 250,
		CHORD_MS = 1000,
		REPEAT_FIRST_MS = 350,
		REPEAT_NEXT_MS = 90,
	};
	static GamepadStateType _previous = {};
	static unsigned long _next_poll = 0;
	static unsigned long _repeat_at = 0;

	static unsigned long _chord_since = 0;

	if (Keyboard == NULL) {
		return;
	}
	unsigned long now = timeGetTime();
	if (now < _next_poll) {
		return;
	}
	bool controller = Options.ControlScheme == CONTROL_CONTROLLER;
	// Under the keyboard scheme the pad is only watched for the way back, and slowly,
	// since asking XInput about a controller that is not there is slow.
	_next_poll = now + (controller ? POLL_MS : SLOW_POLL_MS);

	GamepadStateType pad = Gamepad_Read();
	// Menu and back held together for a second switch to the controller scheme from
	// anywhere, so a pad-only player who chose the keyboard scheme is never locked out.
	if (pad.Menu && pad.Back) {
		if (_chord_since == 0) {
			_chord_since = now;
		} else if (now - _chord_since >= CHORD_MS && !controller) {
			Options.ControlScheme = CONTROL_CONTROLLER;
			Options.ControlSchemeAuto = false;
			Options.Save_Settings();
			if (dialog != NULL) {
				PostMessage((HWND)dialog, WM_KEYDOWN, VK_ESCAPE, 0);
				PostMessage((HWND)dialog, WM_KEYUP, VK_ESCAPE, 0xC0000000);
			}
			Keyboard->Put(KN_ESC);
			Keyboard->Put(KN_ESC | WWKEY_RLS_BIT);
			_chord_since = now;
		}
	} else {
		_chord_since = 0;
	}
	// Steam hands a game its virtual pad a moment after the window exists, so an Auto scheme
	// that found none at launch keeps looking through the startup movies until it is settled.
	if (!controller && !_AutoSettled && Options.ControlSchemeAuto && pad.Connected) {
		DebugString("ControlScheme is Controller (auto, pad appeared after launch)\n");
		Options.ControlScheme = CONTROL_CONTROLLER;
		controller = true;
	}
	// Once settled, a pad button pressed on a keyboard-scheme shell page is proof a pad is in
	// the player's hands, so Auto switches to the controller scheme there and then.
	bool pressed = (pad.Accept && !_previous.Accept) || (pad.Back && !_previous.Back) || (pad.Menu && !_previous.Menu)
		|| (pad.Up && !_previous.Up) || (pad.Down && !_previous.Down) || (pad.Left && !_previous.Left) || (pad.Right && !_previous.Right);
	if (!controller && _AutoSettled && Options.ControlSchemeAuto && !ScenarioActive && pressed) {
		DebugString("ControlScheme is Controller (auto, pad pressed in the shell)\n");
		Options.ControlScheme = CONTROL_CONTROLLER;
		_previous = pad;
		return;
	}
	if (!controller) {
		_previous = pad;
		return;
	}
	if (pad.Menu && !_previous.Menu && !_MenuStarts) {
		Keyboard->Put(KN_ESC);
		Keyboard->Put(KN_ESC | WWKEY_RLS_BIT);
	}

	if (dialog == NULL && ScenarioActive && !IgnoreInput && GameInFocus) {
		Play_Input(pad, _previous, now);
		_previous = pad;
		return;
	}

	HWND window = (HWND)dialog;
	if (window != NULL) {
		auto press = [&](int vk) {
			HWND target = GetFocus();
			if (target == NULL || !IsChild(window, target)) {
				target = window;
			}
			PostMessage(target, WM_KEYDOWN, vk, 0);
			PostMessage(target, WM_KEYUP, vk, 0xC0000000);
		};
		// A held direction repeats, as it does on the console screens.
		bool any = pad.Up || pad.Down || pad.Left || pad.Right;
		bool fresh = (pad.Up && !_previous.Up) || (pad.Down && !_previous.Down) || (pad.Left && !_previous.Left) || (pad.Right && !_previous.Right);
		if (any && (fresh || now >= _repeat_at)) {
			if (pad.Up) press(VK_UP);
			if (pad.Down) press(VK_DOWN);
			if (pad.Left) press(VK_LEFT);
			if (pad.Right) press(VK_RIGHT);
			_repeat_at = now + (fresh ? REPEAT_FIRST_MS : REPEAT_NEXT_MS);
		}
		if (pad.Accept && !_previous.Accept) press(VK_RETURN);
		if (pad.Back && !_previous.Back) press(VK_ESCAPE);
	}
	_previous = pad;
}
