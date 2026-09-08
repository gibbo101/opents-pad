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
#include "_tactica.h"
#include "builtype.h"
#include "cell.h"
#include "dbgprint.h"
#include "globals.h"
#include "goptions.h"
#include "house.h"
#include "infantry.h"
#include "infatype.h"
#include "init.h"
#include "mainopt.h"
#include "misc.h"
#include "object.h"
#include "options.h"
#include "padglyph.h"
#include "rules.h"
#include "scenario.h"
#include "session.h"
#include "super.h"
#include "suprtype.h"
#include "tactical.h"
#include "techno.h"
#include "unit.h"
#include "unittype.h"
#include "video.h"
#include "vidscale.h"
#include "voc.h"
#include "waypoint.h"
#include "win.h"
#include "wincursor.h"

#include <Xinput.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

typedef DWORD (WINAPI * XInputGetStateType)(DWORD index, XINPUT_STATE * state);

enum {
	STICK_DEADZONE = 16000,
	POINTER_DEADZONE = 7000,
	TRIGGER_THRESHOLD = 64,
	PAD_COUNT = 4,
};

// Marks input the pad sends through the system so it never counts as a real keyboard or mouse.
constexpr ULONG_PTR PAD_INPUT_TAG = 0x50414431;

// The lparam of a WM_KEYUP: the key was down and is being released.
constexpr LPARAM KEY_RELEASE_LPARAM = 0xC0000000;


