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
	bool Fast;
	bool Menu;
};

/// <summary>
/// Reads the first connected controller. Returns a disconnected state when no controller
/// or no controller support is present.
/// </summary>
GamepadStateType Gamepad_Read(void);

/// <summary>
/// Feeds the keys the controller stands in for: the menu button presses Escape into the
/// keyboard buffer, and while a dialog is open the d-pad, accept, and back go to it as
/// arrow, Enter, and Escape key messages. Call it from the message pump.
/// </summary>
/// <param name="dialog">The topmost open dialog, or NULL when none is open.</param>
void Gamepad_Pump(void * dialog);

/// <summary>
/// Has a real keyboard key or mouse button been pressed on the game window since launch?
/// Steam presents a mouse and keyboard whether or not the player has one, so use is the
/// only evidence.
/// </summary>
bool Keyboard_Mouse_Seen(void);
void Note_Keyboard_Mouse_Use(void);
void Note_Keyboard_Mouse_Reset(void);
