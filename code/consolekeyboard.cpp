/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "consolekeyboard.h"

#include "_keyboar.h"
#include "_surface.h"
#include "_xmouse.h"
#include "conquer.h"
#include "consolemenu.h"
#include "dsurface.h"
#include "gamepad.h"
#include "gametime.h"
#include "globals.h"
#include "gscreen.h"
#include "mschoice.h"
#include "msfont.h"
#include "msgloop.h"
#include "padglyph.h"
#include "rgb.h"
#include "surface.h"
#include "win.h"
#include "winstub.h"

#include <algorithm>
#include <cctype>
#include <vector>

extern PaletteClass CCPalette;

enum {
	MENU_WIDTH = 640,
	MENU_HEIGHT = 400,
	PANEL_INSET = 8,
	PANEL_OPACITY = 80,
	TITLE_Y = 12,
	FIELD_X = 120,
	FIELD_Y = 52,
	FIELD_WIDTH = 400,
	FIELD_HEIGHT = 30,
	FIELD_PAD = 8,
	GRID_X = 122,
	GRID_Y = 112,
	KEY_WIDTH = 36,
	KEY_HEIGHT = 28,
	KEY_GAP = 4,
	COLUMNS = 10,
	WIDE_WIDTH = 96,
	PROMPT_Y = 368,
	PROMPT_INSET = 24,
	GLYPH_INSET = 2,
	GLYPH_GAP = 6,
	PROMPT_GAP = 28,
	REPEAT_FIRST_MS = 350,
	REPEAT_NEXT_MS = 90,
	FRAME_NORMAL = 2,
	HIGHLIGHT_OPACITY = 35,
};

static char const * const _rows[] = {
	"1234567890",
	"ABCDEFGHIJ",
	"KLMNOPQRST",
	"UVWXYZ-_.'",
};
enum { LETTER_ROWS = 4, WIDE_ROW = LETTER_ROWS };
enum WideKeyType { WIDE_SPACE, WIDE_DELETE, WIDE_CAPS, WIDE_DONE, WIDE_COUNT };
static char const * const _wide[WIDE_COUNT] = {"Space", "Delete", "Caps", "Done"};

enum NavType { NAV_NONE, NAV_UP, NAV_DOWN, NAV_LEFT, NAV_RIGHT };


// Where a key sits within the 640x400 area.
static Rect Key_Rect(int row, int column)
{
	if (row == WIDE_ROW) {
		return(Rect(GRID_X + column * (WIDE_WIDTH + KEY_GAP), GRID_Y + row * (KEY_HEIGHT + KEY_GAP + 2), WIDE_WIDTH, KEY_HEIGHT));
	}
	return(Rect(GRID_X + column * (KEY_WIDTH + KEY_GAP), GRID_Y + row * (KEY_HEIGHT + KEY_GAP + 2), KEY_WIDTH, KEY_HEIGHT));
}


static int Columns_In(int row)
{
	return(row == WIDE_ROW ? int(WIDE_COUNT) : COLUMNS);
}


