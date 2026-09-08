/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "mainopt.h"

#include "_map.h"
#include "consolemenu.h"
#include "consolemp.h"
#include "data.h"
#include "dsurface.h"
#include "event.h"
#include "gamepad.h"
#include "globals.h"
#include "goptions.h"
#include "house.h"
#include "language/language.h"
#include "options.h"
#include "padglyph.h"
#include "session.h"
#include "techno.h"
#include "theme.h"
#include "video.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

enum {
	VOLUME_STEPS = 10,
};


static int Volume_Steps(float volume)
{
	return(std::clamp(int(volume * VOLUME_STEPS + 0.5f), 0, int(VOLUME_STEPS)));
}


// Asks for a key press or a mouse click within a few seconds and reports whether one came.
static bool Confirm_Keyboard_Mouse(void)
{
	enum { CONFIRM_SECONDS = 10 };
	Note_Keyboard_Mouse_Reset();
	unsigned long started = timeGetTime();
	bool confirmed = false;
	auto seconds_left = [&](void) {
		return(std::max(0, int(CONFIRM_SECONDS - (timeGetTime() - started) / 1000)));
	};
	int shown = -1;
	ConsoleMenuClass menu("Keyboard & Mouse");
	menu.Set_Prompts("", "Cancel");
	menu.Add_Row({"Press a key or click the mouse to confirm", nullptr, nullptr, nullptr});
	menu.Add_Row({"", [&]{ return("Reverting in " + std::to_string(seconds_left()) + " seconds"); }, nullptr, nullptr});
	menu.Set_Idle([&]{
		if (Keyboard_Mouse_Seen()) {
			confirmed = true;
			menu.Finish(CONSOLE_MENU_ACCEPT);
		} else if (timeGetTime() - started >= CONFIRM_SECONDS * 1000) {
			menu.Finish(CONSOLE_MENU_BACK);
		}
		if (seconds_left() != shown) {
			shown = seconds_left();
			menu.Refresh();
		}
	});
	menu.Process();
	return(confirmed);
}


