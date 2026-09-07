/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "netdlg2.h"

#include "_rand.h"
#include "_rules.h"
#include "_timer.h"
#include "addon.h"
#include "conquer.h"
#include "consolekeyboard.h"
#include "consolemenu.h"
#include "consolemp.h"
#include "data.h"
#include "globals.h"
#include "goptions.h"
#include "houstype.h"
#include "ipxmgr.h"
#include "language/language.h"
#include "mapgen.h"
#include "mplayer.h"
#include "netdlg.h"
#include "netshare.h"
#include "preview.h"
#include "rules.h"
#include "scenario.h"
#include "session.h"
#include "srfcache.h"
#include "stimer.h"
#include "surface.h"
#include "timer.h"

#include <algorithm>
#include <string>
#include <vector>

enum {
	LAN_MIN_MONEY = 2500,
	LAN_MONEY_STEP = 250,
	LAN_MAX_AI = 6,
	LAN_MAX_UNITS = 10,
	JOIN_TIMEOUT = 10 * TIMER_SECOND,
	SIGN_OFF_TIMEOUT = 10 * TIMER_SECOND,
	REQUEST_HOLD = 2 * TIMER_SECOND,
	PLAYERS_X = 24,
	PLAYERS_Y = 224,
	PLAYERS_BOTTOM = 364,
	PLAYERS_NAME_WIDTH = 100,
	PLAYERS_GAP = 6,
};

/*
 * The console-style LAN screens. The game list runs the same discovery as the dialog and
 * shows the games it finds as rows. Hosting and joining run the same join protocol as the
 * dialogs through the shared handlers; only the screens differ.
 */


static Surface * Cached_Icon(char const * name)
{
	Surface * icon = SurfaceCache.GetSurface(name);
	if (icon == NULL && SurfaceCache.CachePCX(name)) {
		icon = SurfaceCache.GetSurface(name);
	}
	return(icon);
}


static std::vector<int> Multiplay_Sides(void)
{
	std::vector<int> sides;
	for (int index = 0; index < HouseTypes.Count(); index++) {
		if (HouseTypes[index]->IsMultiplay) {
			sides.push_back(index);
		}
	}
	return(sides);
}


static std::string Side_Name(std::vector<int> const & sides, int side)
{
	if (side < 0 || side >= int(sides.size())) return(std::string());
	return(std::string(HouseTypes[sides[side]]->GivenName));
}


static bool Color_Taken(int color, int self)
{
	for (int index = 0; index < Session.Players.Count(); index++) {
		if (index != self && Session.Players[index]->Player.Color == color) return(true);
	}
	return(false);
}


// Steps a colour past any another player already holds.
static int Free_Color(int from, int step, int self)
{
	int color = Console_Wrap(from + step, 0, MAX_MPLAYER_COLORS - 1);
	int direction = step < 0 ? -1 : 1;
	for (int tries = 0; tries < MAX_MPLAYER_COLORS && Color_Taken(color, self); tries++) {
		color = Console_Wrap(color + direction, 0, MAX_MPLAYER_COLORS - 1);
	}
	return(color);
}


static std::string Difficulty_Name(int difficulty)
{
	return(Fetch_String(difficulty == 0 ? TXT_EASY : difficulty == 1 ? TXT_NORMAL : TXT_HARD));
}


