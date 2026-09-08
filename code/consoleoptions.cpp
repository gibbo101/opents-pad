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
#include "theme.h"
#include "video.h"

#include <algorithm>
#include <string>
#include <vector>

enum {
	VOLUME_STEPS = 10,
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
/// Runs the console-style audio screen. The volumes change as they are stepped and are
/// kept when the player accepts or put back when they back out. In play the screen also
/// offers shuffle, repeat, a track to play with accept on its row, and a stop, which take
/// effect at once as the dialog's do.
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
		menu.Add_Row({"Shuffle", [&]{ return(On_Off(Options.IsScoreShuffle)); },
			[&](int) { Options.Set_Shuffle(!Options.IsScoreShuffle); if (Options.IsScoreShuffle) Options.Set_Repeat(false); }, nullptr});
		menu.Add_Row({"Repeat", [&]{ return(On_Off(Options.IsScoreRepeat)); },
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
				[&](int step) { track = Wrap(track + step, 0, int(tracks.size()) - 1); },
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


/// <summary>
/// Lists what the controller's buttons do, each with its glyph, and notes that the bindings
/// are not final while the in-game scheme is still to come.
/// </summary>
void Console_Controls_Screen(void)
{
	enum { FIRST_Y = 48, PITCH = 24, GLYPH_GAP = 8, NOTE_Y = 336 };
	struct BindingType {
		char const * Action;
		char const * Button;
		int Glyph;			// A PadButtonType, or -1 for none.
	};
	static BindingType const _bindings[] = {
		{"Move", "D-pad or left stick", -1},
		{"Change a value", "Left or Right", -1},
		{"Step by five", "LB or RB with Left or Right", -1},
		{"Accept, select", "A", PAD_BUTTON_ACCEPT},
		{"Back", "B", PAD_BUTTON_BACK},
		{"Start the game", "Start, on a setup screen", PAD_BUTTON_MENU},
		{"Pause menu", "Start, in play", PAD_BUTTON_MENU},
		{"Keyboard delete", "X", PAD_BUTTON_THIRD},
		{"Keyboard space", "Y", PAD_BUTTON_FOURTH},
		{"Switch to controller", "Start and B held together", -1},
	};

	ConsoleMenuClass menu("Controls");
	menu.Set_Prompts("", "Back");
	int y = FIRST_Y;
	for (BindingType const & binding : _bindings) {
		ConsoleRowType row = {binding.Action, [&binding]{ return(std::string(binding.Button)); }, nullptr, nullptr};
		row.Y = y;
		menu.Add_Row(row);
		y += PITCH;
	}
	ConsoleRowType note = {"Controls are not final yet: the in-game scheme is still to come", nullptr, nullptr, nullptr};
	note.Y = NOTE_Y;
	note.Quiet = true;
	menu.Add_Row(note);
	menu.Set_Backdrop_Panel([&](ConsoleCanvas & canvas) {
		int glyph = canvas.LineHeight + 4;
		int row_y = FIRST_Y;
		for (BindingType const & binding : _bindings) {
			if (binding.Glyph >= 0) {
				int x = canvas.Box.X + ConsoleMenuClass::Value_Left() + canvas.Width(binding.Button) + GLYPH_GAP;
				Draw_Pad_Glyph(canvas.Frame, PadButtonType(binding.Glyph), x, canvas.Box.Y + row_y - 2, glyph);
			}
			row_y += PITCH;
		}
	});
	menu.Process();
}


/// <summary>
/// Runs the console-style options screen. Everything applies when the player accepts, and
/// accepting saves the settings file; the Audio row opens the audio screen, which keeps its
/// own changes.
/// </summary>
/// <returns>bool; Did the player accept the settings?</returns>
bool Console_Options_Screen(bool in_game)
{
	// 0 follows the connected controller, 1 is keyboard and mouse, 2 is controller.
	int scheme = Options.ControlSchemeAuto ? 0 : (Options.ControlScheme == CONTROL_CONTROLLER ? 2 : 1);
	int prompts = std::clamp(Options.PromptStyle, 0, int(PROMPT_STYLE_DECK));
	int zoom_width;
	int zoom_height;
	Pad_Zoom_Size(zoom_width, zoom_height);
	bool stretch = Options.StretchMovies;
	int scale_mode = Options.ScaleMode;
	int speed = (OptionsClass::MAX_SPEED_SETTING - 1) - Options.GameSpeed;
	int pointer_speed = Options.PadPointerSpeed;
	int fast_speed = Options.PadFastSpeed;
	int scroll_speed = Options.PadScrollSpeed;
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
		[&](int step) { scheme = Wrap(scheme + step, 0, 2); }, nullptr});
	static char const * const _prompt_names[] = {"Auto", "Text", "Xbox", "PlayStation", "Steam Deck"};
	menu.Add_Row({"Button Prompts", [&]{ return(std::string(_prompt_names[prompts])); },
		[&](int step) { prompts = Wrap(prompts + step, 0, int(PROMPT_STYLE_DECK)); Options.PromptStyle = prompts; }, nullptr});
	// The zoom is a render size on the panel's shape; stepping up the row zooms in, as the
	// stick does. In play the row applies as the menu closes and the play size returns.
	menu.Add_Row({"Zoom", [&]{ char name[48]; Pad_Zoom_Name(zoom_width, zoom_height, name, sizeof(name)); return(std::string(name)); },
		[&](int step) { zoom_height = Pad_Zoom_Neighbour(zoom_height, step); zoom_width = Pad_Zoom_Width(zoom_height); }, nullptr});
	if (!in_game) {
		menu.Add_Row({"Scale Mode", [&]{ return(std::string(_scale_names[std::clamp(scale_mode, 0, 2)])); },
			[&](int step) { scale_mode = Wrap(scale_mode + step, 0, 2); }, nullptr});
		menu.Add_Row({"Stretch Movies", [&]{ return(On_Off(stretch)); }, [&](int) { stretch = !stretch; }, nullptr});
	}
	menu.Add_Row({"Game Speed", [&]{ return(std::to_string(speed)); },
		[&](int step) { speed = std::clamp(speed + step, 0, int(OptionsClass::MAX_SPEED_SETTING) - 1); }, nullptr});
	auto step_speed = [](int & value) { return [&value](int delta) { value = std::clamp(value + delta, int(OptionsClass::PAD_SPEED_MIN), int(OptionsClass::PAD_SPEED_MAX)); }; };
	menu.Add_Row({"Pointer Speed", [&]{ return(std::to_string(pointer_speed)); }, step_speed(pointer_speed), nullptr});
	menu.Add_Row({"Fast Pointer", [&]{ return(std::to_string(fast_speed)); }, step_speed(fast_speed), nullptr});
	menu.Add_Row({"Stick Scroll Speed", [&]{ return(std::to_string(scroll_speed)); }, step_speed(scroll_speed), nullptr});
	menu.Add_Row({"Detail Level", [&]{ return(std::string(_detail_names[std::clamp(detail, 0, 2)])); },
		[&](int step) { detail = std::clamp(detail + step, 0, int(OptionsClass::MAX_DETAIL_SETTING) - 1); }, nullptr});
	// The difficulty is a setting for the next campaign mission, so it is not offered in play.
	if (!in_game) {
		menu.Add_Row({"Campaign Difficulty", [&]{ return(std::string(Fetch_String(_difficulty_names[std::clamp(difficulty, 0, 2)]))); },
			[&](int step) { difficulty = std::clamp(difficulty + step, 0, std::min(int(OptionsClass::MAX_DIFFICULTY_SETTING), 3) - 1); }, nullptr});
	}
	menu.Add_Row({"Sidebar Cameo Text", [&]{ return(On_Off(cameo_text)); }, [&](int) { cameo_text = !cameo_text; }, nullptr});
	menu.Add_Row({"Action Lines", [&]{ return(On_Off(action_lines)); }, [&](int) { action_lines = !action_lines; }, nullptr});
	menu.Add_Row({"Tool Tips", [&]{ return(On_Off(tooltips)); }, [&](int) { tooltips = !tooltips; }, nullptr});
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
	if (scheme == 1 && Options.ControlScheme == CONTROL_CONTROLLER && !Confirm_Keyboard_Mouse()) {
		scheme = Options.ControlSchemeAuto ? 0 : 2;
	}
	Options.ControlSchemeAuto = scheme == 0;
	if (scheme == 0) {
		Options.ControlScheme = Gamepad_Read().Connected ? CONTROL_CONTROLLER : CONTROL_KEYBOARD_MOUSE;
	} else {
		Options.ControlScheme = scheme == 2 ? CONTROL_CONTROLLER : CONTROL_KEYBOARD_MOUSE;
	}
	Options.PadZoomWidth = zoom_width;
	Options.PadZoomHeight = zoom_height;
	Options.ScaleMode = scale_mode;
	Options.StretchMovies = stretch;
	Options.GameSpeed = (OptionsClass::MAX_SPEED_SETTING - 1) - speed;
	Options.PadPointerSpeed = pointer_speed;
	Options.PadFastSpeed = fast_speed;
	Options.PadScrollSpeed = scroll_speed;
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