/// <summary>
/// Runs the audio screen. Volumes apply as stepped and are kept on accept or restored on
/// back; in play the track rows act at once.
/// </summary>
/// <param name="in_game">Is a scenario in progress?</param>
/// <returns>bool; Did the player accept the volumes?</returns>
bool Console_Audio_Screen(bool in_game)
{
	float const old_score = Options.ScoreVolume;
	float const old_sound = Options.SoundVolume;
	float const old_voice = Options.VoiceVolume;
	int music = Volume_Steps(old_score);
	int sound = Volume_Steps(old_sound);
	int voice = Volume_Steps(old_voice);

	std::vector<ThemeType> tracks;
	int track = 0;
	for (ThemeType index = THEME_FIRST; index < Theme.Max_Themes(); index = ThemeType(index + 1)) {
		if (Theme.Is_Allowed(index)) {
			if (index == Theme.What_Is_Playing()) track = int(tracks.size());
			tracks.push_back(index);
		}
	}

	ConsoleMenuClass menu("Audio");
	menu.Set_Prompts("Accept", "Back");
	menu.Add_Row({"Music Volume", [&]{ return(std::to_string(music)); },
		[&](int step) { music = std::clamp(music + step, 0, int(VOLUME_STEPS)); Options.Set_Score_Volume(music / float(VOLUME_STEPS), true); }, nullptr});
	menu.Add_Row({"Sound Volume", [&]{ return(std::to_string(sound)); },
		[&](int step) { sound = std::clamp(sound + step, 0, int(VOLUME_STEPS)); Options.Set_Sound_Volume(sound / float(VOLUME_STEPS), true); }, nullptr});
	menu.Add_Row({"Voice Volume", [&]{ return(std::to_string(voice)); },
		[&](int step) { voice = std::clamp(voice + step, 0, int(VOLUME_STEPS)); Options.Set_Voice_Volume(voice / float(VOLUME_STEPS), true); }, nullptr});
	if (in_game) {
		menu.Add_Row({"Shuffle", [&]{ return(Console_On_Off(Options.IsScoreShuffle)); },
			[&](int) { Options.Set_Shuffle(!Options.IsScoreShuffle); if (Options.IsScoreShuffle) Options.Set_Repeat(false); }, nullptr});
		menu.Add_Row({"Repeat", [&]{ return(Console_On_Off(Options.IsScoreRepeat)); },
			[&](int) { Options.Set_Repeat(!Options.IsScoreRepeat); if (Options.IsScoreRepeat) Options.Set_Shuffle(false); }, nullptr});
		if (!tracks.empty()) {
			menu.Add_Row({"Now Playing", [&]{
					ThemeType playing = Theme.What_Is_Playing();
					return(std::string(Theme.Is_Allowed(playing) ? Theme.Full_Name(playing) : "None"));
				}, nullptr, nullptr});
			menu.Add_Row({"Track", [&]{
					ThemeType theme = tracks[track];
					int length = Theme.Track_Length(theme);
					char buffer[128];
					snprintf(buffer, sizeof(buffer), "%02d - %s [%d:%02d]", track + 1, Theme.Full_Name(theme), length / 60, length % 60);
					return(std::string(buffer));
				},
				[&](int step) { track = Console_Wrap(track + step, 0, int(tracks.size()) - 1); },
				[&]{ Theme.Stop(); Theme.Queue_Song(tracks[track]); menu.Refresh(); }});
			menu.Set_Row_Prompt(int(menu.Row_Count()) - 1, "Play");
			menu.Add_Row({"Stop Music", nullptr, nullptr, [&]{ Theme.Queue_Song(THEME_QUIET); menu.Refresh(); }});
		}
	}

	bool accepted = menu.Process() == CONSOLE_MENU_ACCEPT;
	if (!accepted) {
		Options.Set_Score_Volume(old_score, false);
		Options.Set_Sound_Volume(old_sound, false);
		Options.Set_Voice_Volume(old_voice, false);
		return(false);
	}
	Options.Save_Settings();
	return(true);
}