// The players of the game, each in their colour with a faction icon and a host or ready mark.
static void Draw_Players(ConsoleCanvas & canvas)
{
	int count = Session.Players.Count();
	if (count == 0) return;
	int pitch = std::min(canvas.LineHeight + 2, (PLAYERS_BOTTOM - PLAYERS_Y) / count);
	int y = canvas.Box.Y + PLAYERS_Y;
	int left = canvas.Box.X + PLAYERS_X;
	for (int index = 0; index < count; index++, y += pitch) {
		NodeNameType const & player = *Session.Players[index];
		COLORREF rgb = PlayerColorTable[std::clamp(player.Player.Color, 0, MAX_MPLAYER_COLORS - 1)];
		std::string name = player.Name;
		while (name.size() > 1 && canvas.Width(name) > PLAYERS_NAME_WIDTH) {
			name.pop_back();
		}
		canvas.PrintColor(name, left, y, RGBClass(GetRValue(rgb), GetGValue(rgb), GetBValue(rgb)));
		int x = left + PLAYERS_NAME_WIDTH + PLAYERS_GAP;
		Surface * side = Console_Side_Icon(player.Player.House == HOUSE_GOOD);
		if (side != NULL) {
			Rect source = side->Get_Rect();
			Console_Draw_Icon(canvas.Frame, *side, x, y + (canvas.LineHeight - source.Height) / 2);
			x += source.Width + PLAYERS_GAP;
		}
		bool is_host = strcmp(player.Name, Session.GameName) == 0;
		Surface * mark = is_host ? Cached_Icon("wolhost.pcx") : player.Player.Status != 0 ? Cached_Icon("wolacpt.pcx") : NULL;
		if (mark != NULL) {
			Console_Draw_Icon(canvas.Frame, *mark, x, y + (canvas.LineHeight - mark->Get_Rect().Height) / 2);
		}
	}
}


// A digest of what the player panel and option rows show, so the screen redraws only on change.
static std::string Lobby_Signature(void)
{
	std::string text;
	for (int index = 0; index < Session.Players.Count(); index++) {
		NodeNameType const & player = *Session.Players[index];
		text += player.Name;
		text += ',' + std::to_string(player.Player.House) + ',' + std::to_string(player.Player.Color) + ',' + std::to_string(player.Player.Status) + ';';
	}
	text += Session.Options.ScenarioDescription;
	text += ';' + std::to_string(Session.Options.UnitCount) + ',' + std::to_string(BuildLevel) + ',' + std::to_string(Session.Options.Credits)
		+ ',' + std::to_string(Session.Options.AIPlayers) + ',' + std::to_string(int(Session.Options.AIDifficulty)) + ',' + std::to_string(Session.Options.GameSpeed)
		+ ',' + std::to_string(Session.Options.Bases) + ',' + std::to_string(Session.Options.Goodies) + ',' + std::to_string(Session.Options.FogOfWar)
		+ ',' + std::to_string(Session.Options.BridgeDestruction) + ',' + std::to_string(Session.Options.MCVRedeploy) + ',' + std::to_string(Session.Options.ShortGame)
		+ ',' + std::to_string(Session.Options.CrapEngineers) + ',' + std::to_string(Session.Options.AlliesAllowed) + ',' + std::to_string(Session.Options.HarvTruce);
	text += ';' + std::to_string(MultiplayerMapPreview != NULL);
	return(text);
}


