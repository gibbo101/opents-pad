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
#include "data.h"
#include "gamepad.h"
#include "globals.h"
#include "goptions.h"
#include "language/language.h"
#include "options.h"
#include "padglyph.h"
#include "techno.h"
#include "video.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

enum {
	VOLUME_STEPS = 10,
	MODE_MIN_WIDTH = 640,
	MODE_MIN_HEIGHT = 400,
	MODE_MAX_WIDTH = 8192,
	MODE_MAX_HEIGHT = 8192,
};


static std::string On_Off(bool value)
{
	return(Fetch_String(value ? TXT_ON : TXT_OFF));
}


static int Wrap(int value, int low, int high)
{
	int span = high - low + 1;
	if (span <= 0) return(low);
	return(low + ((value - low) % span + span) % span);
}


// The display's modes, with the configured size kept in the list even when the display
// does not report it, so the player can always step back to it.
static std::vector<std::pair<int, int>> Display_Modes(int width, int height)
{
	std::vector<std::pair<int, int>> modes;
	int * list = EnumDisplayModes(MODE_MIN_WIDTH, MODE_MIN_HEIGHT, MODE_MAX_WIDTH, MODE_MAX_HEIGHT);
	if (list != NULL) {
		for (int * mode = list; *mode != 0; mode += 2) {
			modes.push_back({mode[0], mode[1]});
		}
		delete [] list;
	}
	if (std::find(modes.begin(), modes.end(), std::make_pair(width, height)) == modes.end()) {
		modes.insert(modes.begin(), {width, height});
	}
	return(modes);
}


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
	ConsoleMenuClass menu("Keyboard & Mouse");
	menu.Set_Prompts("", "Cancel");
	menu.Add_Row({"Press a key or click the mouse to confirm", nullptr, nullptr, nullptr});
	menu.Add_Row({"", [&]{
		int left = std::max(0, int(CONFIRM_SECONDS - (timeGetTime() - started) / 1000));
		return("Reverting in " + std::to_string(left) + " seconds");
	}, nullptr, nullptr});
	menu.Set_Idle([&]{
		if (Keyboard_Mouse_Seen()) {
			confirmed = true;
			menu.Finish(CONSOLE_MENU_ACCEPT);
		} else if (timeGetTime() - started >= CONFIRM_SECONDS * 1000) {
			menu.Finish(CONSOLE_MENU_BACK);
		}
		menu.Refresh();
	});
	menu.Process();
	return(confirmed);
}


