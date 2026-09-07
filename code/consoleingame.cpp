/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "goptions.h"

#include "_keyboar.h"
#include "_map.h"
#include "consolemenu.h"
#include "data.h"
#include "event.h"
#include "globals.h"
#include "house.h"
#include "language/language.h"
#include "loaddlg.h"
#include "mainopt.h"
#include "restate.h"
#include "savemgr.h"
#include "scenario.h"
#include "session.h"
#include "wincursor.h"

#include <string>


static bool Single_Player(void)
{
	return(Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH);
}


static bool Confirm(char const * title, char const * question)
{
	ConsoleMenuClass menu(title);
	menu.Set_Prompts("Confirm", "Back");
	menu.Add_Row({question, nullptr, nullptr, nullptr});
	menu.Set_Row_Quiet(0);
	return(menu.Process() == CONSOLE_MENU_ACCEPT);
}


// Saves into a fresh slot under the mission's name and reports how it went.
static std::string Save_Now(void)
{
	LoadOptionsClass saver;
	char filename[256];
	saver.Pick_Filename(filename);
	bool saved = saver.Save_File(filename, Scen->Description);
	return(Fetch_String(saved ? TXT_GAME_WAS_SAVED : TXT_ERROR_SAVING_GAME));
}


ConsoleIngameResult Console_Ingame_Menu(void)
{
	enum { ACTION_NONE, ACTION_RESUME, ACTION_OPTIONS, ACTION_BRIEFING, ACTION_SAVE, ACTION_LOAD, ACTION_RESTART, ACTION_ABORT };

	IgnoreInput = true;
	Keyboard->Clear();

	// The menu is its own screen at the shell's size, like the main menus, so its text and
	// artwork are the same on every panel; the play size returns on the way out.
	Shell_Display_Mode();

	ConsoleIngameResult result = INGAME_MENU_RESUME;
	std::string notice;
	int focus = -1;
	bool done = false;
	while (!done) {
		int action = ACTION_NONE;
		ConsoleMenuClass menu("Game Paused");
		menu.Set_Prompts("Select", "Return");
		auto add = [&](char const * label, int which) {
			menu.Add_Row({label, nullptr, nullptr, [&, which]{ action = which; menu.Finish(CONSOLE_MENU_ACCEPT); }});
		};
		add("Game Options", ACTION_OPTIONS);
		if (Session.Type == GAME_NORMAL) {
			add("Mission Briefing", ACTION_BRIEFING);
		}
		if (Single_Player() || SaveManager.Is_Multiplayer_Saving_Allowed()) {
			add("Save Game", ACTION_SAVE);
		}
		if (Single_Player() ? LoadOptionsClass().Files_Present() : (SaveManager.Multiplayer_Load_Is_Allowed() && MultiplayerLoadOptionsClass().Files_Present())) {
			add("Load Game", ACTION_LOAD);
		}
		if (Single_Player()) {
			add("Restart Mission", ACTION_RESTART);
		} else if (!PlayerPtr->IsDefeated && !PlayerPtr->IsToWin && !PlayerPtr->IsToLose && !PlayerPtr->IsToDie) {
			add("Surrender", ACTION_RESTART);
		}
		add("Abort Mission", ACTION_ABORT);
		add("Return To Mission", ACTION_RESUME);
		int return_row = int(menu.Row_Count()) - 1;
		if (!notice.empty()) {
			menu.Add_Row({notice, nullptr, nullptr, nullptr});
			menu.Set_Row_Quiet(int(menu.Row_Count()) - 1);
		}
		menu.Set_Focus(focus < 0 ? return_row : focus);
		ConsoleMenuResult outcome = menu.Process();
		focus = menu.Get_Focus();
		notice.clear();
		if (outcome != CONSOLE_MENU_ACCEPT || action == ACTION_NONE) {
			action = ACTION_RESUME;
		}

		switch (action) {
			case ACTION_RESUME:
				done = true;
				break;

			case ACTION_OPTIONS:
				Console_Options_Screen(true);
				break;

			case ACTION_BRIEFING:
				Restate_Mission(Scen);
				break;

			case ACTION_SAVE:
				if (Single_Player()) {
					notice = Save_Now();
				} else {
					OutList.push_back(EventClass(PlayerPtr->HeapID, EventClass::SAVEGAME));
					done = true;
				}
				break;

			case ACTION_LOAD:
				if (Single_Player()) {
					if (LoadOptionsClass().Console_Load()) {
						result = INGAME_MENU_LOADED;
						done = true;
					}
				} else {
					// The multiplayer list must run between frames, so the menu loop opens it.
					SpecialDialog = SDLG_LOAD;
					done = true;
				}
				break;

			case ACTION_RESTART:
				if (Single_Player() ? Confirm("Restart Mission", "Start the mission again from the beginning?")
						: Confirm(Fetch_String(TXT_SURRENDER), "Give up this game?")) {
					result = INGAME_MENU_RESTART;
					done = true;
				}
				break;

			case ACTION_ABORT:
				if (Confirm("Abort Mission", "Leave the mission and return to the menu?")) {
					result = INGAME_MENU_ABORT;
					done = true;
				}
				break;
		}
	}

	Play_Display_Mode();

	Keyboard->Clear();
	IgnoreInput = Scen->IsInputLocked;
	if (result == INGAME_MENU_LOADED) {
		if (!MouseCursor->Is_Hidden() && Scen->IsInputLocked) {
			Hide_Mouse();
		} else if (MouseCursor->Is_Hidden() && !Scen->IsInputLocked) {
			Show_Mouse();
		}
	}
	Map.Flag_To_Redraw(GS_REDRAW_ALL);
	return(result);
}
