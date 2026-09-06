/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "consolemenu.h"

#include "_keyboar.h"
#include "_surface.h"
#include "_xmouse.h"
#include "conquer.h"
#include "dsurface.h"
#include "gamepad.h"
#include "gametime.h"
#include "globals.h"
#include "gscreen.h"
#include "msfont.h"
#include "msgloop.h"
#include "newmenu.h"
#include "rgb.h"
#include "srfcache.h"
#include "surface.h"
#include "win.h"
#include "winstub.h"

#include <algorithm>

extern PaletteClass CCPalette;

enum {
	MENU_WIDTH = 640,
	MENU_HEIGHT = 400,
	PANEL_INSET = 8,
	TITLE_Y = 12,
	ROWS_TOP = 44,
	ROWS_BOTTOM = 356,
	LABEL_RIGHT = 316,
	VALUE_LEFT = 336,
	VALUE_RIGHT = 616,
	SIDE_X = 24,
	SIDE_Y = 44,
	SIDE_WIDTH = 176,
	SIDE_HEIGHT = 176,
	PROMPT_Y = 368,
	PROMPT_INSET = 24,
	SWATCH_GAP = 10,
	SWATCH_INSET = 2,
	PANEL_OPACITY = 80,
	REPEAT_FIRST_MS = 350,
	REPEAT_NEXT_MS = 90,
	FAST_STEP = 5,
	FRAME_NORMAL = 2,
};


ConsoleMenuClass::ConsoleMenuClass(char const * title) :
	Title(title != NULL ? title : ""),
	AcceptPrompt("Accept"),
	BackPrompt("Back"),
	Font(NULL),
	FocusFont(NULL),
	Backdrop(NULL),
	Focus(0),
	IsDirty(true)
{
}


ConsoleMenuClass::~ConsoleMenuClass(void)
{
	delete Font;
	delete FocusFont;
	delete Backdrop;
}




int ConsoleMenuClass::Add_Row(ConsoleRowType const & row)
{
	Rows.push_back(row);
	return(int(Rows.size()) - 1);
}


void ConsoleMenuClass::Set_Prompts(char const * accept, char const * back)
{
	AcceptPrompt = accept != NULL ? accept : "";
	BackPrompt = back != NULL ? back : "";
	IsDirty = true;
}


void ConsoleMenuClass::Set_Side_Panel(std::function<void(Surface &, Rect const &)> draw)
{
	SidePanel = draw;
	IsDirty = true;
}


void ConsoleMenuClass::Move_Focus(int step)
{
	if (Rows.empty()) return;
	Focus = (Focus + step + int(Rows.size())) % int(Rows.size());
	IsDirty = true;
}


void ConsoleMenuClass::Step_Value(int step)
{
	if (Focus < 0 || Focus >= int(Rows.size())) return;
	ConsoleRowType & row = Rows[Focus];
	if (row.Step) {
		row.Step(step);
		IsDirty = true;
	}
}


enum NavType {
	NAV_NONE,
	NAV_UP,
	NAV_DOWN,
	NAV_LEFT,
	NAV_RIGHT,
};


// Turns key presses and controller input into menu movement. A held direction repeats,
// since the engine drops the system's key repeat. Returns true when the menu is finished.
bool ConsoleMenuClass::Poll_Input(ConsoleMenuResult & result)
{
	static unsigned int _repeat_at = 0;
	static NavType _held = NAV_NONE;
	static GamepadStateType _previous = {};

	GamepadStateType pad = Gamepad_Read();

	// A held shift or shoulder button steps values five at a time.
	int stride = (Keyboard->Down(KN_LSHIFT) || pad.Fast) ? FAST_STEP : 1;

	auto navigate = [&](NavType nav) {
		switch (nav) {
			case NAV_UP: Move_Focus(-1); break;
			case NAV_DOWN: Move_Focus(1); break;
			case NAV_LEFT: Step_Value(-stride); break;
			case NAV_RIGHT: Step_Value(stride); break;
			default: break;
		}
		_held = nav;
		_repeat_at = Get_Game_Time() + REPEAT_FIRST_MS;
	};
	auto accept = [&](void) -> bool {
		if (Focus >= 0 && Focus < int(Rows.size()) && Rows[Focus].Activate) {
			Rows[Focus].Activate();
			IsDirty = true;
			return(false);
		}
		result = CONSOLE_MENU_ACCEPT;
		return(true);
	};

	while (Keyboard->Check() != KN_NONE) {
		KeyNumType key = KeyNumType(Keyboard->Get() & ~(WWKEY_SHIFT_BIT|WWKEY_ALT_BIT|WWKEY_CTRL_BIT|WWKEY_VK_BIT));
		switch (key) {
			case KN_UP: navigate(NAV_UP); break;
			case KN_DOWN: navigate(NAV_DOWN); break;
			case KN_LEFT: navigate(NAV_LEFT); break;
			case KN_RIGHT: navigate(NAV_RIGHT); break;
			case KN_RETURN:
			case KN_SPACE:
				if (accept()) return(true);
				break;
			case KN_ESC:
				result = CONSOLE_MENU_BACK;
				return(true);
			default:
				break;
		}
	}

	if (pad.Up && !_previous.Up) navigate(NAV_UP);
	if (pad.Down && !_previous.Down) navigate(NAV_DOWN);
	if (pad.Left && !_previous.Left) navigate(NAV_LEFT);
	if (pad.Right && !_previous.Right) navigate(NAV_RIGHT);
	bool accept_pressed = pad.Accept && !_previous.Accept;
	bool back_pressed = pad.Back && !_previous.Back;
	_previous = pad;
	if (accept_pressed && accept()) return(true);
	if (back_pressed) {
		result = CONSOLE_MENU_BACK;
		return(true);
	}

	auto still_held = [&](NavType nav) {
		switch (nav) {
			case NAV_UP: return(Keyboard->Down(KN_UP) != 0 || pad.Up);
			case NAV_DOWN: return(Keyboard->Down(KN_DOWN) != 0 || pad.Down);
			case NAV_LEFT: return(Keyboard->Down(KN_LEFT) != 0 || pad.Left);
			case NAV_RIGHT: return(Keyboard->Down(KN_RIGHT) != 0 || pad.Right);
			default: return(false);
		}
	};
	if (_held != NAV_NONE) {
		if (!still_held(_held)) {
			_held = NAV_NONE;
		} else if (Get_Game_Time() >= _repeat_at) {
			NavType nav = _held;
			navigate(nav);
			_repeat_at = Get_Game_Time() + REPEAT_NEXT_MS;
		}
	}
	return(false);
}


