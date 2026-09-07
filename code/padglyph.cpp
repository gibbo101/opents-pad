/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "padglyph.h"

#include "dsurface.h"
#include "gamepad.h"
#include "globals.h"
#include "goptions.h"
#include "padglyphdata.h"
#include "rect.h"
#include "rgb.h"
#include "surface.h"

#include <algorithm>

enum { SAMPLES = 4 };


// Steam marks a game running on a Steam Deck with this variable, and Proton passes it through.
static bool On_Steam_Deck(void)
{
	static int _answer = -1;
	if (_answer < 0) {
		char value[8] = "";
		GetEnvironmentVariableA("SteamDeck", value, sizeof(value));
		_answer = strcmp(value, "1") == 0;
	}
	return(_answer != 0);
}


int Resolved_Prompt_Style(void)
{
	if (Options.PromptStyle != PROMPT_STYLE_AUTO) {
		return(Options.PromptStyle);
	}
	if (On_Steam_Deck()) {
		return(PROMPT_STYLE_DECK);
	}
	return(Gamepad_Read().Connected ? PROMPT_STYLE_XBOX : PROMPT_STYLE_TEXT);
}


static int Style_Row(int style)
{
	switch (style) {
		case PROMPT_STYLE_XBOX: return(0);
		case PROMPT_STYLE_PLAYSTATION: return(1);
		case PROMPT_STYLE_DECK: return(2);
		default: return(-1);
	}
}


int Draw_Pad_Glyph(Surface & surface, PadButtonType button, int x, int y, int size)
{
	int row = Style_Row(Resolved_Prompt_Style());
	if (row < 0 || size < 4) {
		return(0);
	}
	unsigned char const * pixels = PadGlyphPixels[row][int(button) & 3];

	// Each drawn pixel averages a grid of source samples, then blends in by its coverage.
	float scale = float(PAD_GLYPH_SOURCE_SIZE) / float(size);
	for (int dy = 0; dy < size; dy++) {
		for (int dx = 0; dx < size; dx++) {
			int red = 0, green = 0, blue = 0, alpha = 0;
			for (int sy = 0; sy < SAMPLES; sy++) {
				for (int sx = 0; sx < SAMPLES; sx++) {
					int px = std::min(int((dx + (sx + 0.5f) / SAMPLES) * scale), int(PAD_GLYPH_SOURCE_SIZE) - 1);
					int py = std::min(int((dy + (sy + 0.5f) / SAMPLES) * scale), int(PAD_GLYPH_SOURCE_SIZE) - 1);
					unsigned char const * source = pixels + (py * PAD_GLYPH_SOURCE_SIZE + px) * 4;
					int a = source[3];
					red += source[0] * a;
					green += source[1] * a;
					blue += source[2] * a;
					alpha += a;
				}
			}
			if (alpha == 0) continue;
			RGBClass color(red / alpha, green / alpha, blue / alpha);
			int opacity = alpha * 100 / (255 * SAMPLES * SAMPLES);
			Rect pixel(x + dx, y + dy, 1, 1);
			if (opacity >= 100) {
				surface.Fill_Rect(pixel, DSurface::Build_Hicolor_Pixel(color));
			} else if (opacity > 0) {
				surface.Fill_Rect_Trans(pixel, color, opacity);
			}
		}
	}
	return(size);
}