// The pad drawn on the Controls screen, in the units of docs/controller-layout.svg with the
// pad's own origin, scaled to fit beside the labels.
namespace {

struct PadSpotType
{
	int X;
	int Y;
};

enum PadPartType {
	PAD_PART_FOURTH,
	PAD_PART_BACK,
	PAD_PART_THIRD,
	PAD_PART_ACCEPT,
	PAD_PART_L2,
	PAD_PART_L1,
	PAD_PART_R2,
	PAD_PART_R1,
	PAD_PART_DPAD,
	PAD_PART_LEFT_STICK,
	PAD_PART_RIGHT_STICK,
	PAD_PART_VIEW,
	PAD_PART_MENU,
};

// Where each part sits, in the drawing's units.
PadSpotType const _PadSpots[] = {
	{300, 88}, {335, 120}, {265, 120}, {300, 152},
	{60, 10}, {60, 35}, {300, 10}, {300, 35},
	{60, 120}, {120, 185}, {240, 185},
	{154, 104}, {206, 104},
};

// The face buttons in the drawing's order, and their names when there is no glyph to draw.
PadButtonType const _FaceButtons[4] = {PAD_BUTTON_FOURTH, PAD_BUTTON_BACK, PAD_BUTTON_THIRD, PAD_BUTTON_ACCEPT};
char const * const _FaceNames[4] = {"Y", "B", "X", "A"};

struct PadCalloutType
{
	PadPartType Part;
	char const * Name[4];		// By prompt style: text, Xbox, PlayStation, Deck; NULL draws the face glyph.
	char const * Line1;
	char const * Line2;
};

struct PadPageType
{
	char const * Title;
	PadCalloutType const * Callouts;
	int Count;
};

PadCalloutType const _FacePage[] = {
	{PAD_PART_FOURTH, {NULL, NULL, NULL, NULL}, "Sidebar in and out", "With R1: keep it in"},
	{PAD_PART_BACK, {NULL, NULL, NULL, NULL}, "Cancel, deselect", "Sidebar: hold, cancel, back"},
	{PAD_PART_ACCEPT, {NULL, NULL, NULL, NULL}, "Select, order, place", "Hold still: all of a type"},
	{PAD_PART_THIRD, {NULL, NULL, NULL, NULL}, "Cycle repair, sell,", "power and waypoint"},
};
PadCalloutType const _ShoulderPage[] = {
	{PAD_PART_L2, {"LT", "LT", "L2", "L2"}, "Shape: make team 1 to 4", "With R1: force move"},
	{PAD_PART_L1, {"LB", "LB", "L1", "L1"}, "Shape: select team 1 to 4", "With R1: force fire"},
	{PAD_PART_R2, {"RT", "RT", "R2", "R2"}, "Scatter. With R1: guard", "Waypoints: undo the last"},
	{PAD_PART_R1, {"RB", "RB", "R1", "R1"}, "Hold: fast pointer", "With B: build again"},
};
PadCalloutType const _StickPage[] = {
	{PAD_PART_DPAD, {"D-pad", "D-pad", "D-pad", "D-pad"}, "Move the pointer", "Sidebar: move the focus"},
	{PAD_PART_LEFT_STICK, {"Left stick", "Left stick", "Left stick", "Left stick"}, "Move the pointer", "Click: deploy"},
	{PAD_PART_VIEW, {"View", "View", "Share", "View"}, "Ally with the owner", "of the selected unit"},
	{PAD_PART_RIGHT_STICK, {"Right stick", "Right stick", "Right stick", "Right stick"}, "Scroll the map", "With R1: zoom. Click: base"},
	{PAD_PART_MENU, {"Menu", "Menu", "Options", "Menu"}, "Pause menu", "Setup screens: keyboard"},
};
PadPageType const _PadPages[] = {
	{"Face buttons", _FacePage, 4},
	{"Shoulders", _ShoulderPage, 4},
	{"Sticks and menu buttons", _StickPage, 5},
};
enum { PAD_PAGE_COUNT = 3 };


int Pixel_Of(int red, int green, int blue)
{
	return(DSurface::Build_Hicolor_Pixel(red, green, blue));
}


void Fill_Ellipse(Surface & surface, int cx, int cy, int rx, int ry, int color)
{
	for (int dy = -ry; dy <= ry; dy++) {
		double share = 1.0 - (double(dy) * dy) / (double(ry) * ry);
		int half = int(rx * sqrt(share < 0.0 ? 0.0 : share));
		surface.Fill_Rect(Rect(cx - half, cy + dy, half * 2 + 1, 1), color);
	}
}


void Fill_Pill(Surface & surface, Rect const & box, int color)
{
	int radius = box.Height / 2;
	surface.Fill_Rect(Rect(box.X + radius, box.Y, box.Width - radius * 2, box.Height), color);
	Fill_Ellipse(surface, box.X + radius, box.Y + radius, radius, radius, color);
	Fill_Ellipse(surface, box.X + box.Width - radius - 1, box.Y + radius, radius, radius, color);
}


Point2D Pad_Point(int x, int y, double scale, PadPartType part)
{
	return(Point2D(x + int(_PadSpots[part].X * scale), y + int(_PadSpots[part].Y * scale)));
}


// Draws the pad with its origin at x, y, in the drawing's units times scale.
void Draw_Pad(Surface & surface, int x, int y, double scale, int glyph)
{
	int body = Pixel_Of(84, 84, 82);
	int shoulder = Pixel_Of(64, 64, 62);
	int part = Pixel_Of(120, 120, 116);
	int cap = Pixel_Of(150, 150, 146);
	auto at = [&](int px, int py) { return(Point2D(x + int(px * scale), y + int(py * scale))); };
	auto size = [&](int value) { return(std::max(1, int(value * scale))); };

	Fill_Pill(surface, Rect(at(15, 0).X, at(15, 0).Y, size(90), size(20)), shoulder);
	Fill_Pill(surface, Rect(at(15, 24).X, at(15, 24).Y, size(90), size(22)), shoulder);
	Fill_Pill(surface, Rect(at(255, 0).X, at(255, 0).Y, size(90), size(20)), shoulder);
	Fill_Pill(surface, Rect(at(255, 24).X, at(255, 24).Y, size(90), size(22)), shoulder);
	Fill_Pill(surface, Rect(at(0, 50).X, at(0, 50).Y, size(360), size(170)), body);
	Fill_Ellipse(surface, at(45, 230).X, at(45, 230).Y, size(55), size(85), body);
	Fill_Ellipse(surface, at(315, 230).X, at(315, 230).Y, size(55), size(85), body);
	// The d-pad.
	surface.Fill_Rect(Rect(at(48, 85).X, at(48, 85).Y, size(24), size(70)), part);
	surface.Fill_Rect(Rect(at(25, 108).X, at(25, 108).Y, size(70), size(24)), part);
	// The sticks.
	Fill_Ellipse(surface, at(120, 185).X, at(120, 185).Y, size(26), size(26), part);
	Fill_Ellipse(surface, at(240, 185).X, at(240, 185).Y, size(26), size(26), part);
	Fill_Ellipse(surface, at(120, 185).X, at(120, 185).Y, size(16), size(16), cap);
	Fill_Ellipse(surface, at(240, 185).X, at(240, 185).Y, size(16), size(16), cap);
	// View and menu.
	surface.Fill_Rect(Rect(at(142, 98).X, at(142, 98).Y, size(24), size(12)), part);
	surface.Fill_Rect(Rect(at(194, 98).X, at(194, 98).Y, size(24), size(12)), part);
	// The face buttons carry the player's glyphs.
	for (int index = 0; index < 4; index++) {
		Point2D centre = Pad_Point(x, y, scale, PadPartType(PAD_PART_FOURTH + index));
		Fill_Ellipse(surface, centre.X, centre.Y, size(14), size(14), cap);
		// Plain text prompts draw no glyph; the callout names the button instead.
		Draw_Pad_Glyph(surface, _FaceButtons[index], centre.X - glyph / 2, centre.Y - glyph / 2, glyph);
	}
}

}  // namespace


