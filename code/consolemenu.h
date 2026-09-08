/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "gamepad.h"
#include "padglyph.h"
#include "point.h"
#include "rect.h"
#include "rgb.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class MSFont;
class MSSfxEntry;
class Surface;

// The area every console screen lays out within, centred on the frame.
enum {
	CONSOLE_SHELL_WIDTH = 640,
	CONSOLE_SHELL_HEIGHT = 400,
	CONSOLE_PANEL_INSET = 8,
	CONSOLE_PANEL_OPACITY = 80,
	CONSOLE_TITLE_Y = 12,
	CONSOLE_PROMPT_Y = 368,
	CONSOLE_PROMPT_INSET = 24,
	CONSOLE_FRAME_NORMAL = 2,
};

// Focused text is teal; a boxed page's idle rows are blue and its focused row white.
extern RGBClass const CONSOLE_FOCUS_COLOR;
extern RGBClass const CONSOLE_IDLE_COLOR;

enum ConsoleNavType {
	CONSOLE_NAV_NONE,
	CONSOLE_NAV_UP,
	CONSOLE_NAV_DOWN,
	CONSOLE_NAV_LEFT,
	CONSOLE_NAV_RIGHT,
};

/*
 * Repeats a held direction at the menus' rate, since the engine drops the system's key
 * repeat: Press records a fresh press, and Due names the direction to step again this
 * tick, or none.
 */
class ConsoleRepeatClass
{
	public:
		void Press(ConsoleNavType nav);
		ConsoleNavType Due(GamepadStateType const & pad);

	private:
		ConsoleNavType Held = CONSOLE_NAV_NONE;
		unsigned int RepeatAt = 0;
};

/*
 * What a screen's own drawing gets from the menu: the frame, the undimmed backdrop, the
 * menu box, and the menu's text in its normal, focused, or a given colour.
 */
struct ConsoleCanvas
{
	Surface & Frame;
	Surface const & Backdrop;
	Rect Box;
	std::function<void(std::string const &, int, int, bool)> Print;
	std::function<void(std::string const &, int, int, RGBClass const &)> PrintColor;
	std::function<int(std::string const &)> Width;
	int LineHeight;
};

// A menu row: label, optional value stepped with left and right, optional action on
// accept. No value provider means a centred action row.
struct ConsoleRowType
{
	std::string Label;
	std::function<std::string()> Value;
	std::function<void(int)> Step;
	std::function<void()> Activate;
	std::function<Surface *()> Icon;			// A small image to show after the value, or NULL.
	// Colors to show after the value as hicolor pixels; the Selected one is drawn larger.
	std::function<std::vector<int>()> Swatches;
	std::function<int()> Selected;
	int Y = 0;									// A fixed line within the box for this row, or 0 to follow the list.
	bool Quiet = false;							// No click when the focus lands here.
	// What the accept prompt reads while this row has focus, when the row has its own action.
	std::string Prompt;
};

/// <summary>
/// Makes every console menu draw on the named title-screen file instead of the shell
/// page's backdrop until cleared with NULL, for screens opened from within a mission.
/// </summary>
void Console_Set_Backdrop_File(char const * name);

// The title-screen file console screens draw on right now.
char const * Console_Backdrop_File(void);

// That file at the frame's size, reloaded when the file or the frame size changes.
Surface & Console_Backdrop_Surface(void);

// Draws one of the lobby's magenta-keyed icons with its key left out.
void Console_Draw_Icon(Surface & surface, Surface & icon, int x, int y);

// A pad prompt's glyph box beside text of the given height, and the room the glyph and
// its gap take ahead of the text: none while prompts are plain text.
int Pad_Prompt_Glyph_Size(int line_height);
int Pad_Prompt_Inset(int line_height);

// Draws a prompt: the button's glyph, when the style has one, then the text. Returns the
// width taken.
int Draw_Pad_Prompt(Surface & surface, MSFont & font, PadButtonType button, char const * text, int x, int y);

enum ConsoleMenuResult
{
	CONSOLE_MENU_ACCEPT,
	CONSOLE_MENU_BACK,
};

/// <summary>
/// A console-style menu: a titled list of rows on the backdrop, one in focus, with accept
/// and back prompts. Process runs it until the player accepts or backs out.
/// </summary>
class ConsoleMenuClass
{
	public:
		ConsoleMenuClass(char const * title);
		~ConsoleMenuClass(void);

