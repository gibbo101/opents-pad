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
#include "consolekeyboard.h"
#include "consolemenu.h"
#include "data.h"
#include "event.h"
#include "globals.h"
#include "house.h"
#include "language/language.h"
#include "loaddlg.h"
#include "mainopt.h"
#include "restate.h"
#include "rgb.h"
#include "savemgr.h"
#include "scenario.h"
#include "session.h"
#include "wincursor.h"

#include <algorithm>
#include <cctype>
#include <string>


static bool Single_Player(void)
{
	return(Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH);
}


enum {
	ROW_PITCH = 26,
	PANEL_PAD = 16,
	TITLE_GAP = 36,
	MENU_HEIGHT = 400,
	GLYPH_TOP = 4,			// Where the menu font's letters start and end within its cell,
	GLYPH_BOTTOM = 15,		// so the box pads the letters evenly rather than the cell.
};

// Boxes the rows in the manner of the menu pages: each on its own line, centred as a group,
// the panel sized to the widest, the title above the box and any note beneath it. A value
// width means the rows carry values, so the box spans the label and value columns.
static void Box_Rows(ConsoleMenuClass & menu, std::string const & title, std::string const & note = std::string(), int value_width = 0)
{
	int count = int(menu.Row_Count());
	int first_y = (MENU_HEIGHT - count * ROW_PITCH) / 2 + 8;
	int widest = menu.Text_Width(title.c_str());
	for (int index = 0; index < count; index++) {
		menu.Set_Row_Y(index, first_y + index * ROW_PITCH);
		widest = std::max(widest, menu.Text_Width(menu.Row_Label(index).c_str()));
	}
	int box_top = first_y + GLYPH_TOP - PANEL_PAD;
	int box_bottom = first_y + (count - 1) * ROW_PITCH + GLYPH_BOTTOM + PANEL_PAD;
	Rect panel((640 - widest) / 2 - PANEL_PAD, box_top, widest + 2 * PANEL_PAD, box_bottom - box_top);
	if (value_width > 0) {
		int left = ConsoleMenuClass::Label_Right() - widest - PANEL_PAD;
		int right = ConsoleMenuClass::Value_Left() + value_width + PANEL_PAD;
		panel.X = left;
		panel.Width = right - left;
	}
	menu.Set_Panel(panel);
	menu.Set_Row_Colors(RGBClass(96, 208, 248), RGBClass(255, 255, 255));
	int note_y = box_bottom + TITLE_GAP / 2;
	menu.Set_Backdrop_Panel([title, note, first_y, note_y](ConsoleCanvas & canvas) {
		canvas.Print(title, canvas.Box.X + (canvas.Box.Width - canvas.Width(title)) / 2, canvas.Box.Y + first_y - TITLE_GAP - PANEL_PAD + GLYPH_TOP, false);
		if (!note.empty()) {
			canvas.Print(note, canvas.Box.X + (canvas.Box.Width - canvas.Width(note)) / 2, canvas.Box.Y + note_y, false);
		}
	});
}


// Asks the question as one row whose value flips between No and Yes, No to begin with,
// in the manner of the C&C pause menu; accepting on Yes is the only way to answer yes.
static bool Confirm(char const * question)
{
	bool yes = false;
	ConsoleMenuClass menu("");
	menu.Set_Prompts("Accept", "Back");
	menu.Add_Row({question, [&]{ return(std::string(Fetch_String(yes ? TXT_YES : TXT_NO))); }, [&](int) { yes = !yes; }, nullptr});
	int value_width = std::max(menu.Text_Width(Fetch_String(TXT_YES)), menu.Text_Width(Fetch_String(TXT_NO)));
	Box_Rows(menu, "Game Paused", std::string(), value_width);
	return(menu.Process() == CONSOLE_MENU_ACCEPT && yes);
}


