/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "options.h"

class Surface;

// The face buttons a prompt can name, by what they do rather than by any one pad's labels.
enum PadButtonType {
	PAD_BUTTON_ACCEPT,		// A, or cross.
	PAD_BUTTON_BACK,		// B, or circle.
	PAD_BUTTON_THIRD,		// X, or square.
	PAD_BUTTON_FOURTH,		// Y, or triangle.
	PAD_BUTTON_MENU,		// Menu, Options, or Start.
	PAD_BUTTON_COUNT,
};

/// <summary>
/// The style prompts draw in right now. Auto resolves to the Deck set on a Steam Deck, to
/// the PlayStation set when the system lists a Sony controller, to the Xbox set while a
/// controller is connected, and to plain text otherwise.
/// </summary>
int Resolved_Prompt_Style(void);

/// <summary>
/// Draws the glyph for a button in the current style within a box of the given size at x, y
/// and returns the width used, or 0 when the style is plain text and nothing was drawn.
/// </summary>
int Draw_Pad_Glyph(Surface & surface, PadButtonType button, int x, int y, int size);
