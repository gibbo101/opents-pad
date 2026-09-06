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
#include "globals.h"
#include "goptions.h"
#include "options.h"
#include "win.h"

#include <Xinput.h>

typedef DWORD (WINAPI * XInputGetStateType)(DWORD index, XINPUT_STATE * state);

enum {
	STICK_DEADZONE = 16000,
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
		result.Up = (pad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0 || pad.sThumbLY > STICK_DEADZONE;
		result.Down = (pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0 || pad.sThumbLY < -STICK_DEADZONE;
		result.Left = (pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0 || pad.sThumbLX < -STICK_DEADZONE;
		result.Right = (pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0 || pad.sThumbLX > STICK_DEADZONE;
		result.Accept = (pad.wButtons & XINPUT_GAMEPAD_A) != 0;
		result.Back = (pad.wButtons & XINPUT_GAMEPAD_B) != 0;
		result.Fast = (pad.wButtons & (XINPUT_GAMEPAD_RIGHT_SHOULDER|XINPUT_GAMEPAD_LEFT_SHOULDER)) != 0;
		result.Menu = (pad.wButtons & XINPUT_GAMEPAD_START) != 0;
		break;
	}
	return(result);
}


// The menu button is Escape everywhere: it skips a movie, opens the in-game menu, and
// backs out of a screen. Polling is held to once per frame because asking XInput about
// a controller that is not there is slow.
void Gamepad_Pump(void)
{
	static bool _menu_was_down = false;
	static unsigned long _next_poll = 0;

	if (Options.ControlScheme != CONTROL_CONTROLLER || Keyboard == NULL) {
		return;
	}
	unsigned long now = timeGetTime();
	if (now < _next_poll) {
		return;
	}
	_next_poll = now + 16;

	GamepadStateType pad = Gamepad_Read();
	if (pad.Menu && !_menu_was_down) {
		Keyboard->Put(KN_ESC);
		Keyboard->Put(KN_ESC | WWKEY_RLS_BIT);
	}
	_menu_was_down = pad.Menu;
}
