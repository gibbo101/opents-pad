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
bool On_Steam_Deck(void);

/// <summary>
/// Draws the glyph for a button in the current style within a box of the given size at x, y
/// and returns the width used, or 0 when the style is plain text and nothing was drawn.
/// </summary>
int Draw_Pad_Glyph(Surface & surface, PadButtonType button, int x, int y, int size);

/// <summary>
/// Draws a button's glyph with its drawn content, rather than its whole tile, fitted to a
/// box of the given size at x, y, so glyphs whose art carries different margins come out
/// the same size. Returns the width used, or 0 when the style is plain text.
/// </summary>
int Draw_Pad_Glyph_Fitted(Surface & surface, PadButtonType button, int x, int y, int size);

/// <summary>
/// Draws a baked square RGBA image, source pixels on a side, into a box of the given size at
/// x, y, averaging source samples per drawn pixel and blending by coverage scaled by opacity
/// in percent.
/// </summary>
void Draw_Baked_Image(Surface & surface, unsigned char const * pixels, int source, int x, int y, int size, int opacity);

/// <summary>
/// Draws a part of a baked square RGBA image, the source rectangle given in source pixels,
/// into a box of the given width and height at x, y, the same way.
/// </summary>
void Draw_Baked_Image_Part(Surface & surface, unsigned char const * pixels, int source, int sourcex, int sourcey, int sourcewidth, int sourceheight, int x, int y, int width, int height, int opacity);
