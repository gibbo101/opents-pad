/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "padglyph.h"

/*
 * The state of the first connected controller, reduced to what a menu needs. The
 * directions combine the d-pad and the left stick.
 */
struct GamepadStateType
{
	bool Connected;
	bool Up;
	bool Down;
	bool Left;
	bool Right;
	bool Accept;
	bool Back;
	bool Third;		// X, or square.
	bool Fourth;	// Y, or triangle.
	bool Fast;		// Either shoulder button.
	bool Menu;
	bool View;		// Back, or select.
	bool LeftShoulder;
	bool RightShoulder;
	bool LeftTrigger;
	bool RightTrigger;
	bool LeftThumb;		// The left stick pressed in.
	bool RightThumb;
	bool PadUp;			// The d-pad alone, apart from the stick.
	bool PadDown;
	bool PadLeft;
	bool PadRight;
	float StickX;		// The left stick, -1 to 1, zero within the dead zone.
	float StickY;		// Up is positive.
	float RightStickX;
	float RightStickY;
};

// What make of controller the system reports, from its device list.
enum GamepadKindType {
	GAMEPAD_KIND_UNKNOWN,
	GAMEPAD_KIND_XBOX,
	GAMEPAD_KIND_PLAYSTATION,
};

GamepadKindType Gamepad_Kind(void);
GamepadStateType Gamepad_Read(void);
bool Gamepad_Connected(void);

// dialog is the topmost open dialog, or nullptr when none is open.
void Gamepad_Pump(void * dialog);

// Call from the main loop between frames.
// The face button that makes and selects team 1 to 4 (0 to 3), or PAD_BUTTON_COUNT past them.
PadButtonType Gamepad_Team_Button(int team);
void Gamepad_Frame_Tick(void);

// Puts the pointer mid-map as a scenario starts under the controller scheme.
void Gamepad_Centre_Pointer(void);

// Call once before the first shell screen; the scheme no longer follows the pad after it.
void Gamepad_Settle_Auto_Scheme(unsigned wait_ms);

void Gamepad_Menu_Starts(bool on);
bool Gamepad_Menu_Starting(void);

bool Keyboard_Mouse_Seen(void);
void Note_Keyboard_Mouse_Use(void);
void Note_Keyboard_Mouse_Reset(void);
