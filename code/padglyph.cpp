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


static int Pixel(RGBClass const & color)
{
	return(DSurface::Build_Hicolor_Pixel(color));
}


enum { SAMPLES = 4 };

// Paints a shape with edge coverage from a sample grid per pixel, blending the colour in
// by coverage, so a small glyph keeps round edges at the shell's 640x400 size.
template<typename InsideType>
static void Paint(Surface & surface, Rect const & box, RGBClass const & color, InsideType inside)
{
	for (int y = box.Y; y < box.Y + box.Height; y++) {
		for (int x = box.X; x < box.X + box.Width; x++) {
			int hits = 0;
			for (int sy = 0; sy < SAMPLES; sy++) {
				for (int sx = 0; sx < SAMPLES; sx++) {
					if (inside(x + (sx + 0.5f) / SAMPLES, y + (sy + 0.5f) / SAMPLES)) hits++;
				}
			}
			if (hits == 0) continue;
			int opacity = hits * 100 / (SAMPLES * SAMPLES);
			if (opacity >= 100) {
				surface.Fill_Rect(Rect(x, y, 1, 1), Pixel(color));
			} else {
				surface.Fill_Rect_Trans(Rect(x, y, 1, 1), color, opacity);
			}
		}
	}
}


static float Distance(float x, float y, float cx, float cy)
{
	return(std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy)));
}


static float Segment_Distance(float x, float y, float ax, float ay, float bx, float by)
{
	float dx = bx - ax;
	float dy = by - ay;
	float length = dx * dx + dy * dy;
	float t = length > 0.0f ? std::clamp(((x - ax) * dx + (y - ay) * dy) / length, 0.0f, 1.0f) : 0.0f;
	return(Distance(x, y, ax + t * dx, ay + t * dy));
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


static void Draw_Shape(Surface & surface, PadButtonType button, Rect const & box, float cx, float cy, float radius, RGBClass const & color)
{
	float s = std::max(radius * 0.55f, 2.0f);
	float half = SHAPE_WIDTH * 0.5f;
	switch (button) {
		case PAD_BUTTON_ACCEPT:
			Paint(surface, box, color, [&](float x, float y) {
				return(Segment_Distance(x, y, cx - s, cy - s, cx + s, cy + s) <= half || Segment_Distance(x, y, cx - s, cy + s, cx + s, cy - s) <= half);
			});
			break;
		case PAD_BUTTON_BACK:
			Paint(surface, box, color, [&](float x, float y) {
				float d = Distance(x, y, cx, cy);
				return(d <= s + half && d > s - half);
			});
			break;
		case PAD_BUTTON_THIRD:
			Paint(surface, box, color, [&](float x, float y) {
				float d = std::max(std::fabs(x - cx), std::fabs(y - cy));
				return(d <= s + half && d > s - half);
			});
			break;
		case PAD_BUTTON_FOURTH: {
			float top = cy - s - 1.0f;
			float base = cy + s * 0.8f;
			float wide = s * 1.15f;
			Paint(surface, box, color, [&](float x, float y) {
				return(Segment_Distance(x, y, cx, top, cx - wide, base) <= half
					|| Segment_Distance(x, y, cx, top, cx + wide, base) <= half
					|| Segment_Distance(x, y, cx - wide, base, cx + wide, base) <= half);
			});
			break;
		}
	}
}


int Draw_Pad_Glyph(Surface & surface, PadButtonType button, int x, int y, int size)
{
	int style = Resolved_Prompt_Style();
	if (style == PROMPT_STYLE_TEXT || size < 6) {
		return(0);
	}
	Rect box(x, y, size, size);
	float radius = size * 0.5f - 0.5f;
	float cx = x + size * 0.5f;
	float cy = y + size * 0.5f;
	int index = int(button) & 3;

	RGBClass const & ring = style == PROMPT_STYLE_XBOX ? _xbox[index] : _pale;
	Paint(surface, box, ring, [&](float px, float py) { return(Distance(px, py, cx, cy) <= radius); });
	Paint(surface, box, _dark, [&](float px, float py) { return(Distance(px, py, cx, cy) <= radius - RING_WIDTH); });

	switch (style) {
		case PROMPT_STYLE_XBOX:
			Draw_Letter(surface, _letters[index], int(cx), int(cy), _xbox[index]);
			break;
		case PROMPT_STYLE_DECK:
			Draw_Letter(surface, _letters[index], int(cx), int(cy), _white);
			break;
		case PROMPT_STYLE_PLAYSTATION:
			Draw_Shape(surface, button, box, cx, cy, radius - RING_WIDTH, _playstation[index]);
			break;
	}
	return(size);
}