bool Console_Keyboard(char const * title, std::string & text, int max_length)
{
	std::string edit = text;
	if (max_length > 0 && int(edit.size()) > max_length) edit.resize(max_length);
	bool upper = edit.empty() || edit.back() == ' ';
	int row = 1;
	int column = 0;
	bool finished = false;
	bool accepted = false;
	bool dirty = true;
	NavType held = NAV_NONE;
	unsigned int repeat_at = 0;

	Keyboard->Clear();
	// Start finishes here, so the poll must not turn it into Escape while the keyboard is up.
	bool starts_were = Gamepad_Menu_Starting();
	Gamepad_Menu_Starts(true);
	GamepadStateType previous = Gamepad_Read();
	Point2D last_mouse(Get_Mouse_X(), Get_Mouse_Y());

	DSurface backdrop(HiddenSurface->Get_Width(), HiddenSurface->Get_Height());
	backdrop.Fill(0);
	Load_Title_Screen(Console_Backdrop_File(), &backdrop, &CCPalette);

	MSFont font(false);
	MSFont focus_font(false);
	focus_font.Set_Color(RGBClass(48, 224, 248));
	MSSfxEntry click("HighlightSound", (char *)"CHOICE1.AUD");
	int height = font.Get_Font_Height();

	auto key_label = [&](int r, int c) -> std::string {
		if (r == WIDE_ROW) return(_wide[c]);
		char letter = _rows[r][c];
		if (isalpha((unsigned char)letter)) letter = upper ? char(toupper((unsigned char)letter)) : char(tolower((unsigned char)letter));
		return(std::string(1, letter));
	};
	// Capitals come on at the start of each word and go off after the first letter, as a
	// phone keyboard does, unless the player has locked them with Caps.
	bool caps_lock = false;
	auto settle_case = [&](void) {
		if (!caps_lock) {
			upper = edit.empty() || edit.back() == ' ';
		}
	};
	auto add = [&](char letter) {
		if (max_length <= 0 || int(edit.size()) < max_length) {
			edit += letter;
			settle_case();
			dirty = true;
		}
	};
	auto erase = [&](void) {
		if (!edit.empty()) {
			edit.pop_back();
			settle_case();
			dirty = true;
		}
	};
	auto activate = [&](void) {
		if (row == WIDE_ROW) {
			switch (column) {
				case WIDE_SPACE: add(' '); break;
				case WIDE_DELETE: erase(); break;
				case WIDE_CAPS: caps_lock = !caps_lock; upper = caps_lock ? true : (edit.empty() || edit.back() == ' '); dirty = true; break;
				case WIDE_DONE: finished = true; accepted = true; break;
			}
		} else {
			add(key_label(row, column)[0]);
		}
	};
	auto move = [&](NavType nav) {
		int was_row = row;
		int was_column = column;
		switch (nav) {
			case NAV_UP: row = (row + LETTER_ROWS) % (LETTER_ROWS + 1); break;
			case NAV_DOWN: row = (row + 1) % (LETTER_ROWS + 1); break;
			case NAV_LEFT: column = (column + Columns_In(row) - 1) % Columns_In(row); break;
			case NAV_RIGHT: column = (column + 1) % Columns_In(row); break;
			default: break;
		}
		// Moving between the letter rows and the wide keys keeps the same place across the width.
		if (row != was_row && Columns_In(row) != Columns_In(was_row)) {
			column = std::clamp(column * Columns_In(row) / Columns_In(was_row), 0, Columns_In(row) - 1);
		}
		if (row != was_row || column != was_column) {
			click.Play();
			dirty = true;
		}
		held = nav;
		repeat_at = Get_Game_Time() + REPEAT_FIRST_MS;
	};

	while (!finished) {
		Call_Back();
		Windows_Message_Handler();
		if (!GameActive) break;

		Rect frame = HiddenSurface->Get_Rect();
		int left = (frame.Width - MENU_WIDTH) / 2;
		int top = (frame.Height - MENU_HEIGHT) / 2;

		if (GameInFocus) {
			GamepadStateType pad = Gamepad_Read();
			Point2D mouse(Get_Mouse_X(), Get_Mouse_Y());
			auto key_at = [&](Point2D const & point, int & r, int & c) -> bool {
				for (int rr = 0; rr <= LETTER_ROWS; rr++) {
					for (int cc = 0; cc < Columns_In(rr); cc++) {
						Rect area = Key_Rect(rr, cc);
						area.X += left;
						area.Y += top;
						if (area.Is_Point_Within(point)) { r = rr; c = cc; return(true); }
					}
				}
				return(false);
			};
			if (mouse != last_mouse) {
				last_mouse = mouse;
				int r, c;
				if (key_at(mouse, r, c) && (r != row || c != column)) {
					row = r;
					column = c;
					click.Play();
					dirty = true;
				}
			}

			while (Keyboard->Check() != KN_NONE) {
				KeyNumType raw = Keyboard->Get();
				KeyNumType key = KeyNumType(raw & ~(WWKEY_SHIFT_BIT|WWKEY_ALT_BIT|WWKEY_CTRL_BIT|WWKEY_VK_BIT));
				if (key == KN_LMOUSE) {
					int r, c;
					if (key_at(mouse, r, c)) {
						row = r;
						column = c;
						activate();
					}
					continue;
				}
				switch (key) {
					case KN_UP: move(NAV_UP); break;
					case KN_DOWN: move(NAV_DOWN); break;
					case KN_LEFT: move(NAV_LEFT); break;
					case KN_RIGHT: move(NAV_RIGHT); break;
					case KN_RETURN: finished = true; accepted = true; break;
					case KN_ESC: finished = true; break;
					case KN_BACKSPACE: erase(); break;
					default: {
						char ascii = Keyboard->To_ASCII(raw);
						if (ascii >= ' ' && ascii < 127) add(ascii);
						break;
					}
				}
			}

			if (pad.Up && !previous.Up) move(NAV_UP);
			if (pad.Down && !previous.Down) move(NAV_DOWN);
			if (pad.Left && !previous.Left) move(NAV_LEFT);
			if (pad.Right && !previous.Right) move(NAV_RIGHT);
			if (pad.Accept && !previous.Accept) activate();
			if (pad.Back && !previous.Back) finished = true;
			if (pad.Menu && !previous.Menu) { finished = true; accepted = true; }
			if (pad.Third && !previous.Third) erase();
			if (pad.Fourth && !previous.Fourth) add(' ');
			previous = pad;

			auto still_held = [&](NavType nav) {
				switch (nav) {
					case NAV_UP: return(Keyboard->Down(KN_UP) != 0 || pad.Up);
					case NAV_DOWN: return(Keyboard->Down(KN_DOWN) != 0 || pad.Down);
					case NAV_LEFT: return(Keyboard->Down(KN_LEFT) != 0 || pad.Left);
					case NAV_RIGHT: return(Keyboard->Down(KN_RIGHT) != 0 || pad.Right);
					default: return(false);
				}
			};
			if (held != NAV_NONE) {
				if (!still_held(held)) {
					held = NAV_NONE;
				} else if (Get_Game_Time() >= repeat_at) {
					NavType nav = held;
					move(nav);
					repeat_at = Get_Game_Time() + REPEAT_NEXT_MS;
				}
			}
		}

		if (dirty) {
			Hide_Mouse();
			Surface & surface = *HiddenSurface;
			surface.Blit_From(backdrop);
			surface.Fill_Rect_Trans(Rect(left + PANEL_INSET, top + PANEL_INSET, MENU_WIDTH - 2 * PANEL_INSET, MENU_HEIGHT - 2 * PANEL_INSET), RGBClass(0, 0, 0), PANEL_OPACITY);
			auto print = [&](std::string const & what, int x, int y, bool focused) {
				(focused ? focus_font : font).Draw_String(&surface, (unsigned char const *)what.c_str(), x, y, FRAME_NORMAL);
			};
			auto width = [&](std::string const & what) { return(font.Get_String_Width(what.c_str())); };

			std::string heading = title != NULL ? title : "";
			print(heading, left + (MENU_WIDTH - width(heading)) / 2, top + TITLE_Y, false);

			Rect field(left + FIELD_X, top + FIELD_Y, FIELD_WIDTH, FIELD_HEIGHT);
			surface.Fill_Rect_Trans(field, RGBClass(0, 0, 0), 60);
			surface.Draw_Rect(field, DSurface::Build_Hicolor_Pixel(RGBClass(96, 208, 248)));
			std::string shown = edit + "_";
			while (shown.size() > 1 && width(shown) > FIELD_WIDTH - 2 * FIELD_PAD) shown.erase(0, 1);
			print(shown, field.X + FIELD_PAD, field.Y + (FIELD_HEIGHT - height) / 2, false);

			for (int r = 0; r <= LETTER_ROWS; r++) {
				for (int c = 0; c < Columns_In(r); c++) {
					Rect area = Key_Rect(r, c);
					area.X += left;
					area.Y += top;
					bool focused = r == row && c == column;
					if (focused) {
						surface.Fill_Rect_Trans(area, RGBClass(48, 224, 248), HIGHLIGHT_OPACITY);
					}
					std::string label = key_label(r, c);
					print(label, area.X + (area.Width - width(label)) / 2, area.Y + (area.Height - height) / 2, focused);
				}
			}

			// Back and Delete at the left, Done, Space and Select at the right.
			int glyph = height + 2 * GLYPH_INSET;
			bool glyphs = Resolved_Prompt_Style() != PROMPT_STYLE_TEXT;
			auto prompt = [&](std::string const & label, PadButtonType button, int x, bool at_right) -> int {
				int used = glyphs ? glyph + GLYPH_GAP : 0;
				int total = used + width(label);
				int start = at_right ? x - total : x;
				if (glyphs) Draw_Pad_Glyph(surface, button, start, top + PROMPT_Y - GLYPH_INSET, glyph);
				print(label, start + used, top + PROMPT_Y, false);
				return(total);
			};
			int x = left + PROMPT_INSET;
			x += prompt("Back", PAD_BUTTON_BACK, x, false) + PROMPT_GAP;
			prompt("Delete", PAD_BUTTON_THIRD, x, false);
			x = left + MENU_WIDTH - PROMPT_INSET;
			x -= prompt("Done", PAD_BUTTON_MENU, x, true) + PROMPT_GAP;
			x -= prompt("Select", PAD_BUTTON_ACCEPT, x, true) + PROMPT_GAP;
			prompt("Space", PAD_BUTTON_FOURTH, x, true);

			Update_Visible_Surface(&surface);
			Show_Mouse();
			dirty = false;
		}
		Sleep(1);
	}

	Keyboard->Clear();
	Gamepad_Menu_Starts(starts_were);
	if (accepted) {
		text = edit;
	}
	return(accepted);
}