/// <summary>
/// Runs the Controls screen: the pad drawn beside a page of its buttons, each ringed and
/// joined to what it does, with left and right stepping through the pages.
/// </summary>
void Console_Controls_Screen(void)
{
	// The pad sits mid-screen; parts on its left half are labelled to the left and the rest
	// to the right, so every line is short and none crosses the pad or another line.
	enum { PAGE_Y = 44, PAD_Y = 110, PAD_WIDTH = 222, COLUMN_GAP = 14, LABEL_TOP = 96, LABEL_GAP = 10, RING = 3 };
	const double PAD_SCALE = PAD_WIDTH / 360.0;
	int page = 0;

	ConsoleMenuClass menu("Controls");
	menu.Set_Prompts("", "Back");
	ConsoleRowType row = {"Page", [&]{ return(std::string(_PadPages[page].Title) + "  " + std::to_string(page + 1) + "/" + std::to_string(int(PAD_PAGE_COUNT))); },
		[&](int step) { page = Console_Wrap(page + step, 0, PAD_PAGE_COUNT - 1); }, nullptr};
	row.Y = PAGE_Y;
	menu.Add_Row(row);
	menu.Set_Backdrop_Panel([&](ConsoleCanvas & canvas) {
		int glyph = canvas.LineHeight + 2;
		int x = canvas.Box.X + (canvas.Box.Width - PAD_WIDTH) / 2;
		int y = canvas.Box.Y + PAD_Y;
		Draw_Pad(canvas.Frame, x, y, PAD_SCALE, glyph);

		int yellow = Pixel_Of(248, 216, 48);
		RGBClass gold(248, 216, 48);
		PadPageType const & shown = _PadPages[page];
		// Which of a callout's names each prompt style shows.
		static int const _name_by_style[] = {0, 0, 1, 2, 3};
		int name_index = _name_by_style[std::clamp(Resolved_Prompt_Style(), 0, int(PROMPT_STYLE_DECK))];
		int pitch = canvas.LineHeight * 3 + LABEL_GAP;
		int radius = int(16 * PAD_SCALE) + RING;
		int next_y[2] = {canvas.Box.Y + LABEL_TOP, canvas.Box.Y + LABEL_TOP};
		for (int index = 0; index < shown.Count; index++) {
			PadCalloutType const & callout = shown.Callouts[index];
			Point2D centre = Pad_Point(x, y, PAD_SCALE, callout.Part);
			bool right = _PadSpots[callout.Part].X >= 180;
			// A label sits level with its part when the column has room, else below the last.
			int label_y = std::max(next_y[right], centre.Y - canvas.LineHeight - canvas.LineHeight / 2);
			next_y[right] = label_y + pitch;
			int column_x = right ? x + PAD_WIDTH + COLUMN_GAP : canvas.Box.X + 12;
			int column_edge = right ? column_x - 6 : x - COLUMN_GAP + 6;

			canvas.Frame.Draw_Ellipse(centre, radius, radius, canvas.Box, yellow);
			// From the ring, level to the column's edge, up or down to the label, then in.
			int line_y = label_y + canvas.LineHeight / 2;
			Point2D from(centre.X + (right ? radius + 1 : -radius - 1), centre.Y);
			canvas.Frame.Draw_Line(from, Point2D(column_edge, from.Y), yellow);
			canvas.Frame.Draw_Line(Point2D(column_edge, from.Y), Point2D(column_edge, line_y), yellow);
			canvas.Frame.Draw_Line(Point2D(column_edge, line_y), Point2D(column_edge + (right ? 4 : -4), line_y), yellow);

			if (callout.Name[name_index] != NULL) {
				canvas.PrintColor(callout.Name[name_index], column_x, label_y, gold);
			} else {
				if (Draw_Pad_Glyph(canvas.Frame, _FaceButtons[callout.Part - PAD_PART_FOURTH], column_x, label_y - 1, glyph) == 0) {
					canvas.PrintColor(_FaceNames[callout.Part - PAD_PART_FOURTH], column_x, label_y, gold);
				}
			}
			canvas.Print(callout.Line1, column_x, label_y + canvas.LineHeight, false);
			if (callout.Line2[0] != '\0') {
				canvas.Print(callout.Line2, column_x, label_y + canvas.LineHeight * 2, false);
			}
		}
	});
	menu.Process();
}


