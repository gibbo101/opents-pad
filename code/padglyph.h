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

int Resolved_Prompt_Style(void);
bool On_Steam_Deck(void);

int Draw_Pad_Glyph(Surface & surface, PadButtonType button, int x, int y, int size);
int Draw_Pad_Glyph_Fitted(Surface & surface, PadButtonType button, int x, int y, int size);

// pixels is a square RGBA image, source pixels on a side; opacity is in percent.
void Draw_Baked_Image(Surface & surface, unsigned char const * pixels, int source, int x, int y, int size, int opacity);
void Draw_Baked_Image_Part(Surface & surface, unsigned char const * pixels, int source, int sourcex, int sourcey, int sourcewidth, int sourceheight, int x, int y, int width, int height, int opacity);
