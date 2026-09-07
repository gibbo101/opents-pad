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
#include "mschoice.h"
#include "msfont.h"
#include "msgloop.h"
#include "newmenu.h"
#include "padglyph.h"
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
	GLYPH_INSET = 2,
	GLYPH_GAP = 6,
	BOX_INSET = 16,
	BOX_TITLE_GAP = 12,
	SWATCH_GAP = 10,
	SWATCH_INSET = 2,
	PANEL_OPACITY = 80,
	REPEAT_FIRST_MS = 350,
	REPEAT_NEXT_MS = 90,
	FAST_STEP = 5,
	FRAME_NORMAL = 2,
};


void Console_Draw_Icon(Surface & surface, Surface & icon, int x, int y)
{
	Rect source = icon.Get_Rect();
	SurfaceCache.DrawTrans(Rect(x, y, source.Width, source.Height), surface, icon, (255 >> DSurface::RedLeft << DSurface::RedRight) | (255u >> DSurface::BlueLeft << DSurface::BlueRight));
}


ConsoleMenuClass::ConsoleMenuClass(char const * title) :
	Title(title != NULL ? title : ""),
	AcceptPrompt("Accept"),
	BackPrompt("Back"),
	Font(NULL),
	FocusFont(NULL),
	Click(NULL),
	PanelOpacity(PANEL_OPACITY),
	PanelColor(0, 0, 0),
	BoxOuter(0, 0, 0),
	BoxInner(0, 0, 0),
	KeepBackdrop(false),
	IdleFont(NULL),
	FocusOverride(NULL),
	Backdrop(NULL),
	PreviousPad(),
	Focus(0),
	First(0),
	IsDirty(true),
	IsFinished(false),
	FinishResult(CONSOLE_MENU_BACK)
{
}


ConsoleMenuClass::~ConsoleMenuClass(void)
{
	delete Font;
	delete FocusFont;
	delete Click;
	for (auto & entry : ColorFonts) {
		delete entry.second;
	}
	delete Backdrop;
}