/// <summary>
/// Runs the console-style options screen. Sound volumes change as they are stepped so the
/// player hears them; everything else applies when the player accepts, and backing out
/// restores the volumes. Accepting saves the settings file.
/// </summary>
/// <returns>bool; Did the player accept the settings?</returns>
bool Console_Options_Screen(bool in_game)
{
	// 0 follows the connected controller, 1 is keyboard and mouse, 2 is controller.
	int scheme = Options.ControlSchemeAuto ? 0 : (Options.ControlScheme == CONTROL_CONTROLLER ? 2 : 1);
	int prompts = std::clamp(Options.PromptStyle, 0, int(PROMPT_STYLE_DECK));
	std::vector<std::pair<int, int>> modes = Display_Modes(Options.ScreenWidth, Options.ScreenHeight);
	int mode = int(std::find(modes.begin(), modes.end(), std::make_pair(Options.ScreenWidth, Options.ScreenHeight)) - modes.begin());
	bool stretch = Options.StretchMovies;
	int scale_mode = Options.ScaleMode;
	bool integer_scaling = Options.IntegerScaling;
	int speed = (OptionsClass::MAX_SPEED_SETTING - 1) - Options.GameSpeed;
	int scroll = (OptionsClass::MAX_SCROLL_SETTING - 1) - Options.ScrollRate;
	int detail = Options.DetailLevel;
	int difficulty = Options.Difficulty;
	bool cameo_text = Options.SidebarCameoText;
	bool action_lines = Options.ActionLines;
	bool tooltips = Options.ToolTips;
	bool coasting = Options.ScrollMethod == 0;
	float const old_score = Options.ScoreVolume;
	float const old_sound = Options.SoundVolume;
	float const old_voice = Options.VoiceVolume;
	int music = Volume_Steps(old_score);
	int sound = Volume_Steps(old_sound);
	int voice = Volume_Steps(old_voice);

	static char const * const _scale_names[] = {"Nearest", "Linear", "Pixel Art"};
	static char const * const _detail_names[] = {"Low", "Medium", "High"};
	static int const _difficulty_names[] = {TXT_EASY, TXT_NORMAL, TXT_HARD};

	ConsoleMenuClass menu("Options");
	menu.Set_Prompts("Accept", "Back");

	static char const * const _scheme_names[] = {"Auto", "Keyboard & Mouse", "Controller"};
	menu.Add_Row({"Control Scheme", [&]{ return(std::string(_scheme_names[scheme])); },
		[&](int step) { scheme = Wrap(scheme + step, 0, 2); }, nullptr});
	static char const * const _prompt_names[] = {"Auto", "Text", "Xbox", "PlayStation", "Steam Deck"};
	menu.Add_Row({"Button Prompts", [&]{ return(std::string(_prompt_names[prompts])); },
		[&](int step) { prompts = Wrap(prompts + step, 0, int(PROMPT_STYLE_DECK)); Options.PromptStyle = prompts; }, nullptr});
	if (!in_game) {
		menu.Add_Row({"Resolution", [&]{ return(std::to_string(modes[mode].first) + " x " + std::to_string(modes[mode].second)); },
			[&](int step) { mode = Wrap(mode + step, 0, int(modes.size()) - 1); }, nullptr});
		menu.Add_Row({"Scale Mode", [&]{ return(std::string(_scale_names[std::clamp(scale_mode, 0, 2)])); },
			[&](int step) { scale_mode = Wrap(scale_mode + step, 0, 2); }, nullptr});
		menu.Add_Row({"Integer Scaling", [&]{ return(On_Off(integer_scaling)); }, [&](int) { integer_scaling = !integer_scaling; }, nullptr});
		menu.Add_Row({"Stretch Movies", [&]{ return(On_Off(stretch)); }, [&](int) { stretch = !stretch; }, nullptr});
	}
	menu.Add_Row({"Game Speed", [&]{ return(std::to_string(speed)); },
		[&](int step) { speed = std::clamp(speed + step, 0, int(OptionsClass::MAX_SPEED_SETTING) - 1); }, nullptr});
	menu.Add_Row({"Scroll Rate", [&]{ return(std::to_string(scroll)); },
		[&](int step) { scroll = std::clamp(scroll + step, 0, int(OptionsClass::MAX_SCROLL_SETTING) - 1); }, nullptr});
	menu.Add_Row({"Scroll Coasting", [&]{ return(On_Off(coasting)); }, [&](int) { coasting = !coasting; }, nullptr});
	menu.Add_Row({"Detail Level", [&]{ return(std::string(_detail_names[std::clamp(detail, 0, 2)])); },
		[&](int step) { detail = std::clamp(detail + step, 0, int(OptionsClass::MAX_DETAIL_SETTING) - 1); }, nullptr});
	menu.Add_Row({"Campaign Difficulty", [&]{ return(std::string(Fetch_String(_difficulty_names[std::clamp(difficulty, 0, 2)]))); },
		[&](int step) { difficulty = std::clamp(difficulty + step, 0, std::min(int(OptionsClass::MAX_DIFFICULTY_SETTING), 3) - 1); }, nullptr});
	menu.Add_Row({"Sidebar Cameo Text", [&]{ return(On_Off(cameo_text)); }, [&](int) { cameo_text = !cameo_text; }, nullptr});
	menu.Add_Row({"Action Lines", [&]{ return(On_Off(action_lines)); }, [&](int) { action_lines = !action_lines; }, nullptr});
	menu.Add_Row({"Tool Tips", [&]{ return(On_Off(tooltips)); }, [&](int) { tooltips = !tooltips; }, nullptr});
	menu.Add_Row({"Music Volume", [&]{ return(std::to_string(music)); },
		[&](int step) { music = std::clamp(music + step, 0, int(VOLUME_STEPS)); Options.Set_Score_Volume(music / float(VOLUME_STEPS), true); }, nullptr});
	menu.Add_Row({"Sound Volume", [&]{ return(std::to_string(sound)); },
		[&](int step) { sound = std::clamp(sound + step, 0, int(VOLUME_STEPS)); Options.Set_Sound_Volume(sound / float(VOLUME_STEPS), true); }, nullptr});
	menu.Add_Row({"Voice Volume", [&]{ return(std::to_string(voice)); },
		[&](int step) { voice = std::clamp(voice + step, 0, int(VOLUME_STEPS)); Options.Set_Voice_Volume(voice / float(VOLUME_STEPS), true); }, nullptr});

	int const old_prompts = Options.PromptStyle;
	bool accepted = menu.Process() == CONSOLE_MENU_ACCEPT;

	if (!accepted) {
		Options.PromptStyle = old_prompts;
		Options.Set_Score_Volume(old_score, false);
		Options.Set_Sound_Volume(old_sound, false);
		Options.Set_Voice_Volume(old_voice, false);
		return(false);
	}

	// Leaving the controller scheme for the keyboard one is confirmed with a real key or
	// click first, so a pad-only player cannot lock themselves out.
	if (scheme == 1 && Options.ControlScheme == CONTROL_CONTROLLER && !Confirm_Keyboard_Mouse()) {
		scheme = Options.ControlSchemeAuto ? 0 : 2;
	}
	Options.ControlSchemeAuto = scheme == 0;
	if (scheme == 0) {
		Options.ControlScheme = Gamepad_Read().Connected ? CONTROL_CONTROLLER : CONTROL_KEYBOARD_MOUSE;
	} else {
		Options.ControlScheme = scheme == 2 ? CONTROL_CONTROLLER : CONTROL_KEYBOARD_MOUSE;
	}
	Options.ScreenWidth = modes[mode].first;
	Options.ScreenHeight = modes[mode].second;
	Options.ScaleMode = scale_mode;
	Options.IntegerScaling = integer_scaling;
	Options.StretchMovies = stretch;
	Options.GameSpeed = (OptionsClass::MAX_SPEED_SETTING - 1) - speed;
	Options.ScrollRate = (OptionsClass::MAX_SCROLL_SETTING - 1) - scroll;
	Options.ScrollMethod = coasting ? 0 : 1;
	if (Options.DetailLevel != detail) {
		Options.DetailLevel = detail;
		Map.Reinit_Cell_Drawers();
	}
	Options.Difficulty = difficulty;
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