// The option rows every lobby shows; the host steps them and a guest only reads them.
static void Add_Option_Rows(ConsoleMenuClass & menu, bool host, std::function<void()> changed, std::function<int()> max_ai)
{
	auto step_if_host = [&](std::function<void(int)> step) -> std::function<void(int)> {
		if (!host) return(nullptr);
		return([step, changed](int amount) { step(amount); changed(); });
	};
	menu.Add_Row({"Unit Count", [&]{ return(std::to_string(Session.Options.UnitCount)); },
		step_if_host([](int step) { Session.Options.UnitCount = std::clamp(Session.Options.UnitCount + step, 1, int(LAN_MAX_UNITS)); }), nullptr});
	menu.Add_Row({"Tech Level", [&]{ return(std::to_string(BuildLevel)); },
		step_if_host([](int step) { BuildLevel = std::clamp(BuildLevel + step, 1, int(MPLAYER_BUILD_LEVEL_MAX)); }), nullptr});
	menu.Add_Row({"AI Level", [&]{ return(Difficulty_Name(int(Session.Options.AIDifficulty))); },
		step_if_host([](int step) { Session.Options.AIDifficulty = DiffType(std::clamp(int(Session.Options.AIDifficulty) + step, 0, 2)); }), nullptr});
	menu.Add_Row({"AI Players", [&]{ return(std::to_string(Session.Options.AIPlayers)); },
		step_if_host([max_ai](int step) { Session.Options.AIPlayers = std::clamp(Session.Options.AIPlayers + step, 0, max_ai()); }), nullptr});
	menu.Add_Row({"Game Speed", [&]{ return(std::to_string(6 - Session.Options.GameSpeed)); },
		step_if_host([](int step) { Session.Options.GameSpeed = 6 - std::clamp(6 - Session.Options.GameSpeed + step, 0, 6); Options.GameSpeed = Session.Options.GameSpeed; }), nullptr});
	menu.Add_Row({"Credits", [&]{ return(std::to_string(Session.Options.Credits)); },
		step_if_host([](int step) { Session.Options.Credits = std::clamp(Session.Options.Credits + step * LAN_MONEY_STEP, int(LAN_MIN_MONEY), Rule->MPMaxMoney); }), nullptr});
	menu.Add_Row({"Bases", [&]{ return(Console_On_Off(Session.Options.Bases)); },
		step_if_host([](int) { Session.Options.Bases = !Session.Options.Bases; if (!Session.Options.Bases) Session.Options.ShortGame = false; }), nullptr});
	menu.Add_Row({"Crates", [&]{ return(Console_On_Off(Session.Options.Goodies)); },
		step_if_host([](int) { Session.Options.Goodies = !Session.Options.Goodies; }), nullptr});
	menu.Add_Row({"Fog Of War", [&]{ return(Console_On_Off(Session.Options.FogOfWar)); },
		step_if_host([](int) { Session.Options.FogOfWar = !Session.Options.FogOfWar; }), nullptr});
	menu.Add_Row({"Bridges Destroyable", [&]{ return(Console_On_Off(Session.Options.BridgeDestruction)); },
		step_if_host([](int) { Session.Options.BridgeDestruction = !Session.Options.BridgeDestruction; }), nullptr});
	menu.Add_Row({"Re-Deployable MCV", [&]{ return(Console_On_Off(Session.Options.MCVRedeploy)); },
		step_if_host([](int) { Session.Options.MCVRedeploy = !Session.Options.MCVRedeploy; }), nullptr});
	menu.Add_Row({"Short Game", [&]{ return(Console_On_Off(Session.Options.ShortGame)); },
		step_if_host([](int) { Session.Options.ShortGame = !Session.Options.ShortGame; if (Session.Options.ShortGame) Session.Options.Bases = true; }), nullptr});
	menu.Add_Row({"Multi Engineer", [&]{ return(Console_On_Off(Session.Options.CrapEngineers)); },
		step_if_host([](int) { Session.Options.CrapEngineers = !Session.Options.CrapEngineers; }), nullptr});
	menu.Add_Row({"Allies Allowed", [&]{ return(Console_On_Off(Session.Options.AlliesAllowed)); },
		step_if_host([](int) { Session.Options.AlliesAllowed = !Session.Options.AlliesAllowed; }), nullptr});
	menu.Add_Row({"Harvester Truce", [&]{ return(Console_On_Off(Session.Options.HarvTruce)); },
		step_if_host([](int) { Session.Options.HarvTruce = !Session.Options.HarvTruce; }), nullptr});
}


static void Service_Lobby(void)
{
	Ipx.Service();
	Send_Join_Queries(false, false, false, false);
	Get_Join_Responses();
	Net2ServiceGameList();
}


// Signs off from everyone still listening and waits for the acknowledgements.
static void Sign_Off_Everywhere(void)
{
	if (JoinState == JOIN_CONFIRMED) {
		Unjoin_Game(CurGame);
	} else {
		GlobalPacketType packet;
		memset(&packet, 0, sizeof(packet));
		packet.Command = NET_SIGN_OFF;
		strcpy(packet.Name, Session.Handle);
		for (int index = 1; index < Session.Chat.Count(); index++) {
			Ipx.Send_Global_Message(&packet, sizeof(packet), 1, &Session.Chat[index]->Address);
			Call_Back();
		}
		Ipx.Send_Global_Message(&packet, sizeof(packet), 0, NULL);
	}
	CDTimerClass<SystemTimerClass> timeout = SIGN_OFF_TIMEOUT;
	while (Ipx.Global_Num_Send() > 0 && timeout != 0) {
		if (Ipx.Service() == 0) break;
		Call_Back();
	}
	Session.GameName[0] = '\0';
	JoinState = JOIN_NOTHING;
	_netresponse = 0;
	CurGame = 0;
	Clear_Vector(&Session.Players);
}


