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
#include "_rect.h"
#include "_rules.h"
#include "dbgprint.h"
#include "globals.h"
#include "goptions.h"
#include "house.h"
#include "infantry.h"
#include "infatype.h"
#include "init.h"
#include "misc.h"
#include "options.h"
#include "rules.h"
#include "session.h"
#include "unit.h"
#include "unittype.h"
#include "vidscale.h"
#include "voc.h"
#include "waypoint.h"
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


// Tells the player what a held cross just selected, in the message list at the top left.
static void Announce(char const * text)
{
	Session.Messages.Add_Message(NULL, 0, text, PlayerPtr->Scheme, TextPrintType(TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_FULLSHADOW), int(Rule->MessageDelay * TICKS_PER_MINUTE));
	Map.Flag_To_Redraw();
}


// Whether a unit fights: harvesters, engineers, and vehicles that deploy into buildings do not.
static bool Is_Combat(ObjectClass const * object)
{
	if (object->RTTI == RTTI_UNIT) {
		UnitClass const * unit = (UnitClass const *)object;
		return(!unit->Class->IsToHarvest && !unit->Class->IsToVeinHarvest && unit->Class->DeploysInto == NULL);
	}
	if (object->RTTI == RTTI_INFANTRY) {
		return(!((InfantryClass const *)object)->Class->IsEngineer);
	}
	return(false);
}


// Selects every combat unit on screen, through the engine's own select-in-view command.
static void Select_Combat_On_Screen(void)
{
	Execute_Command("SelectView");
	for (int index = CurrentObject.Count() - 1; index >= 0; index--) {
		ObjectClass * object = CurrentObject[index];
		if (!Is_Combat(object)) {
			object->Unselect();
		}
	}
}


// Selects every combat unit the player controls anywhere on the map.
static void Select_Combat_On_Map(void)
{
	auto take = [](ObjectClass * object) {
		HouseClass * house = object->Owner_HouseClass();
		if (house == NULL || !house->Is_Player_Control() || object->IsInLimbo) return;
		if (!object->Class_Of()->IsSelectable || !Is_Combat(object)) return;
		if (!object->IsSelected) object->Select();
	};
	for (int index = 0; index < Units.Count(); index++) take(Units[index]);
	for (int index = 0; index < Infantry.Count(); index++) take(Infantry[index]);
}


