/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "skirmish.h"

#include "_rules.h"
#include "ccfile.h"
#include "consolekeyboard.h"
#include "consolemenu.h"
#include "consolemp.h"
#include "data.h"
#include "dsurface.h"
#include "globals.h"
#include "goptions.h"
#include "houstype.h"
#include "language/language.h"
#include "mapgen.h"
#include "mplayer.h"
#include "netshare.h"
#include "preview.h"
#include "rules.h"
#include "session.h"
#include "srfcache.h"
#include "surface.h"
#include "xsurface.h"

#include <algorithm>
#include <string>
#include <vector>

enum {
	SKIRMISH_MIN_MONEY = 2500,
	SKIRMISH_MONEY_STEP = 250,
	SKIRMISH_MAX_AI = 7,
};


std::string Console_On_Off(bool value)
{
	return(Fetch_String(value ? TXT_ON : TXT_OFF));
}


std::string Console_Tidy_Description(char const * text)
{
	std::string result;
	for (char const * p = text; *p != '\0'; p++) {
		if (*p == ' ' && (result.empty() || result.back() == ' ' || result.back() == '(' || p[1] == ')')) {
			continue;
		}
		result += *p;
	}
	return(result);
}


int Console_Wrap(int value, int low, int high)
{
	int span = high - low + 1;
	if (span <= 0) return(low);
	return(low + ((value - low) % span + span) % span);
}


void Console_Load_Map_Preview(void)
{
	delete MultiplayerMapPreview;
	MultiplayerMapPreview = nullptr;
	if (CCFileClass(Session.ScenarioFileName).Is_Available()) {
		MultiplayerMapPreview = new MapPreviewClass;
		if (stricmp(Session.Scenarios[Session.Options.ScenarioIndex]->Get_Filename(), RANDOM_MAP_FILE_NAME) == 0) {
			MultiplayerMapPreview->Read_PCX_Preview("RandMap.img");
		} else {
			MultiplayerMapPreview->Read_INI_Preview(Session.ScenarioFileName);
		}
	}
}


void Console_Draw_Map_Preview(Surface & surface, Rect const & panel)
{
	if (MultiplayerMapPreview == nullptr) return;
	XSurface * preview = MultiplayerMapPreview->Get_Preview_Surface();
	if (preview == nullptr) return;

	Rect source = preview->Get_Rect();
	int width = std::min(source.Width, panel.Width);
	int height = std::min(source.Height, panel.Height);
	Rect dest(panel.X + (panel.Width - width) / 2, panel.Y + (panel.Height - height) / 2, width, height);
	surface.Blit_From(dest, *preview, Rect(0, 0, width, height));
}


Surface * Console_Side_Icon(bool gdi)
{
	char const * name = gdi ? "gdii.pcx" : "nodi.pcx";
	Surface * icon = SurfaceCache.GetSurface(name);
	if (icon == nullptr && SurfaceCache.CachePCX(name)) {
		icon = SurfaceCache.GetSurface(name);
	}
	return(icon);
}


std::vector<int> Console_Player_Swatches(void)
{
	std::vector<int> colors;
	for (int index = 0; index < MAX_MPLAYER_COLORS; index++) {
		COLORREF rgb = PlayerColorTable[index];
		colors.push_back(DSurface::Build_Hicolor_Pixel(GetRValue(rgb), GetGValue(rgb), GetBValue(rgb)));
	}
	return(colors);
}