static void Enter_Lobby(void)
{
	CurGame = 0;
	Net2IsGameListActive = true;
	JoinState = JOIN_NOTHING;
	_netresponse = 0;
	Net2GameStarted = false;
	Net2ConsoleNotice.clear();
	Session.GameName[0] = '\0';
	Session.Options.ScenarioDescription[0] = '\0';
	Session.ColorIdx = Session.PrefColor;
	Clear_Vector(&Session.Games);
	Clear_Vector(&Session.Players);
	Clear_Vector(&Session.Chat);
	NodeNameType * who = new NodeNameType;
	strcpy(who->Name, Session.Handle);
	who->Chat.LastTime = 0;
	who->Chat.LastChance = 0;
	who->Chat.Color = Session.GPacket.PlayerInfo.Color;
	Session.Chat.Add(who);
	NodeNameType * game = new NodeNameType;
	strcpy(game->Name, "");
	game->Game.IsOpen = 0;
	game->Game.LastTime = 0;
	Session.Games.Add(game);
	Send_Join_Queries(true, false, true, true);
}


static void Leave_Lobby(void)
{
	Session.Write_MultiPlayer_Settings();
	if (JoinState > JOIN_NOTHING) {
		Unjoin_Game(CurGame);
		Ipx.Service();
	}
	JoinState = JOIN_NOTHING;
	Clear_Vector(&Session.Players);
	Clear_Vector(&Session.Games);
	Clear_Vector(&Session.Chat);
	Session.NetOpen = false;
	Ipx.Service();
}


// Why the game at this index cannot be joined, or an empty string when it can.
static std::string Join_Problem(int index)
{
	if (index < 1 || index >= Session.Games.Count()) return(Fetch_String(TXT_NOTHING_TO_JOIN));
	NodeNameType const & game = *Session.Games[index];
	if (!game.Game.IsOpen) return(Fetch_String(TXT_GAME_IS_CLOSED));
	if (game.Game.Addon == ADDON_FIRESTORM && !Addon_Enabled(ADDON_FIRESTORM)) {
		return(Fetch_String(Addon_Installed(ADDON_FIRESTORM) ? TXT_FIRESTORM_MUST_ENABLE : TXT_FIRESTORM_REQUIRED));
	}
	if (game.Game.Addon == ADDON_BASE_GAME && Addon_Enabled(ADDON_FIRESTORM)) return(Fetch_String(TXT_FIRESTORM_NO_JOIN_TS));
	return(std::string());
}