/// <summary>
/// Runs the options screen. Everything applies and saves on accept; the Audio row's screen
/// keeps its own changes.
/// </summary>
/// <returns>bool; Did the player accept the settings?</returns>
bool Console_Options_Screen(bool in_game)
{
	enum { SCHEME_AUTO, SCHEME_KEYBOARD, SCHEME_CONTROLLER };
	int scheme = Options.ControlSchemeAuto ? SCHEME_AUTO : (Options.ControlScheme == CONTROL_CONTROLLER ? SCHEME_CONTROLLER : SCHEME_KEYBOARD);
	int prompts = std::clamp(Options.PromptStyle, 0, int(PROMPT_STYLE_DECK));
	int zoom_width;
	int zoom_height;
	Pad_Zoom_Settle(zoom_width, zoom_height);
	bool stretch = Options.StretchMovies;
	int scale_mode = Options.ScaleMode;
	int speed = (OptionsClass::MAX_SPEED_SETTING - 1) - Options.GameSpeed;
	int pointer_speed = Options.PadPointerSpeed;
	int fast_speed = Options.PadFastSpeed;
	int scroll_speed = Options.PadScrollSpeed;
	int snap = Options.PadSnap;
	int detail = Options.DetailLevel;
	int difficulty = Options.Difficulty;
	bool cameo_text = Options.SidebarCameoText;
	bool action_lines = Options.ActionLines;
	bool tooltips = Options.ToolTips;

	static char const * const _scale_names[] = {"Nearest", "Linear", "Pixel Art"};
	static char const * const _detail_names[] = {"Low", "Medium", "High"};
	static int const _difficulty_names[] = {TXT_EASY, TXT_NORMAL, TXT_HARD};

	ConsoleMenuClass menu("Options");
	menu.Set_Prompts("Accept", "Back");

	static char const * const _scheme_names[] = {"Auto", "Keyboard & Mouse", "Controller"};
	menu.Add_Row({"Control Scheme", [&]{ return(std::string(_scheme_names[scheme])); },
		[&](int step) { scheme = Console_Wrap(scheme + step, SCHEME_AUTO, SCHEME_CONTROLLER); }, nullptr});
	static char const * const _prompt_names[] = {"Auto", "Text", "Xbox", "PlayStation", "Steam Deck"};
	menu.Add_Row({"Button Prompts", [&]{ return(std::string(_prompt_names[prompts])); },
		[&](int step) { prompts = Console_Wrap(prompts + step, 0, int(PROMPT_STYLE_DECK)); Options.PromptStyle = prompts; }, nullptr});
	// The zoom is a render size on the panel's shape; stepping up the row zooms in, as the
	// stick does. In play the row applies as the menu closes and the play size returns.
	menu.Add_Row({"Zoom", [&]{ char name[48]; Pad_Zoom_Name(zoom_width, zoom_height, name, sizeof(name)); return(std::string(name)); },
		[&](int step) { zoom_height = Pad_Zoom_Neighbour(zoom_height, step); zoom_width = Pad_Zoom_Width(zoom_height); }, nullptr});
	if (!in_game) {
		menu.Add_Row({"Scale Mode", [&]{ return(std::string(_scale_names[std::clamp(scale_mode, 0, 2)])); },
			[&](int step) { scale_mode = Console_Wrap(scale_mode + step, 0, 2); }, nullptr});
		menu.Add_Row({"Stretch Movies", [&]{ return(Console_On_Off(stretch)); }, [&](int) { stretch = !stretch; }, nullptr});
	}
	menu.Add_Row({"Game Speed", [&]{ return(std::to_string(speed)); },
		[&](int step) { speed = std::clamp(speed + step, 0, int(OptionsClass::MAX_SPEED_SETTING) - 1); }, nullptr});
	auto step_speed = [](int & value) { return [&value](int delta) { value = std::clamp(value + delta, int(OptionsClass::PAD_SPEED_MIN), int(OptionsClass::PAD_SPEED_MAX)); }; };
	menu.Add_Row({"Pointer Speed", [&]{ return(std::to_string(pointer_speed)); }, step_speed(pointer_speed), nullptr});
	menu.Add_Row({"Fast Pointer", [&]{ return(std::to_string(fast_speed)); }, step_speed(fast_speed), nullptr});
	menu.Add_Row({"Stick Scroll Speed", [&]{ return(std::to_string(scroll_speed)); }, step_speed(scroll_speed), nullptr});
	menu.Add_Row({"Unit Snap", [&]{ return(snap == 0 ? std::string("Off") : std::to_string(snap)); },
		[&](int step) { snap = std::clamp(snap + step, 0, int(OptionsClass::PAD_SNAP_MAX)); }, nullptr});
	menu.Add_Row({"Detail Level", [&]{ return(std::string(_detail_names[std::clamp(detail, 0, 2)])); },
		[&](int step) { detail = std::clamp(detail + step, 0, int(OptionsClass::MAX_DETAIL_SETTING) - 1); }, nullptr});
	// The difficulty is a setting for the next campaign mission, so it is not offered in play.
	if (!in_game) {
		menu.Add_Row({"Campaign Difficulty", [&]{ return(std::string(Fetch_String(_difficulty_names[std::clamp(difficulty, 0, 2)]))); },
			[&](int step) { difficulty = std::clamp(difficulty + step, 0, 2); }, nullptr});
	}
	menu.Add_Row({"Sidebar Cameo Text", [&]{ return(Console_On_Off(cameo_text)); }, [&](int) { cameo_text = !cameo_text; }, nullptr});
	menu.Add_Row({"Action Lines", [&]{ return(Console_On_Off(action_lines)); }, [&](int) { action_lines = !action_lines; }, nullptr});
	menu.Add_Row({"Tool Tips", [&]{ return(Console_On_Off(tooltips)); }, [&](int) { tooltips = !tooltips; }, nullptr});
	int audio_row = menu.Add_Row({"Audio", nullptr, nullptr, [&]{ Console_Audio_Screen(in_game); menu.Refresh(); }});
	menu.Set_Row_Prompt(audio_row, "Open");
	int controls_row = menu.Add_Row({"Controls", nullptr, nullptr, [&]{ Console_Controls_Screen(); menu.Refresh(); }});
	menu.Set_Row_Prompt(controls_row, "Open");

	int const old_prompts = Options.PromptStyle;
	bool accepted = menu.Process() == CONSOLE_MENU_ACCEPT;

	if (!accepted) {
		Options.PromptStyle = old_prompts;
		return(false);
	}

	// Leaving the controller scheme for the keyboard one is confirmed with a real key or
	// click first, so a pad-only player cannot lock themselves out.
	if (scheme == SCHEME_KEYBOARD && Options.ControlScheme == CONTROL_CONTROLLER && !Confirm_Keyboard_Mouse()) {
		scheme = Options.ControlSchemeAuto ? SCHEME_AUTO : SCHEME_CONTROLLER;
	}
	Options.ControlSchemeAuto = scheme == SCHEME_AUTO;
	if (scheme == SCHEME_AUTO) {
		Options.ControlScheme = Gamepad_Read().Connected ? CONTROL_CONTROLLER : CONTROL_KEYBOARD_MOUSE;
	} else {
		Options.ControlScheme = scheme == SCHEME_CONTROLLER ? CONTROL_CONTROLLER : CONTROL_KEYBOARD_MOUSE;
	}
	Options.PadZoomWidth = zoom_width;
	Options.PadZoomHeight = zoom_height;
	Options.ScaleMode = scale_mode;
	Options.StretchMovies = stretch;
	// In a network game the speed is an event, so every player changes together.
	int game_speed = (OptionsClass::MAX_SPEED_SETTING - 1) - speed;
	if (Options.GameSpeed != game_speed) {
		if (GameActive && Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH) {
			OutList.push_back(EventClass(PlayerPtr->HeapID, EventClass::GAMESPEED, game_speed));
		} else {
			Options.GameSpeed = game_speed;
		}
	}
	Options.PadPointerSpeed = pointer_speed;
	Options.PadFastSpeed = fast_speed;
	Options.PadScrollSpeed = scroll_speed;
	Options.PadSnap = snap;
	if (Options.DetailLevel != detail) {
		Options.DetailLevel = detail;
		Map.Reinit_Cell_Drawers();
	}
	if (!in_game) {
		Options.Difficulty = difficulty;
	}
	if (Options.SidebarCameoText != cameo_text) {
		Options.SidebarCameoText = cameo_text;
		Map.Toggle_Cameo_Text(cameo_text);
	}
	Options.ActionLines = action_lines;
	TechnoClass::Set_Action_Lines(action_lines);
	Options.ToolTips = tooltips;
	Options.Save_Settings();
	return(true);
}