void ConsoleMenuClass::Draw(void)
{
	Surface & surface = *HiddenSurface;
	Rect frame = surface.Get_Rect();
	int left = (frame.Width - MENU_WIDTH) / 2;
	int top = (frame.Height - MENU_HEIGHT) / 2;

	surface.Blit_From(*Backdrop);
	surface.Fill_Rect_Trans(Rect(left + PANEL_INSET, top + PANEL_INSET, MENU_WIDTH - 2 * PANEL_INSET, MENU_HEIGHT - 2 * PANEL_INSET), RGBClass(0, 0, 0), PANEL_OPACITY);

	if (Font == NULL) {
		Font = new MSFont(false);
		FocusFont = new MSFont(false);
		FocusFont->Set_Color(RGBClass(48, 224, 248));
	}
	int height = Font->Get_Font_Height();
	auto print = [&](std::string const & text, int x, int y, bool focused = false) {
		(focused ? FocusFont : Font)->Draw_String(&surface, (unsigned char const *)text.c_str(), x, y, FRAME_NORMAL);
	};
	auto width = [&](std::string const & text) {
		return(Font->Get_String_Width(text.c_str()));
	};

	print(Title, left + (MENU_WIDTH - width(Title)) / 2, top + TITLE_Y);

	if (SidePanel) {
		SidePanel(surface, Rect(left + SIDE_X, top + SIDE_Y, SIDE_WIDTH, SIDE_HEIGHT));
	}

	int count = int(Rows.size());
	int pitch = height + 4;
	if (count > 0) {
		pitch = std::min(pitch, (ROWS_BOTTOM - ROWS_TOP) / count);
	}
	int y = top + ROWS_TOP;
	for (int index = 0; index < count; index++) {
		ConsoleRowType const & row = Rows[index];
		bool focused = index == Focus;
		print(row.Label, left + LABEL_RIGHT - width(row.Label), y, focused);
		if (row.Value) {
			std::string value = row.Value();
			// A value that would run past the box is cut short with a trailing "..".
			int limit = VALUE_RIGHT - VALUE_LEFT;
			if (width(value) > limit) {
				while (value.size() > 1 && width(value + "..") > limit) {
					value.pop_back();
				}
				value += "..";
			}
			print(value, left + VALUE_LEFT, y, focused);
			int x = left + VALUE_LEFT + (value.empty() ? 0 : width(value) + SWATCH_GAP);
			if (row.Icon) {
				Surface * icon = row.Icon();
				if (icon != NULL) {
					Rect source = icon->Get_Rect();
					int iy = y + (height - source.Height) / 2;
					// The cached icons carry a magenta key around their artwork.
					SurfaceCache.DrawTrans(Rect(x, iy, source.Width, source.Height), surface, *icon, (255 >> DSurface::RedLeft << DSurface::RedRight) | (255u >> DSurface::BlueLeft << DSurface::BlueRight));
					x += source.Width + SWATCH_GAP;
				}
			}
			if (row.Swatches) {
				std::vector<int> colors = row.Swatches();
				int selected = row.Selected ? row.Selected() : -1;
				int big = height - 2 * SWATCH_INSET;
				int small = big / 2;
				for (int index = 0; index < int(colors.size()); index++) {
					int size = index == selected ? big : small;
					surface.Fill_Rect(Rect(x, y + SWATCH_INSET + (big - size) / 2, size, size), colors[index]);
					x += big + SWATCH_GAP;
				}
			}
		}
		y += pitch;
	}

	print(BackPrompt, left + PROMPT_INSET, top + PROMPT_Y);
	print(AcceptPrompt, left + MENU_WIDTH - PROMPT_INSET - width(AcceptPrompt), top + PROMPT_Y);

	Update_Visible_Surface(&surface);
	IsDirty = false;
}


ConsoleMenuResult ConsoleMenuClass::Process(void)
{
	ConsoleMenuResult result = CONSOLE_MENU_BACK;

	Keyboard->Clear();
	Hide_Mouse();

	// The map preview loader scribbles on AlternateSurface, so the backdrop is kept on a surface of its own.
	if (Backdrop == NULL) {
		Backdrop = new DSurface(HiddenSurface->Get_Width(), HiddenSurface->Get_Height());
	}
	Backdrop->Fill(0);
	Load_Title_Screen(Get_New_Menu()->Background, Backdrop, &CCPalette);
	IsDirty = true;

	while (true) {
		Call_Back();
		Windows_Message_Handler();
		if (!GameActive) {
			result = CONSOLE_MENU_BACK;
			break;
		}
		if (GameInFocus && Poll_Input(result)) {
			break;
		}
		if (IsDirty) {
			Draw();
		}
		Sleep(1);
	}

	Keyboard->Clear();
	Show_Mouse();
	return(result);
}