/// <summary>
/// Runs the host's lobby: the skirmish rows, live for the host, with the joined players
/// listed under the map preview. Returns true once the game has been started, with the
/// players told to go; false when the host backs out, with the game disbanded.
/// </summary>
static bool Console_Host_Screen(std::string & notice)
{
	strcpy(Session.GameName, Session.Handle);
	Session.NetOpen = true;
	Session.NetStealth = false;
	Session.Options.ScenarioIndex = 0;
	Session.PlayingAgainstVersion = VerNum.Version_Number();
	Set_Scenario_Info_From_Index(0);
	Clear_Vector(&Session.Players);

	NodeNameType * who = new NodeNameType;
	strcpy(who->Name, Session.Handle);
	strcpy(who->Player.Serial, SerialNumber);
	who->Player.House = Session.House;
	who->Player.Color = Session.ColorIdx;
	who->Player.Status = 1;
	Session.Players.Add(who);
	JoinState = JOIN_CONFIRMED;

	NodeNameType * game = new NodeNameType;
	strcpy(game->Name, Session.Handle);
	game->Address = Session.GAddress;
	game->Game.IsOpen = true;
	game->Game.LastTime = TickCount;
	game->Game.Addon = Addon_Enabled(ADDON_FIRESTORM);
	Session.Games.Add(game);
	CurGame = Session.Games.Count() - 1;
	Net2IsGameListActive = false;

	VerNum.Init_Clipping();
	srand(NonCriticalRandomNumber(1, 0x7FFF));
	Seed = rand();
	Net2GameStarted = false;
	_netresponse = 0;
	Console_Load_Map_Preview();

	std::vector<int> sides = Multiplay_Sides();
	int side = std::clamp(Session.House, 0, std::max(int(sides.size()) - 1, 0));
	auto max_ai = [](void) {
		return(std::clamp(RandomMapWaypointCount(Session.Options.ScenarioIndex) - Session.Players.Count(), 0, int(LAN_MAX_AI)));
	};
	auto changed = [](void) { PumpGameopts(true, false); };

	bool started = false;
	int focus = 0;
	while (!started) {
		std::string signature;
		ConsoleMenuClass menu("Host Game");
		menu.Set_Prompts("Start", "Back");
		menu.Set_Side_Panel(Console_Draw_Map_Preview);
		menu.Set_Backdrop_Panel(Draw_Players);

		menu.Add_Row({"Side", [&]{ return(Side_Name(sides, side)); },
			[&](int step) {
				side = Console_Wrap(side + step, 0, int(sides.size()) - 1);
				Session.House = side;
				Session.Players[0]->Player.House = side;
				changed();
			}, nullptr,
			[&]{ return(Console_Side_Icon(!sides.empty() && sides[side] == HOUSE_GOOD)); }});
		menu.Add_Row({"Color", [&]{ return(std::string()); },
			[&](int step) {
				int color = Free_Color(Session.ColorIdx, step, 0);
				Session.ColorIdx = color;
				Session.PrefColor = color;
				Session.Players[0]->Player.Color = color;
				changed();
			}, nullptr, nullptr, Console_Player_Swatches, [&]{ return(Session.ColorIdx); }});
		menu.Add_Row({"Map", [&]{ return(std::to_string(Session.Options.ScenarioIndex + 1) + "/" + std::to_string(Session.Scenarios.Count()) + " " + Console_Tidy_Description(Session.Options.ScenarioDescription)); },
			[&](int step) {
				int count = Session.Scenarios.Count();
				if (count <= 0) return;
				// The generated map needs its own dialog, so the list steps past it.
				int index = Console_Wrap(Session.Options.ScenarioIndex + step, 0, count - 1);
				for (int tries = 0; tries < count && stricmp(Session.Scenarios[index]->Get_Filename(), RANDOM_MAP_FILE_NAME) == 0; tries++) {
					index = Console_Wrap(index + (step < 0 ? -1 : 1), 0, count - 1);
				}
				if (Set_Scenario_Info_From_Index(index)) {
					Session.Options.ScenarioIndex = index;
					Console_Load_Map_Preview();
					Session.Options.AIPlayers = std::min(Session.Options.AIPlayers, max_ai());
					changed();
				}
			}, nullptr});
		Add_Option_Rows(menu, true, changed, max_ai);
		if (!notice.empty()) {
			menu.Add_Row({notice, nullptr, nullptr, nullptr});
			menu.Set_Row_Quiet(int(menu.Row_Count()) - 1);
		}
		menu.Set_Focus(focus);
		menu.Set_Idle([&]{
			Service_Lobby();
			PumpGameopts(false);
			std::string now = Lobby_Signature();
			if (now != signature) {
				signature = now;
				menu.Refresh();
			}
		});
		ConsoleMenuResult result = menu.Process();
		focus = menu.Get_Focus();
		notice.clear();
		if (result != CONSOLE_MENU_ACCEPT) {
			break;
		}

		Session.Write_MultiPlayer_Settings();
		if (Session.Players.Count() == 1) {
			notice = Fetch_String(TXT_ONLY_ONE);
			continue;
		}
		bool waiting = false;
		for (int index = 0; index < Session.Players.Count(); index++) {
			if (Session.Players[index]->Player.Status == 0) waiting = true;
		}
		if (waiting) {
			notice = Fetch_String(TXT_ACCEPTFIRST);
			continue;
		}
		if (RandomMapWaypointCount(Session.Options.ScenarioIndex) < Session.Options.AIPlayers + Session.Players.Count()) {
			notice = Fetch_String(TXT_SCENARIO_TOO_SMALL);
			continue;
		}
		Net2Start_Hosted_Game();
		started = true;
	}

	if (!started) {
		delete MultiplayerMapPreview;
		MultiplayerMapPreview = NULL;
		Unjoin_Game(CurGame);
		JoinState = JOIN_NOTHING;
		Net2GameStarted = false;
	}
	return(started);
}


