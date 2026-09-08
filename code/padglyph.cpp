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
bool On_Steam_Deck(void)
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
	if (Gamepad_Kind() == GAMEPAD_KIND_PLAYSTATION) {
		return(PROMPT_STYLE_PLAYSTATION);
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
	Draw_Baked_Image(surface, PadGlyphPixels[row][std::clamp(int(button), 0, int(PAD_BUTTON_COUNT) - 1)], PAD_GLYPH_SOURCE_SIZE, x, y, size, 100);
	return(size);
}


int Draw_Pad_Glyph_Fitted(Surface & surface, PadButtonType button, int x, int y, int size)
{
	int row = Style_Row(Resolved_Prompt_Style());
	if (row < 0 || size < 4) {
		return(0);
	}
	unsigned char const * pixels = PadGlyphPixels[row][std::clamp(int(button), 0, int(PAD_BUTTON_COUNT) - 1)];

	// The content's bounds, from the samples that carry any alpha.
	int left = PAD_GLYPH_SOURCE_SIZE, top = PAD_GLYPH_SOURCE_SIZE, right = -1, bottom = -1;
	for (int py = 0; py < PAD_GLYPH_SOURCE_SIZE; py++) {
		for (int px = 0; px < PAD_GLYPH_SOURCE_SIZE; px++) {
			if (pixels[(py * PAD_GLYPH_SOURCE_SIZE + px) * 4 + 3] > 8) {
				left = std::min(left, px);
				top = std::min(top, py);
				right = std::max(right, px);
				bottom = std::max(bottom, py);
			}
		}
	}
	if (right < left) {
		return(0);
	}
	int width = right - left + 1;
	int height = bottom - top + 1;
	int span = std::max(width, height);
	int drawn_width = std::max(size * width / span, 1);
	int drawn_height = std::max(size * height / span, 1);
	Draw_Baked_Image_Part(surface, pixels, PAD_GLYPH_SOURCE_SIZE, left, top, width, height,
		x + (size - drawn_width) / 2, y + (size - drawn_height) / 2, drawn_width, drawn_height, 100);
	return(size);
}


void Draw_Baked_Image(Surface & surface, unsigned char const * pixels, int source, int x, int y, int size, int opacity)
{
	Draw_Baked_Image_Part(surface, pixels, source, 0, 0, source, source, x, y, size, size, opacity);
}


void Draw_Baked_Image_Part(Surface & surface, unsigned char const * pixels, int source, int sourcex, int sourcey, int sourcewidth, int sourceheight, int x, int y, int width, int height, int opacity)
{
	if (width < 1 || height < 1 || sourcewidth < 1 || sourceheight < 1 || opacity < 1) {
		return;
	}

	// Each drawn pixel averages a grid of source samples, then blends in by its coverage.
	float scalex = float(sourcewidth) / float(width);
	float scaley = float(sourceheight) / float(height);
	for (int dy = 0; dy < height; dy++) {
		for (int dx = 0; dx < width; dx++) {
			int red = 0, green = 0, blue = 0, alpha = 0;
			for (int sy = 0; sy < SAMPLES; sy++) {
				for (int sx = 0; sx < SAMPLES; sx++) {
					int px = std::min(sourcex + int((dx + (sx + 0.5f) / SAMPLES) * scalex), sourcex + sourcewidth - 1);
					int py = std::min(sourcey + int((dy + (sy + 0.5f) / SAMPLES) * scaley), sourcey + sourceheight - 1);
					unsigned char const * sample = pixels + (py * source + px) * 4;
					int a = sample[3];
					red += sample[0] * a;
					green += sample[1] * a;
					blue += sample[2] * a;
					alpha += a;
				}
			}
			if (alpha == 0) continue;
			RGBClass color(red / alpha, green / alpha, blue / alpha);
			int coverage = alpha * opacity / (255 * SAMPLES * SAMPLES);
			Rect pixel(x + dx, y + dy, 1, 1);
			if (coverage >= 100) {
				surface.Fill_Rect(pixel, DSurface::Build_Hicolor_Pixel(color));
			} else if (coverage > 0) {
				surface.Fill_Rect_Trans(pixel, color, coverage);
			}
		}
	}
}
