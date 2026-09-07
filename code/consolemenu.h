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
#include "point.h"
#include "rect.h"

#include <functional>
#include <string>
#include <vector>

class MSFont;
class MSSfxEntry;
class RGBClass;
class Surface;

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

/*
 * One line of a console menu. A row shows a label and, when it has one, a value; the
 * player steps the value with left and right and activates the row with accept. A row
 * with no value provider is an action row, drawn as a single centered entry.
 */
struct ConsoleRowType
{
	std::string Label;
	std::function<std::string()> Value;
	std::function<void(int)> Step;
	std::function<void()> Activate;
	std::function<Surface *()> Icon;			// A small image to show after the value, or NULL.
	std::function<std::vector<int>()> Swatches;	// Colors to show after the value as hicolor pixels; the Selected one is drawn larger.
	std::function<int()> Selected;
	int Y = 0;									// A fixed line within the box for this row, or 0 to follow the list.
};

enum ConsoleMenuResult
{
	CONSOLE_MENU_ACCEPT,
	CONSOLE_MENU_BACK,
};

/// <summary>
/// A console-style menu: a titled list of rows on the menu backdrop, one row in focus, and
/// accept and back prompts along the bottom. Process runs the menu until the player accepts
/// or backs out and returns which.
/// </summary>
class ConsoleMenuClass
{
	public:
		ConsoleMenuClass(char const * title);
		~ConsoleMenuClass(void);

		int Add_Row(ConsoleRowType const & row);
		std::size_t Row_Count(void) const { return(Rows.size()); }
		void Set_Row_Step(int row, std::function<void(int)> step) { if (row >= 0 && row < int(Rows.size())) Rows[row].Step = step; }
		void Set_Prompts(char const * accept, char const * back);
		void Set_Side_Panel(std::function<void(Surface &, Rect const &)> draw);
		void Set_Backdrop_Panel(std::function<void(ConsoleCanvas &)> draw);
		void Refresh(void) { IsDirty = true; }
		int Get_Focus(void) const { return(Focus); }
		void Set_Focus(int focus);
		void Set_Idle(std::function<void()> idle) { Idle = idle; }
		void Set_Panel(Rect const & panel) { Panel = panel; IsDirty = true; }	// The dark panel's area within the box; invalid means the whole box.
		void Set_Panel_Opacity(int percent) { PanelOpacity = percent; IsDirty = true; }	// Zero leaves the backdrop bare.
		int Text_Width(char const * text);		// Runs once per pass of Process, for screens that must service something.

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
		void Move_Focus(int step);
		void Step_Value(int step);
		bool Poll_Input(ConsoleMenuResult & result);
		void Draw(void);

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
		std::vector<Rect> RowRects;			// Where each row was last drawn, in frame coordinates; invalid when not drawn.
		Rect Panel;
		int PanelOpacity;
		Rect BackRect;
		Rect AcceptRect;
		Point2D LastMouse;
		std::function<void(Surface &, Rect const &)> SidePanel;
		std::function<void(ConsoleCanvas &)> BackdropPanel;
		std::function<void()> Idle;
		std::vector<std::pair<unsigned, MSFont *>> ColorFonts;
		MSFont * Font_For(RGBClass const & color);
		MSFont * Font;
		MSFont * FocusFont;
		MSSfxEntry * Click;				// The shell's highlight sound, played as the focus moves.
		void Play_Click(void);
		Surface * Backdrop;
		GamepadStateType PreviousPad;
		int Focus;
		int First;					// The first list row drawn when the list is longer than the panel.
		bool IsDirty;
		bool IsFinished;
		ConsoleMenuResult FinishResult;
};