/// <summary>
/// Runs a guest's lobby: the host's rows read only, the guest's own side and colour live,
/// and Ready as the accept. Returns true when the host starts the game and this machine is
/// ready to play; false when the guest leaves or the game goes away, with the reason in
/// the notice.
/// </summary>
static bool Console_Guest_Screen(std::string & notice)
{
	Net2IsGameListActive = false;
	for (int index = 0; index < Session.Players.Count(); index++) {
		if (strcmp(Session.Players[index]->Name, Session.Handle) == 0) {
			Session.Players[index]->Player.Status = 0;
		}
	}
	Session.Options.ScenarioDescription[0] = '\0';
	_netresponse = 0;
	Net2GameStarted = false;
	delete MultiplayerMapPreview;
	MultiplayerMapPreview = NULL;

	std::vector<int> sides = Multiplay_Sides();
	int wanted_side = -1;
	int wanted_color = -1;
	CDTimerClass<SystemTimerClass> request_hold;
	auto own_side = [&](void) { return(Session.Players.Count() > 0 ? Session.Players[0]->Player.House : Session.House); };
	auto own_color = [&](void) { return(Session.Players.Count() > 0 ? Session.Players[0]->Player.Color : Session.ColorIdx); };
	auto request = [&](int side, int color) {
		char text[64];
		sprintf(text, "R%d,%d", side, color);
		SendPrivateGameopts(Session.GameName, text);
		request_hold = REQUEST_HOLD;
	};
	request(own_side(), own_color());

	char title[80];
	sprintf(title, Fetch_String(TXT_THATGUYS_GAME), Session.GameName);

	bool started = false;
	bool gone = false;
	int focus = 0;
	while (!started && !gone) {
		std::string signature;
		ConsoleMenuClass menu(title);
		menu.Set_Prompts("Ready", "Leave");
		menu.Set_Side_Panel(Console_Draw_Map_Preview);
		menu.Set_Backdrop_Panel(Draw_Players);

		menu.Add_Row({"Side", [&]{ return(Side_Name(sides, wanted_side >= 0 ? wanted_side : own_side())); },
			[&](int step) {
				wanted_side = Console_Wrap((wanted_side >= 0 ? wanted_side : own_side()) + step, 0, int(sides.size()) - 1);
				request(wanted_side, wanted_color >= 0 ? wanted_color : own_color());
			}, nullptr,
			[&]{ int side = wanted_side >= 0 ? wanted_side : own_side(); return(Console_Side_Icon(side >= 0 && side < int(sides.size()) && sides[side] == HOUSE_GOOD)); }});
		menu.Add_Row({"Color", [&]{ return(std::string()); },
			[&](int step) {
				wanted_color = Free_Color(wanted_color >= 0 ? wanted_color : own_color(), step, 0);
				Session.PrefColor = wanted_color;
				request(wanted_side >= 0 ? wanted_side : own_side(), wanted_color);
			}, nullptr, nullptr, Console_Player_Swatches, [&]{ return(wanted_color >= 0 ? wanted_color : own_color()); }});
		menu.Add_Row({"Map", [&]{ return(Console_Tidy_Description(Session.Options.ScenarioDescription)); }, nullptr, nullptr});
		Add_Option_Rows(menu, false, nullptr, nullptr);
		menu.Add_Row({"Status", [&]{
			if (Session.Players.Count() > 0 && Session.Players[0]->Player.Status != 0) return(std::string("Ready, waiting for the host"));
			return(std::string("Press Ready when set"));
		}, nullptr, nullptr});
		menu.Set_Focus(focus);
		menu.Set_Idle([&]{
			Service_Lobby();
			if (request_hold == 0) {
				wanted_side = -1;
				wanted_color = -1;
			}
			if (_netresponse == IDOK) {
				started = true;
				menu.Finish(CONSOLE_MENU_ACCEPT);
				return;
			}
			if (_netresponse == 2 || _netresponse == IDCANCEL || JoinState == JOIN_REJECTED) {
				gone = true;
				menu.Finish(CONSOLE_MENU_BACK);
				return;
			}
			std::string now = Lobby_Signature() + std::to_string(wanted_side) + ',' + std::to_string(wanted_color);
			if (now != signature) {
				signature = now;
				menu.Refresh();
			}
		});
		ConsoleMenuResult result = menu.Process();
		focus = menu.Get_Focus();
		if (started || gone) {
			break;
		}
		if (result == CONSOLE_MENU_ACCEPT) {
			if (Session.Players.Count() > 0 && Session.Players[0]->Player.Status == 0) {
				Session.Players[0]->Player.Status = 1;
				SendPublicGameopts("A1");
			}
			continue;
		}
		Sign_Off_Everywhere();
		return(false);
	}

	if (started) {
		Net2Start_Joined_Game();
		return(true);
	}
	notice = Net2ConsoleNotice.empty() ? std::string("The host closed the game") : Net2ConsoleNotice;
	Net2ConsoleNotice.clear();
	Session.GameName[0] = '\0';
	JoinState = JOIN_NOTHING;
	_netresponse = 0;
	CurGame = 0;
	Clear_Vector(&Session.Players);
	return(false);
}