MSFont * ConsoleMenuClass::Font_For(RGBClass const & color)
{
	unsigned key = (unsigned(color.Get_Red()) << 16) | (unsigned(color.Get_Green()) << 8) | unsigned(color.Get_Blue());
	for (auto & entry : ColorFonts) {
		if (entry.first == key) return(entry.second);
	}
	MSFont * font = new MSFont(false);
	font->Set_Color(color);
	ColorFonts.push_back({key, font});
	return(font);
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


void ConsoleMenuClass::Set_Panel_Color(RGBClass const & color)
{
	PanelColor = color;
	IsDirty = true;
}


void ConsoleMenuClass::Set_Backdrop_From(Surface const & source, int dim_percent)
{
	if (Backdrop == NULL) {
		Backdrop = new DSurface(HiddenSurface->Get_Width(), HiddenSurface->Get_Height());
	}
	Backdrop->Fill(0);
	Backdrop->Blit_From(source);
	if (dim_percent > 0) {
		Backdrop->Fill_Rect_Trans(Backdrop->Get_Rect(), RGBClass(0, 0, 0), dim_percent);
	}
	KeepBackdrop = true;
	IsDirty = true;
}


void ConsoleMenuClass::Set_Box(Rect const & box, RGBClass const & outer, RGBClass const & inner)
{
	Box = box;
	BoxOuter = outer;
	BoxInner = inner;
	Panel = box;
	IsDirty = true;
}


void ConsoleMenuClass::Add_Hit_Area(Rect const & area, std::function<void()> hover, std::function<void()> click)
{
	Hits.push_back({area, hover, click});
}


void ConsoleMenuClass::Set_Backdrop_Panel(std::function<void(ConsoleCanvas &)> draw)
{
	BackdropPanel = draw;
	IsDirty = true;
}


int ConsoleMenuClass::Text_Width(char const * text)
{
	if (Font == NULL) {
		Font = new MSFont(false);
		FocusFont = new MSFont(false);
		FocusFont->Set_Color(RGBClass(48, 224, 248));
	}
	return(Font->Get_String_Width(text));
}


void ConsoleMenuClass::Set_Row_Colors(RGBClass const & idle, RGBClass const & focus)
{
	IdleFont = Font_For(idle);
	FocusOverride = Font_For(focus);
	IsDirty = true;
}


void ConsoleMenuClass::Set_Focus(int focus)
{
	if (Rows.empty()) return;
	Focus = std::clamp(focus, 0, int(Rows.size()) - 1);
	IsDirty = true;
}


void ConsoleMenuClass::Play_Click(void)
{
	if (Click == NULL) {
		Click = new MSSfxEntry("HighlightSound", (char *)"CHOICE1.AUD");
	}
	Click->Play();
}


void ConsoleMenuClass::Move_Focus(int step)
{
	if (Rows.empty()) return;
	int was = Focus;
	Focus = (Focus + step + int(Rows.size())) % int(Rows.size());
	if (Focus != was && !Rows[Focus].Quiet) {
		Play_Click();
	}
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

	Point2D mouse(Get_Mouse_X(), Get_Mouse_Y());
	Rect frame = HiddenSurface->Get_Rect();
	Point2D box_origin((frame.Width - MENU_WIDTH) / 2, (frame.Height - MENU_HEIGHT) / 2);
	auto hit_at = [&](Point2D const & point) -> HitType const * {
		for (HitType const & hit : Hits) {
			Rect area = hit.Area;
			area.X += box_origin.X;
			area.Y += box_origin.Y;
			if (area.Is_Point_Within(point)) return(&hit);
		}
		return(NULL);
	};
	auto row_at = [&](Point2D const & point) -> int {
		for (int index = 0; index < int(RowRects.size()); index++) {
			if (RowRects[index].Is_Valid() && RowRects[index].Is_Point_Within(point)) return(index);
		}
		return(-1);
	};
	if (mouse != LastMouse) {
		LastMouse = mouse;
		int row = row_at(mouse);
		if (row >= 0 && row != Focus) {
			Focus = row;
			if (!Rows[Focus].Quiet) Play_Click();
			IsDirty = true;
		}
		HitType const * hit = hit_at(mouse);
		if (hit != NULL && hit->Hover) {
			hit->Hover();
			IsDirty = true;
		}
	}

	while (Keyboard->Check() != KN_NONE) {
		KeyNumType key = KeyNumType(Keyboard->Get() & ~(WWKEY_SHIFT_BIT|WWKEY_ALT_BIT|WWKEY_CTRL_BIT|WWKEY_VK_BIT));
		if (key == KN_LMOUSE) {
			if (BackRect.Is_Point_Within(mouse)) {
				result = CONSOLE_MENU_BACK;
				return(true);
			}
			if (AcceptRect.Is_Point_Within(mouse)) {
				result = CONSOLE_MENU_ACCEPT;
				return(true);
			}
			HitType const * hit = hit_at(mouse);
			if (hit != NULL && hit->Click) {
				hit->Click();
				IsDirty = true;
				continue;
			}
			int row = row_at(mouse);
			if (row >= 0) {
				Focus = row;
				if (accept()) return(true);
			}
			continue;
		}
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

	if (pad.Up && !PreviousPad.Up) navigate(NAV_UP);
	if (pad.Down && !PreviousPad.Down) navigate(NAV_DOWN);
	if (pad.Left && !PreviousPad.Left) navigate(NAV_LEFT);
	if (pad.Right && !PreviousPad.Right) navigate(NAV_RIGHT);
	bool accept_pressed = pad.Accept && !PreviousPad.Accept;
	bool back_pressed = pad.Back && !PreviousPad.Back;
	PreviousPad = pad;
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
	if (PanelOpacity <= 0) {
		// Bare backdrop.
	} else if (Panel.Is_Valid()) {
		surface.Fill_Rect_Trans(Rect(left + Panel.X, top + Panel.Y, Panel.Width, Panel.Height), PanelColor, PanelOpacity);
	} else {
		surface.Fill_Rect_Trans(Rect(left + PANEL_INSET, top + PANEL_INSET, MENU_WIDTH - 2 * PANEL_INSET, MENU_HEIGHT - 2 * PANEL_INSET), PanelColor, PanelOpacity);
	}
	if (Box.Is_Valid()) {
		Rect frame_box(left + Box.X, top + Box.Y, Box.Width, Box.Height);
		int outer = DSurface::Build_Hicolor_Pixel(BoxOuter);
		int inner = DSurface::Build_Hicolor_Pixel(BoxInner);
		surface.Draw_Rect(frame_box, outer);
		surface.Draw_Rect(Rect(frame_box.X + 1, frame_box.Y + 1, frame_box.Width - 2, frame_box.Height - 2), outer);
		surface.Draw_Rect(Rect(frame_box.X + 3, frame_box.Y + 3, frame_box.Width - 6, frame_box.Height - 6), inner);
	}
	if (Font == NULL) {
		Font = new MSFont(false);
		FocusFont = new MSFont(false);
		FocusFont->Set_Color(RGBClass(48, 224, 248));
	}
	int height = Font->Get_Font_Height();
	auto print = [&](std::string const & text, int x, int y, bool focused = false) {
		MSFont * font = focused ? (FocusOverride != NULL ? FocusOverride : FocusFont) : (IdleFont != NULL ? IdleFont : Font);
		font->Draw_String(&surface, (unsigned char const *)text.c_str(), x, y, FRAME_NORMAL);
	};
	auto width = [&](std::string const & text) {
		return(Font->Get_String_Width(text.c_str()));
	};

	if (BackdropPanel) {
		ConsoleCanvas canvas = {
			surface, *Backdrop, Rect(left, top, MENU_WIDTH, MENU_HEIGHT),
			print,
			[&](std::string const & text, int x, int y, RGBClass const & color) {
				Font_For(color)->Draw_String(&surface, (unsigned char const *)text.c_str(), x, y, FRAME_NORMAL);
			},
			width,
			height,
		};
		BackdropPanel(canvas);
	}

	int rows_top = ROWS_TOP;
	int rows_bottom = ROWS_BOTTOM;
	if (Box.Is_Valid()) {
		rows_top = Box.Y + BOX_INSET;
		rows_bottom = Box.Y + Box.Height - BOX_INSET;
		print(Title, left + (MENU_WIDTH - width(Title)) / 2, top + Box.Y - height - BOX_TITLE_GAP);
	} else {
		print(Title, left + (MENU_WIDTH - width(Title)) / 2, top + TITLE_Y);
	}

	if (SidePanel) {
		SidePanel(surface, Rect(left + SIDE_X, top + SIDE_Y, SIDE_WIDTH, SIDE_HEIGHT));
	}

	int count = int(Rows.size());
	int listed = 0;
	for (ConsoleRowType const & row : Rows) {
		if (row.Y <= 0) listed++;
	}
	int pitch = height + 4;
	int visible = std::max((rows_bottom - rows_top) / pitch, 1);
	// A list a little too long is packed to fit; a long one scrolls, keeping the focused row in view.
	if (listed > visible && listed <= visible + visible / 2) {
		pitch = (rows_bottom - rows_top) / listed;
		visible = listed;
	}
	int focus_order = 0;
	for (int index = 0; index < Focus && index < count; index++) {
		if (Rows[index].Y <= 0) focus_order++;
	}
	if (Rows.empty() || Rows[Focus].Y > 0) focus_order = First;
	if (focus_order < First) First = focus_order;
	if (focus_order >= First + visible) First = focus_order - visible + 1;
	First = std::clamp(First, 0, std::max(listed - visible, 0));
	if (First > 0) {
		print("..", left + (MENU_WIDTH - width("..")) / 2, top + rows_top - height);
	}
	if (First + visible < listed) {
		print("..", left + (MENU_WIDTH - width("..")) / 2, top + rows_bottom);
	}
	RowRects.assign(count, Rect());
	int list_y = top + rows_top;
	int order = 0;
	for (int index = 0; index < count; index++) {
		ConsoleRowType const & row = Rows[index];
		bool focused = index == Focus;
		int y = row.Y > 0 ? top + row.Y : list_y;
		if (row.Y <= 0) {
			int slot = order++;
			if (slot < First || slot >= First + visible) continue;
			list_y += pitch;
		}
		if (row.Label.empty() && !row.Value) continue;
		RowRects[index] = Rect(left + PANEL_INSET, y, MENU_WIDTH - 2 * PANEL_INSET, pitch);
		if (!row.Value) {
			print(row.Label, left + (MENU_WIDTH - width(row.Label)) / 2, y, focused);
		} else {
			print(row.Label, left + LABEL_RIGHT - width(row.Label), y, focused);
		}
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
					Console_Draw_Icon(surface, *icon, x, y + (height - source.Height) / 2);
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
	}

	// A prompt is its button's glyph, when the style has one, then its text.
	int glyph = height + 2 * GLYPH_INSET;
	auto prompt = [&](std::string const & text, PadButtonType button, bool at_right) -> Rect {
		if (text.empty()) return(Rect());
		int used = Resolved_Prompt_Style() == PROMPT_STYLE_TEXT ? 0 : glyph + GLYPH_GAP;
		int total = used + width(text);
		int x = at_right ? left + MENU_WIDTH - PROMPT_INSET - total : left + PROMPT_INSET;
		if (used > 0) {
			Draw_Pad_Glyph(surface, button, x, top + PROMPT_Y - GLYPH_INSET, glyph);
		}
		print(text, x + used, top + PROMPT_Y);
		return(Rect(x - 8, top + PROMPT_Y - 4, total + 16, height + 8));
	};
	BackRect = prompt(BackPrompt, PAD_BUTTON_BACK, false);
	AcceptRect = prompt(AcceptPrompt, PAD_BUTTON_ACCEPT, true);

	Update_Visible_Surface(&surface);
	IsDirty = false;
}


ConsoleMenuResult ConsoleMenuClass::Process(void)
{
	ConsoleMenuResult result = CONSOLE_MENU_BACK;

	Keyboard->Clear();
	// A button still held from the screen before must not count as a press here.
	PreviousPad = Gamepad_Read();
	LastMouse = Point2D(Get_Mouse_X(), Get_Mouse_Y());

	// The map preview loader scribbles on AlternateSurface, so the backdrop is kept on a surface of its own.
	if (Backdrop == NULL) {
		Backdrop = new DSurface(HiddenSurface->Get_Width(), HiddenSurface->Get_Height());
	}
	if (!KeepBackdrop) {
		Backdrop->Fill(0);
		Load_Title_Screen(Get_New_Menu()->Background, Backdrop, &CCPalette);
	}
	IsDirty = true;

	while (true) {
		Call_Back();
		Windows_Message_Handler();
		if (Idle) {
			Idle();
		}
		if (!GameActive) {
			result = CONSOLE_MENU_BACK;
			break;
		}
		if (GameInFocus && Poll_Input(result)) {
			break;
		}
		if (IsFinished) {
			result = FinishResult;
			break;
		}
		if (IsDirty) {
			Hide_Mouse();
			Draw();
			Show_Mouse();
		}
		Sleep(1);
	}

	Keyboard->Clear();
	return(result);
}