// The pad in play: the left stick and the d-pad move the pointer, R1 speeds
// it, and cross and circle are the mouse buttons, posted as the messages a mouse would send
// so every tactical behaviour follows.
static void Play_Input(GamepadStateType const & pad, GamepadStateType const & previous, unsigned long now)
{
	enum { STEP_CAP_MS = 50 };
	// The paces at the default setting, each scaled by the player's setting over the default.
	const float POINTER_RATE = 0.9f * Options.PadPointerSpeed / OptionsClass::PAD_SPEED_DEFAULT;		// Screen heights per second at full stick.
	const float PAD_RATE = 0.6f * Options.PadPointerSpeed / OptionsClass::PAD_SPEED_DEFAULT;			// The d-pad's steady rate.
	const float FAST_FACTOR = 1.0f + 1.2f * Options.PadFastSpeed / OptionsClass::PAD_SPEED_DEFAULT;
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
	if (pad.RightShoulder) {
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

	// In play the pointer keeps to the map: the sidebar is worked by the pad, so reaching
	// its edge scrolls the view as the screen's edge does.
	if (GameActive && ScenarioActive && TacticalRect.Width > 0) {
		POINT top_left = {TacticalRect.X, TacticalRect.Y};
		POINT bottom_right = {TacticalRect.X + TacticalRect.Width, TacticalRect.Y + TacticalRect.Height};
		Game_Point_To_Screen(top_left);
		Game_Point_To_Screen(bottom_right);
		// One pixel inside, so the pointer never rests on the tab bar or the sidebar itself.
		origin.x = std::max(origin.x, top_left.x + 1);
		origin.y = std::max(origin.y, top_left.y + 1);
		corner.x = std::min(corner.x, bottom_right.x - 1);
		corner.y = std::min(corner.y, bottom_right.y - 1);
	}

	_carry_x += vx * dt * height;
	_carry_y += vy * dt * height;
	int dx = int(_carry_x);
	int dy = int(_carry_y);
	_carry_x -= dx;
	_carry_y -= dy;
	if ((dx != 0 || dy != 0) && !Map.PadFocus) {
		POINT at;
		GetCursorPos(&at);
		long wanted_x = at.x + dx;
		long wanted_y = at.y + dy;
		at.x = std::clamp<long>(wanted_x, origin.x, corner.x - 1);
		at.y = std::clamp<long>(wanted_y, origin.y, corner.y - 1);
		SetCursorPos(at.x, at.y);

		// What the pointer could not travel past the screen's edge scrolls the map instead, so
		// the view moves at the pointer's own pace and the shoulder speeds both alike.
		float scale_x = float(VideoModeWidth) / float(corner.x - origin.x);
		float scale_y = float(VideoModeHeight) / float(corner.y - origin.y);
		static float _edge_x = 0.0f;
		static float _edge_y = 0.0f;
		_edge_x += float(wanted_x - at.x) * scale_x;
		_edge_y += float(wanted_y - at.y) * scale_y;
		int ex = int(_edge_x);
		int ey = int(_edge_y);
		_edge_x -= ex;
		_edge_y -= ey;
		if (ex != 0) {
			int distance = ex < 0 ? -ex : ex;
			Map.Scroll_Map(ex < 0 ? FACING_W : FACING_E, distance, true);
		}
		if (ey != 0) {
			int distance = ey < 0 ? -ey : ey;
			Map.Scroll_Map(ey < 0 ? FACING_N : FACING_S, distance, true);
		}
	}

	// The buttons go in as real input rather than posted messages, since the engine reads a
	// held button from the system's key state, which only real input sets.
	auto click = [&](DWORD flag, bool down) {
		INPUT input = {};
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = flag;
		if (down) _SyntheticClicks++;
		SendInput(1, &input, sizeof(input));
	};
	auto pressed = [&](bool now_down, bool was_down) { return(now_down && !was_down); };

	// Triangle takes the pad to the sidebar and back. There the d-pad or stick moves the
	// focus over the sections or the open section's grid, cross builds or opens, circle holds,
	// cancels or steps back, and square opens or closes a section's grid.
	enum { SIDEBAR_REPEAT_FIRST_MS = 350, SIDEBAR_REPEAT_NEXT_MS = 120 };
	static unsigned long _sidebar_repeat_at = 0;
	static bool _sidebar_held = false;
	enum { HOLD_MS = 500, WIDEN_MS = 400, DRAG_PIXELS = 3 };
	static unsigned long _cross_since = 0;
	static POINT _cross_at = {0, 0};
	static bool _cross_sent = false;
	static int _cross_stage = 0;		// 0 undecided, 1 type on screen, 3 combat on screen, 2 done.
	if (pressed(pad.Fourth, previous.Fourth) && !pad.LeftTrigger && !pad.LeftShoulder && !pad.RightShoulder) {
		if (Map.PadFocus) {
			Map.Pad_Leave();
		} else {
			Map.Pad_Enter();
		}
	}
	if (Map.PadFocus) {
		// A cross press spent on the sidebar must not become a click on the map when it is
		// let go after the sidebar hands the pointer back.
		if (pad.Accept) {
			_cross_stage = 2;
			_cross_sent = false;
		}
		// On the radar, cross held turns the stick and d-pad into a marker over the map, and
		// letting go jumps the view there.
		if (Map.PadRow == SidebarClass::PAD_ROW_RADAR && pad.Accept) {
			const float RADAR_RATE = 90.0f;		// Pixels per second at full stick.
			static float _radar_x = 0.0f;
			static float _radar_y = 0.0f;
			float rx = pad.StickX + (pad.PadRight ? 1.0f : 0.0f) - (pad.PadLeft ? 1.0f : 0.0f);
			float ry = -pad.StickY + (pad.PadDown ? 1.0f : 0.0f) - (pad.PadUp ? 1.0f : 0.0f);
			_radar_x += rx * dt * RADAR_RATE;
			_radar_y += ry * dt * RADAR_RATE;
			int nx = int(_radar_x);
			int ny = int(_radar_y);
			_radar_x -= nx;
			_radar_y -= ny;
			if (nx != 0 || ny != 0 || !Map.PadRadarHeld) {
				Map.Pad_Radar_Nudge(nx, ny);
			}
			return;
		}
		if (Map.PadRow == SidebarClass::PAD_ROW_RADAR && !pad.Accept && previous.Accept) {
			Map.Pad_Radar_Jump();
			return;
		}
		bool any = pad.Up || pad.Down || pad.Left || pad.Right;
		bool fresh = pressed(pad.Up, previous.Up) || pressed(pad.Down, previous.Down) || pressed(pad.Left, previous.Left) || pressed(pad.Right, previous.Right);
		if (any && (fresh || (_sidebar_held && now >= _sidebar_repeat_at))) {
			Map.Pad_Move(pad.Left ? -1 : pad.Right ? 1 : 0, pad.Up ? -1 : pad.Down ? 1 : 0);
			_sidebar_repeat_at = now + (fresh ? SIDEBAR_REPEAT_FIRST_MS : SIDEBAR_REPEAT_NEXT_MS);
		}
		_sidebar_held = any;
		if (pressed(pad.Accept, previous.Accept)) Map.Pad_Accept();
		if (pressed(pad.Back, previous.Back)) Map.Pad_Back();
		if (pressed(pad.Third, previous.Third) && !pad.LeftTrigger && !pad.LeftShoulder) Map.Pad_Toggle_Grid();
		return;
	}

	// Cross is the left button, but its press is held back until it is known what the
	// press is: movement makes it a band box, release makes it a click, and a still hold
	// selects the combat units on screen. With L1 it selects every unit of the type under
	// the pointer, widening to the whole map on a second press.
	// A still hold on one of the player's units selects every unit of its type on screen,
	// and holding on widens that to the whole map; a still hold on the ground selects the
	// combat units on screen.
	static bool _select_type_pending = false;
	if (_select_type_pending) {
		_select_type_pending = false;
		Execute_Command("SelectType");
	}
	if (pressed(pad.Accept, previous.Accept)) {
		_cross_since = now;
		GetCursorPos(&_cross_at);
		_cross_sent = false;
		// Under L1 or L2 the press belongs to team 4 below and must never become a click.
		_cross_stage = (pad.LeftTrigger || pad.LeftShoulder) ? 2 : 0;
	} else if (pad.Accept && previous.Accept && !_cross_sent && _cross_stage != 2) {
		POINT at;
		GetCursorPos(&at);
		int moved_x = at.x - _cross_at.x;
		int moved_y = at.y - _cross_at.y;
		if (moved_x < 0) moved_x = -moved_x;
		if (moved_y < 0) moved_y = -moved_y;
		if (_cross_stage == 0 && (moved_x > DRAG_PIXELS || moved_y > DRAG_PIXELS)) {
			click(MOUSEEVENTF_LEFTDOWN, true);
			_cross_sent = true;
		} else if (_cross_stage == 0 && now - _cross_since >= HOLD_MS) {
			ObjectClass * over = Map.HoverObject;
			bool own = over != NULL && over->Owner_HouseClass() != NULL && over->Owner_HouseClass()->Is_Player_Control();
			if (own) {
				click(MOUSEEVENTF_LEFTDOWN, true);
				click(MOUSEEVENTF_LEFTUP, false);
				_select_type_pending = true;
				Announce("All units of this type on screen selected");
				_cross_since = now;
				_cross_stage = 1;
			} else {
				Select_Combat_On_Screen();
				Announce("All combat units on screen selected");
				_cross_since = now;
				_cross_stage = 3;
			}
		} else if (_cross_stage == 1 && now - _cross_since >= WIDEN_MS) {
			Execute_Command("SelectType");
			Announce("All units of this type on the map selected");
			_cross_stage = 2;
		} else if (_cross_stage == 3 && now - _cross_since >= WIDEN_MS) {
			Select_Combat_On_Map();
			Announce("All combat units on the map selected");
			_cross_stage = 2;
		}
	} else if (!pad.Accept && previous.Accept) {
		if (_cross_sent) {
			click(MOUSEEVENTF_LEFTUP, false);
		} else if (_cross_stage == 0) {
			click(MOUSEEVENTF_LEFTDOWN, true);
			click(MOUSEEVENTF_LEFTUP, false);
		}
		_cross_sent = false;
		_cross_stage = 0;
	}
	// Circle is a right-button tap, pressed and released together, so holding it while the
	// pointer moves never becomes the mouse's drag scroll: it only cancels or deselects.
	// With R1 it works the sidebar's parked cell from the map: place, build again, or queue.
	// With L1 or L2 it belongs to the teams below, so no tap goes out to deselect them.
	// In waypoint mode it takes back the last waypoint placed while the path has one.
	if (pressed(pad.Back, previous.Back)) {
		if (pad.RightShoulder) {
			Map.Pad_Repeat();
		} else if (!pad.LeftTrigger && !pad.LeftShoulder) {
			WaypointPathClass * path = (Map.IsWaypointMode && PlayerPtr->SelectedPath != PATH_NONE) ? PlayerPtr->Paths[PlayerPtr->SelectedPath] : NULL;
			if (path != NULL && path->Waypoint_Count() > 0) {
				Execute_Command("DeleteWaypoint");
			} else {
				click(MOUSEEVENTF_RIGHTDOWN, true);
				click(MOUSEEVENTF_RIGHTUP, false);
			}
		}
	}

	// The shapes and cross with L2 make teams 1 to 4. With L1 one press selects a team and
	// a second press within the double tap window centres the view on it as well; square
	// alone cycles the sidebar modes.
	enum { DOUBLE_TAP_MS = 400 };
	static char const * const _make[4] = {"TeamCreate_1", "TeamCreate_2", "TeamCreate_3", "TeamCreate_4"};
	static char const * const _select[4] = {"TeamSelect_1", "TeamSelect_2", "TeamSelect_3", "TeamSelect_4"};
	static char const * const _centre[4] = {"TeamCenter_1", "TeamCenter_2", "TeamCenter_3", "TeamCenter_4"};
	static int _picked = -1;
	static unsigned long _picked_at = 0;
	bool shapes[4] = {pressed(pad.Third, previous.Third), pressed(pad.Fourth, previous.Fourth), pressed(pad.Back, previous.Back), pressed(pad.Accept, previous.Accept)};
	for (int index = 0; index < 4; index++) {
		if (!shapes[index]) continue;
		if (pad.LeftTrigger) {
			Execute_Command(_make[index]);
		} else if (pad.LeftShoulder) {
			bool again = _picked == index && now - _picked_at <= DOUBLE_TAP_MS;
			Execute_Command(again ? _centre[index] : _select[index]);
			_picked = again ? -1 : index;
			_picked_at = now;
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

	// R1 with L1 orders a force fire at the pointer and R1 with L2 a force move, as a Ctrl or
	// Alt click would, the moment the two are held together; no cross is needed.
	auto force_click = [&](WORD vk) {
		INPUT input[4] = {};
		input[0].type = INPUT_KEYBOARD;
		input[0].ki.wVk = vk;
		input[1].type = INPUT_MOUSE;
		input[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		input[2].type = INPUT_MOUSE;
		input[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		input[3].type = INPUT_KEYBOARD;
		input[3].ki.wVk = vk;
		input[3].ki.dwFlags = KEYEVENTF_KEYUP;
		_SyntheticClicks += 2;
		SendInput(4, input, sizeof(INPUT));
	};
	auto chord = [&](bool a, bool a_was, bool b, bool b_was) { return((pressed(a, a_was) && b) || (pressed(b, b_was) && a)); };
	if (chord(pad.RightShoulder, previous.RightShoulder, pad.LeftShoulder, previous.LeftShoulder)) force_click(VK_CONTROL);
	if (chord(pad.RightShoulder, previous.RightShoulder, pad.LeftTrigger, previous.LeftTrigger)) force_click(VK_MENU);

	// The right stick scrolls the map at the pad's own pace, apart from the mouse scroll
	// settings: a full push crosses a few view heights a second, and the squared response
	// keeps a light touch slow.
	{
		const float STICK_SCROLL_RATE = 3.0f * Options.PadScrollSpeed / OptionsClass::PAD_SPEED_DEFAULT;		// View heights per second at full stick.
		static float _scroll_x = 0.0f;
		static float _scroll_y = 0.0f;
		float rx = pad.RightStickX * (pad.RightStickX < 0 ? -pad.RightStickX : pad.RightStickX);
		float ry = -pad.RightStickY * (pad.RightStickY < 0 ? -pad.RightStickY : pad.RightStickY);
		float rate = TacticalRect.Height * STICK_SCROLL_RATE;
		_scroll_x += rx * dt * rate;
		_scroll_y += ry * dt * rate;
		int sx = int(_scroll_x);
		int sy = int(_scroll_y);
		_scroll_x -= sx;
		_scroll_y -= sy;
		if (sx != 0) {
			int distance = sx < 0 ? -sx : sx;
			Map.Scroll_Map(sx < 0 ? FACING_W : FACING_E, distance, true);
		}
		if (sy != 0) {
			int distance = sy < 0 ? -sy : sy;
			Map.Scroll_Map(sy < 0 ? FACING_N : FACING_S, distance, true);
		}
	}

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