/// <summary>
/// Runs the console-style skirmish setup screen. It edits the same session settings as
/// the skirmish dialog and, when the player accepts, records the local player the same
/// way; when the player backs out only the handle, side, and color are kept.
/// </summary>
/// <returns>bool; Did the player accept the settings and ask for the game to start?</returns>
bool Console_Skirmish_Screen(void)
{
	std::vector<int> sides;
	for (int index = 0; index < HouseTypes.Count(); index++) {
		if (HouseTypes[index]->IsMultiplay) {
			sides.push_back(index);
		}
	}
	int side = std::clamp(std::min(int(Session.House), int(HOUSE_BAD)), 0, std::max(int(sides.size()) - 1, 0));

	int const color_count = MAX_MPLAYER_COLORS;
	int color = std::clamp(Session.PrefColor, 0, color_count - 1);

	int units = Session.Options.UnitCount;
	int tech = BuildLevel;
	int difficulty = int(Session.Options.AIDifficulty);
	int ai_players = std::max(Session.Options.AIPlayers, 1);
	int speed = std::clamp(6 - int(Session.Options.GameSpeed), 0, 6);
	int credits = Session.Options.Credits;
	bool bases = Session.Options.Bases;
	bool crates = Session.Options.Goodies;
	bool fog = Session.Options.FogOfWar;
	bool bridges = Session.Options.BridgeDestruction;
	bool redeploy = Session.Options.MCVRedeploy;
	bool short_game = Session.Options.ShortGame;
	bool engineers = Session.Options.CrapEngineers;

	Session.Options.ScenarioIndex = 0;
	Set_Scenario_Info_From_Index(0);
	Clear_Vector(&Session.Players);
	Clear_Vector(&Session.Computers);
	Console_Load_Map_Preview();

	auto max_ai = [&](void) {
		return(std::clamp(RandomMapWaypointCount(Session.Options.ScenarioIndex) - 1, 1, int(SKIRMISH_MAX_AI)));
	};
	ai_players = std::min(ai_players, max_ai());

	ConsoleMenuClass menu("Skirmish");
	menu.Set_Side_Panel(Console_Draw_Map_Preview);
	menu.Set_Prompts("Start", "Back");
	// The name is edited from the menu button on any row, so cross starts the game everywhere.
	menu.Set_Menu_Button("Name", [&]{
		std::string handle = Session.Handle;
		if (Console_Keyboard("Name", handle, MPLAYER_NAME_MAX - 1) && !handle.empty()) {
			strcpy(Session.Handle, handle.c_str());
			Session.Write_MultiPlayer_Settings();
		}
	});
	menu.Add_Row({"Name", [&]{ return(std::string(Session.Handle)); }, nullptr, nullptr});
	menu.Add_Row({"Side", [&]{ return(sides.empty() ? std::string() : std::string(HouseTypes[sides[side]]->GivenName)); },
		[&](int step) { side = Console_Wrap(side + step, 0, int(sides.size()) - 1); }, nullptr,
		[&]{ return(Console_Side_Icon(!sides.empty() && sides[side] == HOUSE_GOOD)); }});
	menu.Add_Row({"Color", [&]{ return(std::string()); },
		[&](int step) { color = Console_Wrap(color + step, 0, color_count - 1); }, nullptr, nullptr,
		Console_Player_Swatches, [&]{ return(color); }});
	menu.Add_Row({"Map", [&]{ return(std::to_string(Session.Options.ScenarioIndex + 1) + "/" + std::to_string(Session.Scenarios.Count()) + " " + Console_Tidy_Description(Session.Options.ScenarioDescription)); },
		[&](int step) {
			int count = Session.Scenarios.Count();
			if (count <= 0) return;
			int index = Console_Wrap(Session.Options.ScenarioIndex + step, 0, count - 1);
			if (Set_Scenario_Info_From_Index(index)) {
				Session.Options.ScenarioIndex = index;
				Console_Load_Map_Preview();
				ai_players = std::min(ai_players, max_ai());
			}
		}, nullptr});
	menu.Add_Row({"Unit Count", [&]{ return(std::to_string(units)); },
		[&](int step) { units = std::clamp(units + step, SessionClass::CountMin[1], SessionClass::CountMax[1]); }, nullptr});
	menu.Add_Row({"Tech Level", [&]{ return(std::to_string(tech)); },
		[&](int step) { tech = std::clamp(tech + step, 1, int(MPLAYER_BUILD_LEVEL_MAX)); }, nullptr});
	menu.Add_Row({"AI Level", [&]{ return(std::string(Fetch_String(difficulty == 0 ? TXT_EASY : difficulty == 1 ? TXT_NORMAL : TXT_HARD))); },
		[&](int step) { difficulty = std::clamp(difficulty + step, 0, 2); }, nullptr});
	menu.Add_Row({"AI Players", [&]{ return(std::to_string(ai_players)); },
		[&](int step) { ai_players = std::clamp(ai_players + step, 1, max_ai()); }, nullptr});
	menu.Add_Row({"Game Speed", [&]{ return(std::to_string(speed)); },
		[&](int step) { speed = std::clamp(speed + step, 0, 6); }, nullptr});
	menu.Add_Row({"Credits", [&]{ return(std::to_string(credits)); },
		[&](int step) { credits = std::clamp(credits + step * SKIRMISH_MONEY_STEP, int(SKIRMISH_MIN_MONEY), Rule->MPMaxMoney); }, nullptr});
	menu.Add_Row({"Bases", [&]{ return(Console_On_Off(bases)); },
		[&](int) { bases = !bases; if (!bases) short_game = false; }, nullptr});
	menu.Add_Row({"Crates", [&]{ return(Console_On_Off(crates)); }, [&](int) { crates = !crates; }, nullptr});
	menu.Add_Row({"Fog Of War", [&]{ return(Console_On_Off(fog)); }, [&](int) { fog = !fog; }, nullptr});
	menu.Add_Row({"Bridges Destroyable", [&]{ return(Console_On_Off(bridges)); }, [&](int) { bridges = !bridges; }, nullptr});
	menu.Add_Row({"Re-Deployable MCV", [&]{ return(Console_On_Off(redeploy)); }, [&](int) { redeploy = !redeploy; }, nullptr});
	menu.Add_Row({"Short Game", [&]{ return(Console_On_Off(short_game)); },
		[&](int) { short_game = !short_game; if (short_game) bases = true; }, nullptr});
	menu.Add_Row({"Multi Engineer", [&]{ return(Console_On_Off(engineers)); }, [&](int) { engineers = !engineers; }, nullptr});

	bool accepted = menu.Process() == CONSOLE_MENU_ACCEPT;

	Session.House = side;
	Session.ColorIdx = color;
	Session.PrefColor = color;

	if (accepted) {
		Session.Options.UnitCount = units;
		BuildLevel = tech;
		Session.Options.Credits = credits;
		Session.Options.AIDifficulty = DiffType(difficulty);
		Session.Options.AIPlayers = ai_players;
		Session.Options.GameSpeed = 6 - speed;
		Options.GameSpeed = Session.Options.GameSpeed;

		NodeNameType * who = new NodeNameType;
		strcpy(who->Name, Session.Handle);
		who->Player.House = Session.House;
		who->Player.Color = Session.ColorIdx;
		who->Player.ProcessTime = -1;
		Session.Players.Add(who);

		Session.Options.Bases = bases;
		Session.Options.Goodies = crates;
		Session.Options.FogOfWar = fog;
		Session.Options.BridgeDestruction = bridges;
		Session.Options.MCVRedeploy = redeploy;
		Session.Options.ShortGame = short_game;
		Session.Options.HarvTruce = false;
		Session.Options.CrapEngineers = engineers;
	}

	delete MultiplayerMapPreview;
	MultiplayerMapPreview = nullptr;
	return(accepted);
}