/// <summary>
/// Runs the console LAN screens in place of the dialogs: the game list, then the host or
/// guest lobby. Returns true with the session set up and the players told to go; false
/// when the player backs out of the list.
/// </summary>
bool Net2Console_Remote_Connect(void)
{
	enum { ACTION_NONE, ACTION_HOST, ACTION_JOIN };

	RulesID = RulesClass::Get_Rule_Unique_ID();
	RulesClass::Load_Art_INI();
	ArtID = RulesClass::Get_Art_Unique_ID();
	AIID = RulesClass::Get_AI_Unique_ID();
	Decrypt_Serial(SerialNumber);
	Ipx.Set_Timing(TIMER_SECOND / 2, -1, 10 * TIMER_SECOND);
	Session.NetOpen = true;
	Session.NetStealth = false;
	Net2IsConsole = true;
	Enter_Lobby();

	int focus = 1;
	std::string notice;
	bool connected = false;
	CDTimerClass<SystemTimerClass> join_timer;
	while (true) {
		int action = ACTION_NONE;
		bool rebuild = false;
		int listed = Session.Games.Count();
		int chatters = Session.Chat.Count();
		bool joining = JoinState == JOIN_WAIT_CONFIRM;

		ConsoleMenuClass menu("LAN Games");
		menu.Set_Prompts("Select", "Back");
		menu.Add_Row({"Name", [&]{ return(std::string(Session.Handle)); }, nullptr, [&]{
			if (joining) return;
			std::string handle = Session.Handle;
			if (Console_Keyboard("Name", handle, MPLAYER_NAME_MAX - 1) && !handle.empty() && handle != Session.Handle) {
				strcpy(Session.Handle, handle.c_str());
				Session.Write_MultiPlayer_Settings();
				strcpy(Session.Chat[0]->Name, Session.Handle);
				Send_Join_Queries(false, false, true, false);
			}
			rebuild = true;
			menu.Finish(CONSOLE_MENU_BACK);
		}});
		menu.Add_Row({"Host New Game", nullptr, nullptr, [&]{ if (!joining) { action = ACTION_HOST; menu.Finish(CONSOLE_MENU_ACCEPT); } }});
		for (int index = 1; index < Session.Games.Count(); index++) {
			menu.Add_Row({std::string(Session.Games[index]->Name) + (Session.Games[index]->Game.IsOpen ? "" : " (closed)"), nullptr, nullptr,
				[&, index]{ if (!joining) { CurGame = index; action = ACTION_JOIN; menu.Finish(CONSOLE_MENU_ACCEPT); } }});
		}
		if (Session.Games.Count() <= 1) {
			menu.Add_Row({"No games found yet", nullptr, nullptr, nullptr});
		}
		menu.Add_Row({"In the lobby", [&]{ return(std::to_string(std::max(Session.Chat.Count(), 1))); }, nullptr, nullptr});
		if (joining) {
			menu.Add_Row({"Joining " + std::string(Session.Games[CurGame]->Name) + "..", nullptr, nullptr, nullptr});
			menu.Set_Row_Quiet(int(menu.Row_Count()) - 1);
		} else if (!notice.empty()) {
			menu.Add_Row({notice, nullptr, nullptr, nullptr});
			menu.Set_Row_Quiet(int(menu.Row_Count()) - 1);
		}
		menu.Set_Focus(focus);
		menu.Set_Idle([&]{
			Service_Lobby();
			if (JoinState == JOIN_CONFIRMED) {
				rebuild = true;
				menu.Finish(CONSOLE_MENU_BACK);
				return;
			}
			if (joining && (JoinState != JOIN_WAIT_CONFIRM || join_timer == 0)) {
				notice = Net2ConsoleNotice.empty() ? "No answer from " + std::string(Session.Games[CurGame]->Name) : Net2ConsoleNotice;
				Net2ConsoleNotice.clear();
				JoinState = JOIN_NOTHING;
				_netresponse = 0;
				rebuild = true;
				menu.Finish(CONSOLE_MENU_BACK);
				return;
			}
			if (Session.Games.Count() != listed || Session.Chat.Count() != chatters) {
				rebuild = true;
				menu.Finish(CONSOLE_MENU_BACK);
			}
		});
		ConsoleMenuResult result = menu.Process();
		focus = menu.Get_Focus();
		if (JoinState == JOIN_CONFIRMED) {
			if (Console_Guest_Screen(notice)) {
				connected = true;
				break;
			}
			Enter_Lobby();
			focus = 1;
			continue;
		}
		if (rebuild) {
			continue;
		}
		if (result != CONSOLE_MENU_ACCEPT) {
			Leave_Lobby();
			break;
		}
		if (action == ACTION_HOST) {
			if (strlen(Session.Handle) < 1) {
				notice = Fetch_String(TXT_NAME_BLANK);
				continue;
			}
			bool unique = true;
			for (int index = 0; index < Session.Games.Count(); index++) {
				if (strcmp(Session.Games[index]->Name, Session.Handle) == 0) unique = false;
			}
			if (!unique) {
				notice = Fetch_String(TXT_GAMENAME_MUSTBE_UNIQUE);
				continue;
			}
			if (Console_Host_Screen(notice)) {
				connected = true;
				break;
			}
			Enter_Lobby();
			focus = 1;
			continue;
		}
		if (action == ACTION_JOIN) {
			notice = Join_Problem(CurGame);
			if (!notice.empty()) {
				continue;
			}
			Session.NetStealth = false;
			Session.Write_MultiPlayer_Settings();
			if (Request_To_Join(CurGame)) {
				JoinState = JOIN_WAIT_CONFIRM;
				join_timer = JOIN_TIMEOUT;
			} else {
				notice = "Could not join " + std::string(Session.Games[CurGame]->Name);
			}
		}
	}

	Net2IsConsole = false;
	if (connected) {
		Session.NetOpen = false;
		Session.Write_MultiPlayer_Settings();
	}
	return(connected);
}
