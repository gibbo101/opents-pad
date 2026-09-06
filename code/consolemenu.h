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
#include "rect.h"

#include <functional>
#include <string>
#include <vector>

class MSFont;
class Surface;

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
		void Set_Prompts(char const * accept, char const * back);
		void Set_Side_Panel(std::function<void(Surface &, Rect const &)> draw);
		void Set_Backdrop_Panel(std::function<void(Surface &, Surface const &, Rect const &)> draw);	// Given the frame, the undimmed backdrop, and the menu box.
		void Refresh(void) { IsDirty = true; }
		int Get_Focus(void) const { return(Focus); }

		/// <summary>
		/// Ends Process with the given result once the current input has been handled, for a
		/// row whose activation is the menu's outcome.
		/// </summary>
		void Finish(ConsoleMenuResult result) { IsFinished = true; FinishResult = result; }

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
		std::function<void(Surface &, Rect const &)> SidePanel;
		std::function<void(Surface &, Surface const &, Rect const &)> BackdropPanel;
		MSFont * Font;
		MSFont * FocusFont;
		Surface * Backdrop;
		GamepadStateType PreviousPad;
		int Focus;
		bool IsDirty;
		bool IsFinished;
		ConsoleMenuResult FinishResult;
};