// XInput is loaded at first use so a machine without it still runs the game.
static XInputGetStateType Get_State_Function(void)
{
	static XInputGetStateType _function = nullptr;
	static bool _tried = false;

	if (!_tried) {
		_tried = true;
		static char const * const _names[] = {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"};
		for (char const * name : _names) {
			HMODULE module = LoadLibraryA(name);
			if (module != nullptr) {
				_function = (XInputGetStateType)GetProcAddress(module, "XInputGetState");
				if (_function != nullptr) {
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
static bool Kind_From_Linux_Devices(GamepadKindType & kind, bool & listed, bool log)
{
	FILE * file = fopen("Z:\\proc\\bus\\input\\devices", "r");
	if (file == nullptr) return(false);
	kind = GAMEPAD_KIND_UNKNOWN;
	listed = false;
	char line[512];
	unsigned vendor = 0;
	unsigned product = 0;
	while (fgets(line, sizeof(line), file) != nullptr) {
		if (line[0] == 'I') {
			vendor = product = 0;
			char const * v = strstr(line, "Vendor=");
			char const * p = strstr(line, "Product=");
			if (v != nullptr) vendor = strtoul(v + 7, nullptr, 16);
			if (p != nullptr) product = strtoul(p + 8, nullptr, 16);
		} else if (line[0] == 'N') {
			// Only game controllers count, not the pad's touchpad or motion sensors.
			bool pad = strstr(line, "Controller") != nullptr || strstr(line, "pad") != nullptr || strstr(line, "Joystick") != nullptr;
			bool extra = strstr(line, "Touchpad") != nullptr || strstr(line, "Motion") != nullptr;
			if (!pad || extra) continue;
			listed = true;
			if (log) {
				line[strcspn(line, "\r\n")] = '\0';
				DebugString("Game controller (host): vendor %04X product %04X %s\n", vendor, product, line + 3);
			}
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
	float value = float(raw);
	float size = std::abs(value);
	if (size <= POINTER_DEADZONE) return(0.0f);
	float scaled = std::min(1.0f, (size - POINTER_DEADZONE) / (32767.0f - POINTER_DEADZONE));
	return(value < 0 ? -scaled : scaled);
}


static bool _PadListed = false;		// The system's device list names a game controller.
static bool _PadSeen = false;		// A controller has been read as connected since launch.
static GamepadStateType _LastRead = {};
static unsigned long _LastReadAt = 0;


/// <summary>
/// Reports the make of the connected controller, or unknown when none is listed.
/// </summary>
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

	if (Kind_From_Linux_Devices(_kind, _PadListed, !_logged)) {
		_logged = true;
		return(_kind);
	}

	UINT count = 0;
	if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) != 0 || count == 0) {
		return(_kind);
	}
	std::vector<RAWINPUTDEVICELIST> list(count);
	count = GetRawInputDeviceList(list.data(), &count, sizeof(RAWINPUTDEVICELIST));
	GamepadKindType found = GAMEPAD_KIND_UNKNOWN;
	bool listed = false;
	if (count != (UINT)-1) {
		for (UINT index = 0; index < count; index++) {
			if (list[index].dwType != RIM_TYPEHID) continue;
			RID_DEVICE_INFO info;
			info.cbSize = sizeof(info);
			UINT size = sizeof(info);
			if (GetRawInputDeviceInfoA(list[index].hDevice, RIDI_DEVICEINFO, &info, &size) == (UINT)-1) continue;
			bool pad = info.hid.usUsagePage == USAGE_PAGE_DESKTOP && (info.hid.usUsage == USAGE_JOYSTICK || info.hid.usUsage == USAGE_GAMEPAD);
			if (!pad) continue;
			listed = true;
			if (!_logged) {
				DebugString("Game controller: vendor %04lX product %04lX usage %u\n", info.hid.dwVendorId, info.hid.dwProductId, info.hid.usUsage);
			}
			if (info.hid.dwVendorId == VENDOR_SONY) {
				found = GAMEPAD_KIND_PLAYSTATION;
			} else if (info.hid.dwVendorId == VENDOR_MICROSOFT && found == GAMEPAD_KIND_UNKNOWN) {
				found = GAMEPAD_KIND_XBOX;
			}
		}
	}
	_logged = true;
	_PadListed = listed;
	_kind = found;
	return(_kind);
}


/// <summary>
/// Reads the first connected controller. Returns a disconnected state when no controller
/// or no controller support is present.
/// </summary>
GamepadStateType Gamepad_Read(void)
{
	GamepadStateType result = {};

	XInputGetStateType get_state = Get_State_Function();
	if (get_state == nullptr) {
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
	if (result.Connected) {
		_PadSeen = true;
	}
	_LastRead = result;
	_LastReadAt = timeGetTime();
	return(result);
}


/// <summary>
/// Whether a controller is connected, from the latest read; rereads at most once a second.
/// </summary>
bool Gamepad_Connected(void)
{
	enum { RECHECK_MS = 1000 };
	unsigned long now = timeGetTime();
	if (_LastReadAt == 0 || now - _LastReadAt >= RECHECK_MS) {
		Gamepad_Read();
	}
	return(_LastRead.Connected);
}


static bool _KeyboardMouseSeen = false;
static bool _MenuStarts = false;
static bool _AutoSettled = false;
static int _ZoomStep = 0;
static bool _ZoomHeld = false;


static bool Pressed(bool now_down, bool was_down)
{
	return(now_down && !was_down);
}


// Buttons go in as real input, since the engine reads a held button from the system's key state.
static void Send_Mouse(DWORD flag)
{
	INPUT input = {};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = flag;
	input.mi.dwExtraInfo = PAD_INPUT_TAG;
	SendInput(1, &input, sizeof(input));
}


static void Send_Key(WORD vk, bool down)
{
	INPUT input = {};
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = vk;
	input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
	input.ki.dwExtraInfo = PAD_INPUT_TAG;
	SendInput(1, &input, sizeof(input));
}


// Tells the player what a held cross just selected, in the message list at the top left.
static void Announce(char const * text)
{
	Session.Messages.Add_Message(nullptr, 0, text, PlayerPtr->Scheme, TextPrintType(TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_FULLSHADOW), int(Rule->MessageDelay * TICKS_PER_MINUTE));
	Map.Flag_To_Redraw();
}


// Grants the player a charged shot of a superweapon, or charges it when the player holds it.
static void Cheat_Superweapon(SuperWeaponType type, char const * name)
{
	for (int index = 0; index < PlayerPtr->SuperWeapon.Count(); index++) {
		SuperClass * super = PlayerPtr->SuperWeapon[index];
		if (super->Class->Type != type) continue;
		if (super->Is_Present()) {
			super->Forced_Charge(true);
		} else {
			super->Enable(true, true, false);
			Map.Add(RTTI_SPECIAL, index);
		}
		DebugString("Cheat: %s (super %d, present %d, ready %d)\n", name, index, super->Is_Present(), super->Can_Place());
		Map.Column[1].Flag_To_Redraw();
		Announce(name);
		return;
	}
	// The rules in play define the weapons a house can hold, so one they leave out cannot be given.
	DebugString("Cheat: %s, no such weapon in this game\n", name);
	Announce("That weapon is not in this game");
}


// What the player had explored before the reveal cheat, a flag byte per cell indexed as the
// cell array is; EXPLORED_HELD marks a cell the map holds.
enum {
	EXPLORED_MAPPED = 1,
	EXPLORED_VISIBLE = 2,
	EXPLORED_FOG_MAPPED = 4,
	EXPLORED_FOG_VISIBLE = 8,
	EXPLORED_HELD = 16,
};
static std::vector<unsigned char> _ExploredBeforeReveal;

static void Remember_Explored(void)
{
	_ExploredBeforeReveal.assign(Map.Array.Length(), 0);
	Map.Reset_Iterator();
	for (CellClass * cell = Map.Iterate(); cell != nullptr; cell = Map.Iterate()) {
		_ExploredBeforeReveal[Cell_Index(cell->CellID)] = EXPLORED_HELD
			| (cell->IsMapped ? EXPLORED_MAPPED : 0) | (cell->IsVisible ? EXPLORED_VISIBLE : 0)
			| (cell->IsFogMapped ? EXPLORED_FOG_MAPPED : 0) | (cell->IsFogVisible ? EXPLORED_FOG_VISIBLE : 0);
	}
	RevealSighted.assign(Map.Array.Length(), 0);
}

// Puts the shroud back as it stood before the reveal, opens what the player's sight reached
// while the map was shown, then lets units look again. Without a matching record the whole
// map is shrouded.
static void Restore_Explored(void)
{
	bool matched = _ExploredBeforeReveal.size() == std::size_t(Map.Array.Length()) && RevealSighted.size() == _ExploredBeforeReveal.size();
	if (matched) {
		Map.Reset_Iterator();
		for (CellClass * cell = Map.Iterate(); cell != nullptr; cell = Map.Iterate()) {
			unsigned char flags = _ExploredBeforeReveal[Cell_Index(cell->CellID)];
			if (!(flags & EXPLORED_HELD)) continue;
			cell->IsMapped = (flags & EXPLORED_MAPPED) != 0;
			cell->IsVisible = (flags & EXPLORED_VISIBLE) != 0;
			cell->IsFogMapped = (flags & EXPLORED_FOG_MAPPED) != 0;
			cell->IsFogVisible = (flags & EXPLORED_FOG_VISIBLE) != 0;
		}
		Map.Reset_Iterator();
		for (CellClass * cell = Map.Iterate(); cell != nullptr; cell = Map.Iterate()) {
			if (RevealSighted[Cell_Index(cell->CellID)]) {
				Map.Map_Cell(cell->CellID, PlayerPtr);
			}
		}
	}
	_ExploredBeforeReveal.clear();
	RevealSighted.clear();
	if (!matched) {
		Map.Shroud_The_Map();
		return;
	}
	Map.All_To_Look();
	PlayerPtr->IsVisionary = false;
	Map.Complete_Radar_Refresh();
	Map.Flag_To_Redraw(GS_REDRAW_ALL);
}


// The cheat codes of Retaliation, keyed in on the sidebar's four mode buttons with circle:
// each press plays the click and enters the button as a symbol, and the last six symbols
// entered are matched against the codes. Only a solo game takes them.
static void Enter_Cheat_Symbol(int symbol)
{
	using enum SidebarClass::PadModeType;
	enum { CODE_LENGTH = 6, CHEAT_CREDITS = 5000 };
	struct CheatType {
		int Code[CODE_LENGTH];
		void (*Apply)(void);
	};
	// Retaliation keys its codes on a glyph row reading cross, circle, triangle, square, so
	// the mode buttons stand in for those in the same order.
	static CheatType const _cheats[] = {
		{{PAD_MODE_REPAIR, PAD_MODE_REPAIR, PAD_MODE_WAYPOINT, PAD_MODE_SELL, PAD_MODE_SELL, PAD_MODE_SELL}, []{ PlayerPtr->Refund_Money(CHEAT_CREDITS); Announce("Credits added"); }},
		{{PAD_MODE_POWER, PAD_MODE_POWER, PAD_MODE_REPAIR, PAD_MODE_SELL, PAD_MODE_POWER, PAD_MODE_WAYPOINT}, []{
			// The revealed map brings the radar with it, and the shroud takes back only the
			// radar the cheat gave.
			static bool _radar_given = false;
			if (PlayerPtr->IsVisionary) {
				Restore_Explored();
				if (_radar_given) Scen->IsFreeRadar = false;
				_radar_given = false;
				PlayerPtr->Recalc_Radar_Availability();
				Announce("Map shrouded");
			} else {
				Remember_Explored();
				Map.Reveal_The_Map(true);
				Map.Flag_To_Redraw(GS_REDRAW_ALL);
				_radar_given = !Scen->IsFreeRadar;
				Scen->IsFreeRadar = true;
				PlayerPtr->Recalc_Radar_Availability();
				Announce("Map revealed");
			}
		}},
		{{PAD_MODE_SELL, PAD_MODE_SELL, PAD_MODE_POWER, PAD_MODE_REPAIR, PAD_MODE_REPAIR, PAD_MODE_WAYPOINT}, []{ PlayerPtr->Flag_To_Win(); Announce("Mission won"); }},
		{{PAD_MODE_SELL, PAD_MODE_REPAIR, PAD_MODE_SELL, PAD_MODE_SELL, PAD_MODE_REPAIR, PAD_MODE_WAYPOINT}, []{ Cheat_Superweapon(SUPER_ION_CANNON, "Ion cannon ready"); }},
		{{PAD_MODE_WAYPOINT, PAD_MODE_SELL, PAD_MODE_POWER, PAD_MODE_REPAIR, PAD_MODE_SELL, PAD_MODE_SELL}, []{ Cheat_Superweapon(SUPER_MULTI_MISSILE, "Multi missile ready"); }},
		{{PAD_MODE_WAYPOINT, PAD_MODE_REPAIR, PAD_MODE_SELL, PAD_MODE_REPAIR, PAD_MODE_POWER, PAD_MODE_POWER}, []{ Cheat_Superweapon(SUPER_CHEM_MISSILE, "Chemical missile ready"); }},
		{{PAD_MODE_REPAIR, PAD_MODE_REPAIR, PAD_MODE_REPAIR, PAD_MODE_SELL, PAD_MODE_POWER, PAD_MODE_WAYPOINT}, []{ Cheat_Superweapon(SUPER_HUNTER_SEEKER, "Hunter seeker ready"); }},
		{{PAD_MODE_WAYPOINT, PAD_MODE_WAYPOINT, PAD_MODE_SELL, PAD_MODE_SELL, PAD_MODE_POWER, PAD_MODE_POWER}, []{ Cheat_Superweapon(SUPER_DROP_PODS, "Drop pods ready"); }},
	};
	static int _entered[CODE_LENGTH] = {-1, -1, -1, -1, -1, -1};

	if (Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH) return;
	Sound_Effect(Rule->GenericClick);
	for (int index = 1; index < CODE_LENGTH; index++) {
		_entered[index - 1] = _entered[index];
	}
	_entered[CODE_LENGTH - 1] = symbol;
	for (CheatType const & cheat : _cheats) {
		if (std::equal(std::begin(cheat.Code), std::end(cheat.Code), std::begin(_entered))) {
			for (int & entry : _entered) entry = -1;
			Sound_Effect(Rule->OptionsChanged);
			cheat.Apply();
			return;
		}
	}
}


// Whether a unit fights: one armed as it stands, or one that arms by deploying, so tick
// tanks and artillery count while harvesters, engineers, sensor arrays and the construction
// vehicle stay out.
static bool Is_Combat(ObjectClass const * object)
{
	if (object->RTTI != RTTI_UNIT && object->RTTI != RTTI_INFANTRY) return(false);
	TechnoTypeClass const * type = ((TechnoClass const *)object)->Techno_Type_Class();
	if (type == nullptr) return(false);
	if (type->Weapons[0].Weapon != nullptr) return(true);
	return(type->DeploysInto != nullptr && type->DeploysInto->Weapons[0].Weapon != nullptr);
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
		if (house == nullptr || !house->Is_Player_Control() || object->IsInLimbo) return;
		if (!object->Class_Of()->IsSelectable || !Is_Combat(object)) return;
		if (!object->IsSelected) object->Select();
	};
	for (int index = 0; index < Units.Count(); index++) take(Units[index]);
	for (int index = 0; index < Infantry.Count(); index++) take(Infantry[index]);
}


// A 60 Hz multimedia timer moves the OS pointer between frames; the game thread sets velocity
// and box and reads back the travel the box refused.
struct PointerMotionType
{
	float VelocityX;		// Screen pixels per second.
	float VelocityY;
	RECT Box;				// Screen pixels; the pointer stays inside, exclusive of right and bottom.
	bool Active;
	float CarryX;
	float CarryY;
	float EdgeX;			// Travel the box refused, in screen pixels, for the game thread.
	float EdgeY;
	unsigned long Last;
};
static PointerMotionType _Motion = {};
static CRITICAL_SECTION _MotionLock;
static bool _MotionLockReady = false;
static MMRESULT _MotionTimer = 0;
static DWORD _MotionOwner = 0;			// The game thread, whose input state the mover shares.
static DWORD _MotionAttached = 0;		// The timer thread already attached to it, or zero.
static bool _MotionAttachFailed = false;

static void CALLBACK Pointer_Motion_Tick(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR)
{
	enum { TICK_CAP_MS = 50 };
	// The pointer shown belongs to the thread that moves it, so the mover shares the game
	// thread's input state and the game's cursor and its hiding stay the game thread's.
	DWORD self = GetCurrentThreadId();
	if (_MotionAttached != self && _MotionOwner != 0 && _MotionOwner != self) {
		_MotionAttachFailed = !AttachThreadInput(self, _MotionOwner, TRUE);
		_MotionAttached = self;
		static bool _logged_join = false;
		if (!_logged_join || _MotionAttachFailed) {
			DebugString("Pad pointer thread %s the game thread's input\n", _MotionAttachFailed ? "could not join" : "joined");
			_logged_join = true;
		}
	}
	EnterCriticalSection(&_MotionLock);
	unsigned long now = timeGetTime();
	float dt = std::min<unsigned long>(now - _Motion.Last, TICK_CAP_MS) / 1000.0f;
	_Motion.Last = now;
	int dx = 0;
	int dy = 0;
	if (_Motion.Active) {
		_Motion.CarryX += _Motion.VelocityX * dt;
		_Motion.CarryY += _Motion.VelocityY * dt;
		dx = int(_Motion.CarryX);
		dy = int(_Motion.CarryY);
		_Motion.CarryX -= dx;
		_Motion.CarryY -= dy;
	}
	RECT box = _Motion.Box;
	LeaveCriticalSection(&_MotionLock);
	if (dx == 0 && dy == 0) {
		return;
	}

	POINT at;
	GetCursorPos(&at);
	long wanted_x = at.x + dx;
	long wanted_y = at.y + dy;
	at.x = std::clamp<long>(wanted_x, box.left, box.right - 1);
	at.y = std::clamp<long>(wanted_y, box.top, box.bottom - 1);
	// Without the shared input state this thread carries the game's cursor itself,
	// never the hidden one the game shows for a moment around each draw.
	if (_MotionAttachFailed) {
		static HCURSOR _carried = nullptr;
		HCURSOR current = Win_Cursor_Current();
		if (current != nullptr) {
			_carried = current;
		}
		if (_carried != nullptr) {
			SetCursor(_carried);
		}
	}
	SetCursorPos(at.x, at.y);

	EnterCriticalSection(&_MotionLock);
	_Motion.EdgeX += float(wanted_x - at.x);
	_Motion.EdgeY += float(wanted_y - at.y);
	LeaveCriticalSection(&_MotionLock);
}


// Stops the timer and waits out a tick in flight before the lock goes.
static void Gamepad_Shutdown(void)
{
	if (_MotionTimer != 0) {
		timeKillEvent(_MotionTimer);
		_MotionTimer = 0;
	}
	if (_MotionLockReady) {
		EnterCriticalSection(&_MotionLock);
		LeaveCriticalSection(&_MotionLock);
		DeleteCriticalSection(&_MotionLock);
		_MotionLockReady = false;
	}
}


// Sets the pointer's motion for the coming polls, starting the timer on the first active
// call and stopping it when the pointer comes to rest. Returns the travel the box refused
// since the last call, in screen pixels.
static void Pointer_Motion_Set(float vx, float vy, RECT const & box, bool active, float & edge_x, float & edge_y)
{
	enum { TICK_MS = 16 };		// A warp a frame at 60 Hz.
	if (!_MotionLockReady) {
		InitializeCriticalSection(&_MotionLock);
		_MotionLockReady = true;
		atexit(Gamepad_Shutdown);
	}
	_MotionOwner = GetCurrentThreadId();
	EnterCriticalSection(&_MotionLock);
	_Motion.VelocityX = vx;
	_Motion.VelocityY = vy;
	_Motion.Box = box;
	if (active && !_Motion.Active) {
		_Motion.CarryX = 0.0f;
		_Motion.CarryY = 0.0f;
		_Motion.Last = timeGetTime();
	}
	_Motion.Active = active;
	edge_x = _Motion.EdgeX;
	edge_y = _Motion.EdgeY;
	_Motion.EdgeX = 0.0f;
	_Motion.EdgeY = 0.0f;
	LeaveCriticalSection(&_MotionLock);
	if (active && _MotionTimer == 0) {
		_MotionTimer = timeSetEvent(TICK_MS, 1, Pointer_Motion_Tick, 0, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
	} else if (!active && _MotionTimer != 0) {
		timeKillEvent(_MotionTimer);
		_MotionTimer = 0;
	}
}


static void Pointer_Motion_Stop(void)
{
	if (_MotionTimer == 0) return;
	RECT none = {};
	float edge_x;
	float edge_y;
	Pointer_Motion_Set(0.0f, 0.0f, none, false, edge_x, edge_y);
}


// Scrolls the map by a distance in map pixels, carrying the fraction under a pixel forward.
static void Scroll_Map_By(float & carry_x, float & carry_y, float dx, float dy)
{
	carry_x += dx;
	carry_y += dy;
	int sx = int(carry_x);
	int sy = int(carry_y);
	carry_x -= sx;
	carry_y -= sy;
	if (sx != 0) {
		int distance = std::abs(sx);
		Map.Scroll_Map(sx < 0 ? FACING_W : FACING_E, distance, true);
	}
	if (sy != 0) {
		int distance = std::abs(sy);
		Map.Scroll_Map(sy < 0 ? FACING_N : FACING_S, distance, true);
	}
}


// When the pointer comes to rest near a unit or building, it is drawn onto that object's
// centre, so a target is hit without pixel aim. Travel is untouched, and a pointer already
// over an object stays where it is.
static void Snap_Pointer_To_Object(void)
{
	enum { SNAP_CELLS = 2 };			// Cells searched each way from the pointer's.
	int snap_radius = Options.PadSnap * OptionsClass::PAD_SNAP_STEP;		// Map pixels.
	if (snap_radius <= 0 || Map.PadFocus || TacticalMap == nullptr || TacticalRect.Width <= 0) {
		return;
	}
	POINT at;
	GetCursorPos(&at);
	Screen_Point_To_Game(at);
	Point2D local(at.x - TacticalRect.X, at.y - TacticalRect.Y);
	if (local.X < 0 || local.Y < 0 || local.X >= TacticalRect.Width || local.Y >= TacticalRect.Height) {
		return;
	}
	Cell cell;
	Coord coord;
	ObjectClass * under = nullptr;
	bool fog = false;
	bool shadow = false;
	Map.Resolve_Point(local, cell, coord, under, fog, shadow);
	if (under != nullptr) {
		return;
	}
	ObjectClass * best = nullptr;
	Point2D best_pixel;
	int best_distance = snap_radius * snap_radius + 1;
	for (int dy = -SNAP_CELLS; dy <= SNAP_CELLS; dy++) {
		for (int dx = -SNAP_CELLS; dx <= SNAP_CELLS; dx++) {
			Cell around = cell + Cell(dx, dy);
			if (!Map.In_Radar(around)) continue;
			// Only what the player can see is a target.
			if (Map[around].Is_Shrouded()) continue;
			if (Scen->Special.IsFogOfWar && Map[around].Is_Fogged()) continue;
			for (ObjectClass * object = Map[around].Cell_Occupier(); object != nullptr; object = object->Next) {
				int kind = object->What_Am_I();
				if (kind != RTTI_UNIT && kind != RTTI_INFANTRY && kind != RTTI_AIRCRAFT && kind != RTTI_BUILDING) continue;
				// The four kinds are all technos, so the cast holds.
				TechnoClass * techno = static_cast<TechnoClass *>(object);
				if (!techno->IsOwnedByPlayer && techno->Cloak == CLOAKED) continue;
				Point2D pixel;
				TacticalMap->Coord_To_Pixel(object->Center_Coord(), pixel);
				int ox = pixel.X - local.X;
				int oy = pixel.Y - local.Y;
				int distance = ox * ox + oy * oy;
				if (distance < best_distance) {
					best_distance = distance;
					best = object;
					best_pixel = pixel;
				}
			}
		}
	}
	if (best == nullptr || best_pixel == local) {
		return;
	}
	POINT target = {TacticalRect.X + best_pixel.X, TacticalRect.Y + best_pixel.Y};
	Game_Point_To_Screen(target);
	SetCursorPos(target.x, target.y);
}




// The pad in play: the left stick and the d-pad move the pointer, R1 speeds
// it, and cross and circle are the mouse buttons, posted as the messages a mouse would send
// so every tactical behaviour follows.
static void Play_Input(GamepadStateType const & pad, GamepadStateType const & previous, unsigned long now)
{
	enum { STEP_CAP_MS = 50 };
	// The paces at the default setting, each scaled by the player's setting over the default.
	const float POINTER_RATE = 1.8f * Options.PadPointerSpeed / OptionsClass::PAD_SPEED_DEFAULT;		// Screen heights per second at full stick.
	const float PAD_RATE = 1.2f * Options.PadPointerSpeed / OptionsClass::PAD_SPEED_DEFAULT;			// The d-pad's full rate.
	const float PAD_START = 0.5f;		// The share of that rate a press starts at, so a tap stays within a cell.
	const float PAD_RAMP_MS = 150.0f;	// How long a press takes to reach the full rate, short enough not to be felt.
	const float FAST_FACTOR = 1.0f + 1.2f * Options.PadFastSpeed / OptionsClass::PAD_SPEED_DEFAULT;
	static unsigned long _last = 0;

	float dt = (_last == 0 ? 16 : std::min<unsigned long>(now - _last, STEP_CAP_MS)) / 1000.0f;
	_last = now;

	// The stick's response is straight.
	float vx = pad.StickX * POINTER_RATE;
	float vy = -pad.StickY * POINTER_RATE;

	// The d-pad starts at half pace and is at full pace before a tap is over, so a tap
	// stays short and a hold travels.
	static unsigned long _pad_since = 0;
	bool pad_held = pad.PadLeft || pad.PadRight || pad.PadUp || pad.PadDown;
	if (!pad_held) {
		_pad_since = 0;
	} else if (_pad_since == 0) {
		_pad_since = now;
	}
	float pad_rate = pad_held ? PAD_RATE * std::min(1.0f, PAD_START + (1.0f - PAD_START) * float(now - _pad_since) / PAD_RAMP_MS) : 0.0f;
	if (pad.PadLeft) vx -= pad_rate;
	if (pad.PadRight) vx += pad_rate;
	if (pad.PadUp) vy -= pad_rate;
	if (pad.PadDown) vy += pad_rate;
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
	// The paces are in map pixels, so a tap or a push covers the same ground at every zoom
	// and on every panel: a screen of 600 map pixels is the yardstick, the Deck's baseline.
	enum { PACE_HEIGHT = 600 };
	float height = float(PACE_HEIGHT) * float(corner.y - origin.y) / float(std::max(VideoModeHeight, 1));

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
		// A panel that is in covers the map's edge, so the pointer stops at the panel instead.
		VideoScaleInfo const & layout = Video_Get_Scale_Info();
		if (layout.SidebarOverlay && Map.PadPanel == SidebarClass::PAD_PANEL_SHOWN) {
			POINT edge = {layout.SidebarDestX, 0};
			POINT beyond = {layout.SidebarDestX + layout.SidebarDestWidth, 0};
			ClientToScreen(MainWindow, &edge);
			ClientToScreen(MainWindow, &beyond);
			if (layout.SidebarOnRight) {
				corner.x = std::min(corner.x, edge.x - 1);
			} else {
				origin.x = std::max(origin.x, beyond.x + 1);
			}
		}
	}

	RECT box = {origin.x, origin.y, corner.x, corner.y};
	bool moving = (vx != 0.0f || vy != 0.0f) && !Map.PadFocus;
	float refused_x;
	float refused_y;
	Pointer_Motion_Set(vx * height, vy * height, box, moving, refused_x, refused_y);
	static bool _was_moving = false;
	if (_was_moving && !moving && !pad.Accept) {
		Snap_Pointer_To_Object();
	}
	_was_moving = moving;
	if (refused_x != 0.0f || refused_y != 0.0f) {
		// What the pointer could not travel past the screen's edge scrolls the map instead, so
		// the view moves at the pointer's own pace and the shoulder speeds both alike.
		float scale_x = float(TacticalRect.Width) / float(std::max<long>(corner.x - origin.x, 1));
		float scale_y = float(TacticalRect.Height) / float(std::max<long>(corner.y - origin.y, 1));
		static float _edge_x = 0.0f;
		static float _edge_y = 0.0f;
		Scroll_Map_By(_edge_x, _edge_y, refused_x * scale_x, refused_y * scale_y);
	}

	// Triangle takes the pad to the sidebar and back. There the d-pad or stick moves the
	// focus over the sections or the open section's grid, cross builds or opens, circle holds,
	// cancels or steps back, and square opens or closes a section's grid.
	enum { SIDEBAR_REPEAT_FIRST_MS = 350, SIDEBAR_REPEAT_NEXT_MS = 120 };
	static unsigned long _sidebar_repeat_at = 0;
	static bool _sidebar_held = false;
	enum { TAP_MS = 350 };
	static unsigned long _cross_since = 0;
	static POINT _cross_at = {0, 0};
	static bool _cross_sent = false;
	static bool _cross_claimed = false;
	static int _cross_taps = 0;
	static unsigned long _cross_tapped = 0;
	if (Pressed(pad.Fourth, previous.Fourth) && !pad.LeftTrigger && !pad.LeftShoulder) {
		if (pad.RightShoulder) {
			if (Map.PadPinned) {
				Map.PadPinned = false;
				Map.Pad_Leave();
			} else {
				Map.Pad_Panel_Show(true, true);
			}
			Options.PadSidebarSticky = Map.PadPinned;
			Options.Save_Settings();
		} else if (Map.PadFocus) {
			Map.Pad_Leave();
		} else {
			Map.Pad_Panel_Show(false, true);
		}
	}
	if (Map.PadFocus) {
		// A cross press spent on the sidebar must not become a click on the map when it is
		// let go after the sidebar hands the pointer back.
		if (pad.Accept) {
			_cross_claimed = true;
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
		bool fresh = Pressed(pad.Up, previous.Up) || Pressed(pad.Down, previous.Down) || Pressed(pad.Left, previous.Left) || Pressed(pad.Right, previous.Right);
		if (any && (fresh || (_sidebar_held && now >= _sidebar_repeat_at))) {
			Map.Pad_Move(pad.Left ? -1 : pad.Right ? 1 : 0, pad.Up ? -1 : pad.Down ? 1 : 0);
			_sidebar_repeat_at = now + (fresh ? SIDEBAR_REPEAT_FIRST_MS : SIDEBAR_REPEAT_NEXT_MS);
		}
		_sidebar_held = any;
		if (Pressed(pad.Accept, previous.Accept)) Map.Pad_Accept();
		if (Pressed(pad.Back, previous.Back)) {
			if (Map.PadRow == SidebarClass::PAD_ROW_MODES) {
				Enter_Cheat_Symbol(Map.PadCol);
			} else {
				Map.Pad_Back();
			}
		}
		if (Pressed(pad.Third, previous.Third) && !pad.LeftTrigger && !pad.LeftShoulder) Map.Pad_Toggle_Grid(true);
		return;
	}

	// Cross is the left button, but its press is held back until it is known what the press
	// is: movement makes it a band box and release a click. A second tap on one of the
	// player's units selects its type on screen and a third the type across the map; on the
	// ground the taps select the combat units on screen, then across the map.
	if (Pressed(pad.Accept, previous.Accept)) {
		_cross_since = now;
		GetCursorPos(&_cross_at);
		_cross_sent = false;
		// Under L1 or L2 the press belongs to team 4 below and must never become a click.
		_cross_claimed = pad.LeftTrigger || pad.LeftShoulder;
	} else if (pad.Accept && previous.Accept && !_cross_sent && !_cross_claimed) {
		POINT at;
		GetCursorPos(&at);
		int moved_x = std::abs(at.x - _cross_at.x);
		int moved_y = std::abs(at.y - _cross_at.y);
		// The same distance the engine wants before a held button becomes a band, a
		// twenty-fifth of the view's height, in the screen's pixels.
		int drag_pixels = std::max(4, int(float(TacticalRect.Height) / 25.0f * float(corner.y - origin.y) / float(std::max(VideoModeHeight, 1))));
		if (moved_x > drag_pixels || moved_y > drag_pixels) {
			Send_Mouse(MOUSEEVENTF_LEFTDOWN);
			_cross_sent = true;
			_cross_taps = 0;
		}
	} else if (!pad.Accept && previous.Accept) {
		if (_cross_sent) {
			Send_Mouse(MOUSEEVENTF_LEFTUP);
		} else if (!_cross_claimed) {
			_cross_taps = (now - _cross_tapped <= TAP_MS) ? _cross_taps + 1 : 1;
			_cross_tapped = now;
			ObjectClass * over = Map.HoverObject;
			bool own = over != nullptr && over->Owner_HouseClass() != nullptr && over->Owner_HouseClass()->Is_Player_Control();
			if (_cross_taps == 1) {
				Send_Mouse(MOUSEEVENTF_LEFTDOWN);
				Send_Mouse(MOUSEEVENTF_LEFTUP);
			} else if (own) {
				Execute_Command("SelectType");
				Announce(_cross_taps == 2 ? "All units of this type on screen selected" : "All units of this type on the map selected");
			} else if (_cross_taps == 2) {
				Select_Combat_On_Screen();
				Announce("All combat units on screen selected");
			} else {
				Select_Combat_On_Map();
				Announce("All combat units on the map selected");
			}
		}
		_cross_sent = false;
		_cross_claimed = false;
	}
	// Circle is a right-button tap, never a held drag; with R1 it repeats the parked sidebar
	// cell, and under L1/L2 it belongs to the team chords.
	if (Pressed(pad.Back, previous.Back)) {
		if (pad.RightShoulder) {
			Map.Pad_Repeat();
		} else if (!pad.LeftTrigger && !pad.LeftShoulder) {
			Send_Mouse(MOUSEEVENTF_RIGHTDOWN);
			Send_Mouse(MOUSEEVENTF_RIGHTUP);
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
	bool shapes[4] = {Pressed(pad.Third, previous.Third), Pressed(pad.Fourth, previous.Fourth), Pressed(pad.Back, previous.Back), Pressed(pad.Accept, previous.Accept)};
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
	if (Pressed(pad.Third, previous.Third) && !pad.LeftTrigger && !pad.LeftShoulder) {
		// The modes in their order, off last. A mode the engine refuses, as repair, sell and
		// power do without a building, is passed over so the cycle always moves on.
		enum { MODE_REPAIR, MODE_SELL, MODE_POWER, MODE_WAYPOINT, MODE_OFF };
		int current = Map.IsRepairMode ? MODE_REPAIR : Map.IsSellMode ? MODE_SELL : Map.IsPowerMode ? MODE_POWER : Map.IsWaypointMode ? MODE_WAYPOINT : MODE_OFF;
		auto set_mode = [](int mode, int on) {
			switch (mode) {
				case MODE_REPAIR: Map.Repair_Mode_Control(on); return(bool(Map.IsRepairMode) == bool(on));
				case MODE_SELL: Map.Sell_Mode_Control(on); return(bool(Map.IsSellMode) == bool(on));
				case MODE_POWER: Map.Power_Mode_Control(on); return(bool(Map.IsPowerMode) == bool(on));
				case MODE_WAYPOINT: Map.Waypoint_Mode_Control(on); return(bool(Map.IsWaypointMode) == bool(on));
				default: return(true);
			}
		};
		if (current != MODE_OFF) {
			set_mode(current, 0);
		}
		for (int mode = current == MODE_OFF ? MODE_REPAIR : current + 1; mode < MODE_OFF; mode++) {
			if (set_mode(mode, 1)) break;
		}
	}

	// R1 with L1 orders a force fire at the pointer and R1 with L2 a force move, as a Ctrl or
	// Alt click would, the moment the two are held together. The modifier key stays down until
	// the chord releases: the engine reads it from live key state after the click.
	static WORD _force_vk = 0;
	static bool _force_with_shoulder = false;
	auto force_click = [&](WORD vk, bool with_shoulder) {
		if (_force_vk != 0 && _force_vk != vk) Send_Key(_force_vk, false);
		_force_vk = vk;
		_force_with_shoulder = with_shoulder;
		Send_Key(vk, true);
		Send_Mouse(MOUSEEVENTF_LEFTDOWN);
		Send_Mouse(MOUSEEVENTF_LEFTUP);
	};
	auto chord = [&](bool a, bool a_was, bool b, bool b_was) { return((Pressed(a, a_was) && b) || (Pressed(b, b_was) && a)); };
	if (chord(pad.RightShoulder, previous.RightShoulder, pad.LeftShoulder, previous.LeftShoulder)) force_click(VK_CONTROL, true);
	if (chord(pad.RightShoulder, previous.RightShoulder, pad.LeftTrigger, previous.LeftTrigger)) force_click(VK_MENU, false);
	if (_force_vk != 0 && (!pad.RightShoulder || !(_force_with_shoulder ? pad.LeftShoulder : pad.LeftTrigger))) {
		Send_Key(_force_vk, false);
		_force_vk = 0;
	}

	// R1 with the right stick steps the zoom, up to zoom in, one step per push and again
	// every quarter second held. The stick is the zoom's while R1 is held, so it does not scroll.
	{
		enum { ZOOM_REPEAT_MS = 250 };
		const float ZOOM_PUSH = 0.5f;
		static unsigned long _zoom_at = 0;
		_ZoomHeld = pad.RightShoulder && (pad.RightStickY > ZOOM_PUSH || pad.RightStickY < -ZOOM_PUSH);
		if (_ZoomHeld) {
			if (_zoom_at == 0 || now >= _zoom_at) {
				_ZoomStep = pad.RightStickY > 0 ? 1 : -1;
				_zoom_at = now + ZOOM_REPEAT_MS;
			}
		} else {
			_zoom_at = 0;
		}
	}

	// The right stick scrolls the map at the pad's own pace, apart from the mouse scroll
	// settings: a full push crosses a few view heights a second, and the squared response
	// keeps a light touch slow.
	if (!pad.RightShoulder) {
		const float STICK_SCROLL_RATE = 3.0f * Options.PadScrollSpeed / OptionsClass::PAD_SPEED_DEFAULT;		// View heights per second at full stick.
		static float _scroll_x = 0.0f;
		static float _scroll_y = 0.0f;
		float rx = pad.RightStickX * std::abs(pad.RightStickX);
		float ry = -pad.RightStickY * std::abs(pad.RightStickY);
		float rate = TacticalRect.Height * STICK_SCROLL_RATE;
		Scroll_Map_By(_scroll_x, _scroll_y, rx * dt * rate, ry * dt * rate);
	}

	if (Pressed(pad.LeftThumb, previous.LeftThumb)) Execute_Command("DeployObject");
	if (Pressed(pad.RightThumb, previous.RightThumb)) Execute_Command("CenterBase");
	// R2 scatters, or with R1 guards; in waypoint mode, where neither has a job, it takes back
	// the last waypoint placed while the path has one.
	if (Pressed(pad.RightTrigger, previous.RightTrigger)) {
		WaypointPathClass * path = (Map.IsWaypointMode && PlayerPtr->SelectedPath != PATH_NONE) ? PlayerPtr->Paths[PlayerPtr->SelectedPath] : nullptr;
		if (path != nullptr && path->Waypoint_Count() > 0) {
			Execute_Command("DeleteWaypoint");
		} else {
			Execute_Command(pad.RightShoulder ? "GuardObject" : "ScatterObject");
		}
	}
	// View centres on the last radar event in a solo game, where allies mean nothing, and
	// allies with the selected unit's owner in a network game.
	if (Pressed(pad.View, previous.View)) {
		bool solo = Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH;
		Execute_Command(solo ? "CenterOnRadarEvent" : "ToggleAlliance");
	}
}


// The pad states read in play since the last frame, kept for the main loop to act on. The
// pump samples from inside the message loop, where play actions must not run.
struct PlaySampleType
{
	GamepadStateType Pad;
	GamepadStateType Previous;
	unsigned long Time;
};
enum { PLAY_SAMPLE_CAP = 64 };
static std::vector<PlaySampleType> _PlaySamples;


static bool Play_Allowed(void)
{
	return(ScenarioActive && !IgnoreInput && GameInFocus && Options.ControlScheme == CONTROL_CONTROLLER);
}


void Gamepad_Centre_Pointer(void)
{
	if (Options.ControlScheme != CONTROL_CONTROLLER || TacticalRect.Width <= 0) {
		return;
	}
	POINT centre = {TacticalRect.X + TacticalRect.Width / 2, TacticalRect.Y + TacticalRect.Height / 2};
	Game_Point_To_Screen(centre);
	SetCursorPos(centre.x, centre.y);
}


/// <summary>
/// Applies the pad's play input since the last frame, then a zoom step it asked for and the
/// sidebar panel's slide with the tab bar's redraw. Call from the main loop between frames.
/// </summary>
PadButtonType Gamepad_Team_Button(int team)
{
	// Teams 1 to 4 are made and selected with the face buttons in this order.
	static PadButtonType const _buttons[4] = {PAD_BUTTON_THIRD, PAD_BUTTON_FOURTH, PAD_BUTTON_BACK, PAD_BUTTON_ACCEPT};
	if (team < 0 || team >= 4) return(PAD_BUTTON_COUNT);
	return(_buttons[team]);
}


void Gamepad_Frame_Tick(void)
{
	if (!ScenarioActive || Options.ControlScheme != CONTROL_CONTROLLER) {
		_PlaySamples.clear();
		_ZoomStep = 0;
		return;
	}
	if (Play_Allowed()) {
		for (PlaySampleType const & sample : _PlaySamples) {
			Play_Input(sample.Pad, sample.Previous, sample.Time);
		}
	}
	_PlaySamples.clear();
	// The bar's credits ride the panel's edge, so the bar is redrawn through a slide and
	// once more when it settles.
	static bool _was_sliding = false;
	bool sliding = Video_Sidebar_Sliding();
	Map.Pad_Panel_Tick();
	if (sliding || _was_sliding) {
		Map.Redraw_Tab();
	}
	_was_sliding = sliding;
	int steps = _ZoomStep;
	_ZoomStep = 0;
	if (steps != 0) {
		Pad_Zoom_Step(steps);
	} else if (!_ZoomHeld) {
		Pad_Zoom_Save();
	}
}


/// <summary>
/// Fixes an Auto control scheme before the first shell screen: a pad that has appeared since
/// launch selects the controller scheme. With none yet the call waits up to the given time
/// only while the system lists a controller. After this the scheme no longer follows the pad.
/// </summary>
void Gamepad_Settle_Auto_Scheme(unsigned wait_ms)
{
	if (_AutoSettled) return;
	if (Options.ControlSchemeAuto && Options.ControlScheme != CONTROL_CONTROLLER) {
		// Only a controller the system lists, or one seen since launch, is worth waiting for.
		Gamepad_Kind();
		unsigned long until = timeGetTime() + ((_PadListed || _PadSeen) ? wait_ms : 0);
		for (;;) {
			if (Gamepad_Read().Connected) {
				DebugString("ControlScheme is Controller (auto, pad found before the shell)\n");
				Options.ControlScheme = CONTROL_CONTROLLER;
				break;
			}
			if (timeGetTime() >= until) break;
			Sleep(50);
		}
	}
	_AutoSettled = true;
}


/// <summary>
/// While on, the menu button is left to the screen that asked instead of pressing Escape,
/// for a screen whose accept is Start.
/// </summary>
void Gamepad_Menu_Starts(bool on)
{
	_MenuStarts = on;
}


bool Gamepad_Menu_Starting(void)
{
	return(_MenuStarts);
}


/// <summary>
/// Has a real keyboard key or mouse button been pressed on the game window since launch?
/// Steam presents a mouse and keyboard whether or not the player has one, so use is the
/// only evidence. Input the pad sent through the system is not counted.
/// </summary>
bool Keyboard_Mouse_Seen(void)
{
	return(_KeyboardMouseSeen);
}


void Note_Keyboard_Mouse_Use(void)
{
	if ((ULONG_PTR)GetMessageExtraInfo() == PAD_INPUT_TAG) return;
	_KeyboardMouseSeen = true;
}


void Note_Keyboard_Mouse_Reset(void)
{
	_KeyboardMouseSeen = false;
}


/// <summary>
/// Samples the controller for the message pump: the menu button presses Escape into the
/// keyboard buffer, the topmost open dialog (nullptr when none) gets the d-pad, accept, and
/// back as key messages, and play input is kept for Gamepad_Frame_Tick.
/// </summary>
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

	if (Keyboard == nullptr) {
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
			if (dialog != nullptr) {
				PostMessage((HWND)dialog, WM_KEYDOWN, VK_ESCAPE, 0);
				PostMessage((HWND)dialog, WM_KEYUP, VK_ESCAPE, KEY_RELEASE_LPARAM);
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
	bool fresh_press = Pressed(pad.Accept, _previous.Accept) || Pressed(pad.Back, _previous.Back) || Pressed(pad.Menu, _previous.Menu)
		|| Pressed(pad.Up, _previous.Up) || Pressed(pad.Down, _previous.Down) || Pressed(pad.Left, _previous.Left) || Pressed(pad.Right, _previous.Right);
	if (!controller && _AutoSettled && Options.ControlSchemeAuto && !ScenarioActive && fresh_press) {
		DebugString("ControlScheme is Controller (auto, pad pressed in the shell)\n");
		Options.ControlScheme = CONTROL_CONTROLLER;
		_previous = pad;
		return;
	}
	if (!controller) {
		_previous = pad;
		return;
	}
	if (Pressed(pad.Menu, _previous.Menu) && !_MenuStarts) {
		Keyboard->Put(KN_ESC);
		Keyboard->Put(KN_ESC | WWKEY_RLS_BIT);
	}

	if (dialog == nullptr && Play_Allowed()) {
		if (_PlaySamples.size() >= PLAY_SAMPLE_CAP) {
			_PlaySamples.erase(_PlaySamples.begin());
		}
		_PlaySamples.push_back({pad, _previous, now});
		_previous = pad;
		return;
	}
	_PlaySamples.clear();
	Pointer_Motion_Stop();

	HWND window = (HWND)dialog;
	if (window != nullptr) {
		auto press = [&](int vk) {
			HWND target = GetFocus();
			if (target == nullptr || !IsChild(window, target)) {
				target = window;
			}
			PostMessage(target, WM_KEYDOWN, vk, 0);
			PostMessage(target, WM_KEYUP, vk, KEY_RELEASE_LPARAM);
		};
		// A held direction repeats, as it does on the console screens.
		bool any = pad.Up || pad.Down || pad.Left || pad.Right;
		bool fresh = Pressed(pad.Up, _previous.Up) || Pressed(pad.Down, _previous.Down) || Pressed(pad.Left, _previous.Left) || Pressed(pad.Right, _previous.Right);
		if (any && (fresh || now >= _repeat_at)) {
			if (pad.Up) press(VK_UP);
			if (pad.Down) press(VK_DOWN);
			if (pad.Left) press(VK_LEFT);
			if (pad.Right) press(VK_RIGHT);
			_repeat_at = now + (fresh ? REPEAT_FIRST_MS : REPEAT_NEXT_MS);
		}
		if (Pressed(pad.Accept, _previous.Accept)) press(VK_RETURN);
		if (Pressed(pad.Back, _previous.Back)) press(VK_ESCAPE);
	}
	_previous = pad;
}