// The name a save is offered under: a campaign mission's side and number from its map file,
// such as "GDI 01", ahead of the mission's name; a skirmish keeps the map's name alone.
static std::string Suggested_Save_Name(void)
{
	std::string name;
	if (Session.Type == GAME_NORMAL) {
		std::string side;
		std::string number;
		for (char const * p = Scen->ScenarioName; *p != '\0' && *p != '.'; p++) {
			if (isalpha((unsigned char)*p) && number.empty()) {
				side += char(toupper((unsigned char)*p));
			} else if (isdigit((unsigned char)*p)) {
				number += *p;
			} else {
				break;
			}
		}
		if (side == "NOD") side = "Nod";
		if (!side.empty() && !number.empty()) {
			if (number.size() < 2) number = "0" + number;
			name = side + " " + number + " - ";
		}
	}
	name += Scen->Description;
	if (name.size() >= DESCRIP_MAX) name.resize(DESCRIP_MAX - 1);
	return(name);
}


// Offers the save under its suggested name; accepting writes a fresh slot, posts the outcome
// to the mission's message list for when play resumes, and reports it for the menu meanwhile.
static std::string Save_Box(void)
{
	std::string name = Suggested_Save_Name();
	bool saving = false;
	while (!saving) {
		ConsoleMenuClass menu("");
		menu.Set_Prompts("Save", "Back");
		bool edit = false;
		menu.Add_Row({"Save As", [&]{ return(name); }, nullptr, [&]{ edit = true; menu.Finish(CONSOLE_MENU_ACCEPT); }});
		menu.Set_Row_Prompt(0, "Edit");
		Box_Rows(menu, "Game Paused", std::string(), menu.Text_Width(name.c_str()));
		if (menu.Process() != CONSOLE_MENU_ACCEPT) {
			return(std::string());
		}
		if (!edit) {
			saving = true;
		} else {
			std::string edited = name;
			if (Console_Keyboard("Save As", edited, DESCRIP_MAX - 1) && !edited.empty()) {
				name = edited;
			}
		}
	}
	LoadOptionsClass saver;
	char filename[256];
	saver.Pick_Filename(filename);
	bool saved = saver.Save_File(filename, name.c_str());
	SaveManager.Post_Save_Notice(saved ? TXT_GAME_WAS_SAVED : TXT_SAVE_FAILED);
	return(Fetch_String(saved ? TXT_GAME_WAS_SAVED : TXT_SAVE_FAILED));
}


ConsoleIngameResult Console_Ingame_Menu(void)
{
	enum { ACTION_NONE, ACTION_RESUME, ACTION_OPTIONS, ACTION_AUDIO, ACTION_CONTROLS, ACTION_BRIEFING, ACTION_SAVE, ACTION_LOAD, ACTION_RESTART, ACTION_ABORT };

	IgnoreInput = true;
	Keyboard->Clear();

	// The menu is its own screen at the shell's size, like the main menus, so its text and
	// artwork are the same on every panel; the play size returns on the way out.
	Shell_Display_Mode();
	// The mission's own plate, the one the briefing uses, rather than the shell's backdrop.
	Console_Set_Backdrop_File("SCORE.PCX");

	ConsoleIngameResult result = INGAME_MENU_RESUME;
	std::string notice;
	int focus = -1;
	bool done = false;
	while (!done) {
		int action = ACTION_NONE;
		ConsoleMenuClass menu("");
		menu.Set_Prompts("Select", "Return");
		auto add = [&](char const * label, int which) {
			menu.Add_Row({label, nullptr, nullptr, [&, which]{ action = which; menu.Finish(CONSOLE_MENU_ACCEPT); }});
		};
		add("Game Options", ACTION_OPTIONS);
		add("Audio Options", ACTION_AUDIO);
		add("Controls", ACTION_CONTROLS);
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
		Box_Rows(menu, "Game Paused", notice);
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

			case ACTION_AUDIO:
				Console_Audio_Screen(true);
				break;

			case ACTION_CONTROLS:
				Console_Controls_Screen();
				break;

			case ACTION_BRIEFING:
				Restate_Mission(Scen);
				break;

			case ACTION_SAVE:
				if (Single_Player()) {
					notice = Save_Box();
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
				if (Confirm(Single_Player() ? "Restart Mission" : Fetch_String(TXT_SURRENDER))) {
					result = INGAME_MENU_RESTART;
					done = true;
				}
				break;

			case ACTION_ABORT:
				if (Confirm("Abort Mission")) {
					result = INGAME_MENU_ABORT;
					done = true;
				}
				break;
		}
	}

	Console_Set_Backdrop_File(NULL);
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