		int Add_Row(ConsoleRowType const & row);
		std::size_t Row_Count(void) const { return(Rows.size()); }
		void Set_Row_Step(int row, std::function<void(int)> step) { if (Valid_Row(row)) Rows[row].Step = step; }
		void Play_Click(void);
		void Set_Row_Quiet(int row) { if (Valid_Row(row)) Rows[row].Quiet = true; }
		void Set_Row_Y(int row, int y) { if (Valid_Row(row)) Rows[row].Y = y; }
		void Set_Row_Prompt(int row, char const * prompt) { if (Valid_Row(row)) Rows[row].Prompt = prompt; }
		std::string const & Row_Label(int row) const { static std::string const _none; return(Valid_Row(row) ? Rows[row].Label : _none); }
		void Set_Prompts(char const * accept, char const * back);
		// The pad's menu button runs the action from any row, with its own prompt.
		void Set_Menu_Button(char const * prompt, std::function<void()> action) { MenuPrompt = prompt; MenuAction = action; IsDirty = true; }
		void Set_Side_Panel(std::function<void(Surface &, Rect const &)> draw);
		void Set_Backdrop_Panel(std::function<void(ConsoleCanvas &)> draw);
		void Refresh(void) { IsDirty = true; }
		int Get_Focus(void) const { return(Focus); }
		void Set_Focus(int focus);
		// Runs once per pass of Process, for screens that must service something.
		void Set_Idle(std::function<void()> idle) { Idle = idle; }
		void Set_Panel(Rect const & panel) { Panel = panel; IsDirty = true; }	// The dark panel's area within the box; invalid means the whole box.
		void Set_Panel_Opacity(int percent) { PanelOpacity = percent; IsDirty = true; }	// Zero leaves the backdrop bare.
		void Set_Row_Colors(RGBClass const & idle, RGBClass const & focus);		// Replaces the white idle and teal focus text.
		int Text_Width(char const * text);
		static int Label_Right(void);			// Where a value row's label column ends, within the 640x400 area.
		static int Value_Left(void);			// Where a value row's value column starts.

		/// <summary>
		/// Ends Process with the given result once the current input has been handled, for a
		/// row whose activation is the menu's outcome.
		/// </summary>
		void Finish(ConsoleMenuResult result) { IsFinished = true; FinishResult = result; }

		/// <summary>
		/// Registers a mouse target within the menu box: pointing at it calls hover, clicking
		/// it calls click. Rows and the two prompts are targets already.
		/// </summary>
		void Add_Hit_Area(Rect const & area, std::function<void()> hover, std::function<void()> click);

		ConsoleMenuResult Process(void);

	private:
		bool Valid_Row(int row) const { return(row >= 0 && row < int(Rows.size())); }
		void Ensure_Fonts(void);
		void Absorb_Held_Buttons(void);
		void Move_Focus(int step);
		void Step_Value(int step);
		bool Poll_Input(ConsoleMenuResult & result);
		void Draw(void);
		MSFont * Font_For(RGBClass const & color);

		std::string Title;
		std::string AcceptPrompt;
		std::string BackPrompt;
		std::vector<ConsoleRowType> Rows;
		struct HitType {
			Rect Area;
			std::function<void()> Hover;
			std::function<void()> Click;
		};
		std::vector<HitType> Hits;
		std::vector<Rect> RowRects;			// Where each row was last drawn, in frame coordinates.
		Rect Panel;
		int PanelOpacity;
		std::string MenuPrompt;
		std::function<void()> MenuAction;
		MSFont * IdleFont;				// Replaces Font when a screen sets its own colours; owned by ColorFonts.
		MSFont * FocusOverride;
		Rect BackRect;
		Rect AcceptRect;
		Point2D LastMouse;
		std::function<void(Surface &, Rect const &)> SidePanel;
		std::function<void(ConsoleCanvas &)> BackdropPanel;
		std::function<void()> Idle;
		std::vector<std::pair<unsigned, std::unique_ptr<MSFont>>> ColorFonts;
		std::unique_ptr<MSFont> Font;
		std::unique_ptr<MSFont> FocusFont;
		std::unique_ptr<MSSfxEntry> Click;		// The shell's highlight sound, played as the focus moves.
		GamepadStateType PreviousPad;
		ConsoleRepeatClass Repeat;
		int Focus;
		int First;					// The first list row drawn when the list is longer than the panel.
		bool IsDirty;
		bool IsFinished;
		ConsoleMenuResult FinishResult;
};

// Boxes the labelled rows in the manner of the menu pages: one per line from first_y, or
// centred when first_y is 0, in a dark panel sized to the widest of them and min_width, or
// spanning the label and value columns when value_width is given. Returns the panel.
Rect Console_Box_Rows(ConsoleMenuClass & menu, int first_y, int min_width = 0, int value_width = 0);
