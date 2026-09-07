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
#include "msfont.h"
#include "point.h"
#include "rect.h"
#include "rgb.h"
#include "surface.h"

#include <algorithm>
#include <cmath>

enum {
	RING_WIDTH = 2,
	SHAPE_WIDTH = 2,
};

static RGBClass const _dark(24, 24, 28);
static RGBClass const _pale(190, 190, 200);
static RGBClass const _white(255, 255, 255);
static RGBClass const _xbox[4] = {RGBClass(60, 200, 80), RGBClass(230, 60, 60), RGBClass(60, 120, 240), RGBClass(240, 200, 50)};
static RGBClass const _playstation[4] = {RGBClass(110, 160, 240), RGBClass(240, 80, 80), RGBClass(230, 110, 200), RGBClass(90, 210, 120)};
static char const _letters[4] = {'A', 'B', 'X', 'Y'};


int Resolved_Prompt_Style(void)
{
	if (Options.PromptStyle != PROMPT_STYLE_AUTO) {
		return(Options.PromptStyle);
	}
	return(Gamepad_Read().Connected ? PROMPT_STYLE_XBOX : PROMPT_STYLE_TEXT);
}


static int Pixel(RGBClass const & color)
{
	return(DSurface::Build_Hicolor_Pixel(color));
}


static void Fill_Disc(Surface & surface, int cx, int cy, int radius, int color)
{
	for (int dy = -radius; dy <= radius; dy++) {
		int half = int(std::sqrt(float(radius * radius - dy * dy)) + 0.5f);
		surface.Fill_Rect(Rect(cx - half, cy + dy, 2 * half + 1, 1), color);
	}
}


static void Thick_Line(Surface & surface, Point2D from, Point2D to, int color)
{
	surface.Draw_Line(from, to, color);
	surface.Draw_Line(from + Point2D(1, 0), to + Point2D(1, 0), color);
	surface.Draw_Line(from + Point2D(0, 1), to + Point2D(0, 1), color);
}


// The letters are the menu font recoloured, one font per colour, kept for the life of the game.
static MSFont * Letter_Font(RGBClass const & color)
{
	struct EntryType {
		unsigned Key;
		MSFont * Font;
	};
	static EntryType _fonts[8];
	static int _count = 0;

	unsigned key = (unsigned(color.Get_Red()) << 16) | (unsigned(color.Get_Green()) << 8) | unsigned(color.Get_Blue());
	for (int index = 0; index < _count; index++) {
		if (_fonts[index].Key == key) return(_fonts[index].Font);
	}
	if (_count >= int(sizeof(_fonts) / sizeof(_fonts[0]))) return(NULL);
	MSFont * font = new MSFont(false);
	font->Set_Color(color);
	_fonts[_count++] = {key, font};
	return(font);
}


static void Draw_Letter(Surface & surface, char letter, int cx, int cy, RGBClass const & color)
{
	MSFont * font = Letter_Font(color);
	if (font == NULL) return;
	unsigned char text[2] = {(unsigned char)letter, 0};
	int width = font->Get_String_Width((char const *)text);
	font->Draw_String(&surface, text, cx - width / 2, cy - font->Get_Font_Height() / 2, 2);
}


static void Draw_Shape(Surface & surface, PadButtonType button, int cx, int cy, int radius, RGBClass const & color)
{
	int s = std::max(radius / 2, 2);
	int pixel = Pixel(color);
	switch (button) {
		case PAD_BUTTON_ACCEPT:
			Thick_Line(surface, Point2D(cx - s, cy - s), Point2D(cx + s, cy + s), pixel);
			Thick_Line(surface, Point2D(cx - s, cy + s), Point2D(cx + s, cy - s), pixel);
			break;
		case PAD_BUTTON_BACK:
			Fill_Disc(surface, cx, cy, s + 1, pixel);
			Fill_Disc(surface, cx, cy, s + 1 - SHAPE_WIDTH, Pixel(_dark));
			break;
		case PAD_BUTTON_THIRD:
			surface.Draw_Rect(Rect(cx - s, cy - s, 2 * s + 1, 2 * s + 1), pixel);
			surface.Draw_Rect(Rect(cx - s + 1, cy - s + 1, 2 * s - 1, 2 * s - 1), pixel);
			break;
		case PAD_BUTTON_FOURTH:
			Thick_Line(surface, Point2D(cx, cy - s - 1), Point2D(cx - s - 1, cy + s), pixel);
			Thick_Line(surface, Point2D(cx, cy - s - 1), Point2D(cx + s + 1, cy + s), pixel);
			Thick_Line(surface, Point2D(cx - s - 1, cy + s), Point2D(cx + s + 1, cy + s), pixel);
			break;
	}
}


int Draw_Pad_Glyph(Surface & surface, PadButtonType button, int x, int y, int size)
{
	int style = Resolved_Prompt_Style();
	if (style == PROMPT_STYLE_TEXT || size < 6) {
		return(0);
	}
	int radius = size / 2 - 1;
	int cx = x + size / 2;
	int cy = y + size / 2;
	int index = int(button) & 3;

	RGBClass const & ring = style == PROMPT_STYLE_XBOX ? _xbox[index] : _pale;
	Fill_Disc(surface, cx, cy, radius, Pixel(ring));
	Fill_Disc(surface, cx, cy, radius - RING_WIDTH, Pixel(_dark));

	switch (style) {
		case PROMPT_STYLE_XBOX:
			Draw_Letter(surface, _letters[index], cx, cy, _xbox[index]);
			break;
		case PROMPT_STYLE_DECK:
			Draw_Letter(surface, _letters[index], cx, cy, _white);
			break;
		case PROMPT_STYLE_PLAYSTATION:
			Draw_Shape(surface, button, cx, cy, radius - RING_WIDTH, _playstation[index]);
			break;
	}
	return(size);
}
