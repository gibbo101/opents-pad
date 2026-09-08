/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

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

/// <summary>
/// Reports the make of the connected controller, or unknown when none is listed.
/// </summary>
GamepadKindType Gamepad_Kind(void);

/// <summary>
/// Reads the first connected controller. Returns a disconnected state when no controller
/// or no controller support is present.
/// </summary>
GamepadStateType Gamepad_Read(void);

/// <summary>
/// Whether a controller is connected, from the latest read; rereads at most once a second.
/// </summary>
bool Gamepad_Connected(void);

/// <summary>
/// Samples the controller for the message pump: the menu button presses Escape into the
/// keyboard buffer, an open dialog gets the d-pad, accept, and back as key messages, and
/// play input is kept for Gamepad_Frame_Tick.
/// </summary>
/// <param name="dialog">The topmost open dialog, or NULL when none is open.</param>
void Gamepad_Pump(void * dialog);

/// <summary>
/// Applies the pad's play input since the last frame, then a zoom step it asked for and the
/// sidebar panel's slide with the tab bar's redraw. Call from the main loop between frames.
/// </summary>
void Gamepad_Frame_Tick(void);

// Puts the pointer mid-map as a scenario starts under the controller scheme.
void Gamepad_Centre_Pointer(void);

/// <summary>
/// Fixes an Auto control scheme before the first shell screen: a pad that has appeared since
/// launch selects the controller scheme. With none yet the call waits up to the given time
/// only while the system lists a controller. After this the scheme no longer follows the pad.
/// </summary>
void Gamepad_Settle_Auto_Scheme(unsigned wait_ms);

/// <summary>
/// While on, the menu button is left to the screen that asked instead of pressing Escape,
/// for a screen whose accept is Start.
/// </summary>
void Gamepad_Menu_Starts(bool on);
bool Gamepad_Menu_Starting(void);

/// <summary>
/// Has a real keyboard key or mouse button been pressed on the game window since launch?
/// Steam presents a mouse and keyboard whether or not the player has one, so use is the
/// only evidence. Input the pad sent through the system is not counted.
/// </summary>
bool Keyboard_Mouse_Seen(void);
void Note_Keyboard_Mouse_Use(void);
void Note_Keyboard_Mouse_Reset(void);
