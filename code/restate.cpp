/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

#include "always.h"

#include "_keyboar.h"
#include "_palette.h"
#include "_rules.h"
#include "_surface.h"
#include "_xmouse.h"
#include "addon.h"
#include "ccfile.h"
#include "consolemenu.h"
#include "convert.h"
#include "data.h"
#include "dbgprint.h"
#include "gamepad.h"
#include "globals.h"
#include "goptions.h"
#include "keyboard.h"
#include "language/language.h"
#include "mainopt.h"
#include "misc.h"
#include "movie.h"
#include "msanim.h"
#include "msengine.h"
#include "msfont.h"
#include "options.h"
#include "ownrdraw.h"
#include "padglyph.h"
#include "rules.h"
#include "scenario.h"
#include "srfcache.h"
#include "surface.h"
#include "textbtn.h"
#include "theme.h"
#include "voc.h"
#include "winstub.h"

#include "dialog.hh"

#include <algorithm>
#include <vector>

class MyButton : public TextButtonClass {
	public:
		MyButton(MSEngine *engine, unsigned id, int text, TextPrintType style, int x, int y, int w=-1, int h=-1, bool black_border=false, bool no_background=false) :
			TextButtonClass(id, text, style, x, y, w, h, black_border, no_background),
			Engine(engine)
		{
		}
		virtual ~MyButton(void) {}

		virtual int Draw_Me(int forced=false)
		{
			if (!IsDisabled) {
				Draw_Background();
				Draw_Text(String);
				if (IsPressed && !WasPressed) {
					Sound_Effect(Rule->GenericClick);
				}
				WasPressed = IsPressed;
				Rect r(X, Y, Width + 1, Height + 1);
				Engine->Add_Update_Rect(r);
				return(true);
			}
			return(false);
		}

		/// <summary>
		/// Restores the screen behind the button.
		/// This routine paints the untouched backdrop back over the button and marks the area
		/// for update. It is how the "more" button vanishes once the player is done with it.
		/// </summary>
		void Draw(void)
		{
			Rect rect(X, Y, Width + 1, Height + 1);
			HiddenSurface->Blit_From(rect, *AlternateSurface, rect);
			Engine->Add_Update_Rect(rect);
		}

	protected:
		/// <summary>
		/// Draws the face of the button.
		/// This routine builds the button face out of the left cap, stretched middle, and right
		/// cap images kept in the surface cache, picking the artwork that suits the height of the
		/// button and whether it is currently pressed.
		/// </summary>
		virtual void Draw_Background(void)
		{
			Rect origin;
			origin.X = X;
			origin.Y = Y;
			origin.Width = Width;
			Rect dest_rect;
			Rect source_rect;

			unsigned int size_index = 0;
			int small_widths[2] = {7, 7};
			int widths[2] = {10, 10};

			origin.Height = Height;
			int heights[2] = {24, 30};

			for (unsigned int i = 0; i < 2; i++) {
				if (heights[i] > origin.Height && i != 0) {
					break;
				}
				size_index = i;
			}

			int height = heights[size_index];
			int small_width = small_widths[size_index];
			int width = widths[size_index];
			origin.Y += (origin.Height - height) / 2;
			Rect rect;
			char buffer[40];

			sprintf(buffer, "b%ce_li%d.pcx", IsPressed != false ? 'd' : 'u', height);
			Surface * image = SurfaceCache.GetSurface(buffer);
			origin.Height = image->Get_Height();
			dest_rect = origin;
			dest_rect.Width = small_width;
			source_rect.Width = small_width;
			source_rect.Y = 0;
			source_rect.X = 0;
			dest_rect.Height = height;
			source_rect.Height = height;
			HiddenSurface->Blit_From(dest_rect, *image, source_rect);

			sprintf(buffer, "b%ce_mi%d.pcx", IsPressed != false ? 'd' : 'u', height);
			image = SurfaceCache.GetSurface(buffer);
			rect = origin;
			rect.X += small_width;
			rect.Width -= width;
			rect.Height = image->Get_Height();
			SurfaceCache.Draw(rect, *HiddenSurface, *image, 0, 0);

			sprintf(buffer, "b%ce_ri%d.pcx", IsPressed != false ? 'd' : 'u', height);
			image = SurfaceCache.GetSurface(buffer);
			dest_rect = origin;
			dest_rect.X += origin.Width - width;
			dest_rect.Width = width;
			dest_rect.Height = image->Get_Height();
			source_rect.Height = dest_rect.Height;
			source_rect.Width = dest_rect.Width;
			source_rect.Y = 0;
			source_rect.X = 0;
			HiddenSurface->Blit_From(dest_rect, *image, source_rect);
		}

		/// <summary>
		/// Draws the label of the button.
		/// This routine prints the label using the dialog system's remapped text colors,
		/// nudging it down and to the right while the button is held down.
		/// </summary>
		virtual void Draw_Text(char const * text)
		{
			Rect rect(X, Y, X + Width, Y + Height - 2);
			if (IsPressed) {
				rect.X += 2;
				rect.Y += 4;
			}
			OD_Draw_Text_Remap(*HiddenSurface, text, rect, "dlgsys", ODColorText, 5, 0);
		}


	private:
		bool WasPressed;
		MSEngine *Engine;
};

/// The binary confirms this was in the cpp as the vtable is inside this module
/// Moving it into a header would make the first thing that includes the header construct the vtable in that module

class RestateMission : public MSEngine {
	public:
		RestateMission(void);
		virtual ~RestateMission(void);
		bool Presentation(ScenarioClass * scen);

		virtual void Do_Custom_Draw(Surface *surface);

	private:
		bool Init(ScenarioClass * scen);
		void Cleanup(void);

		enum InputType { INPUT_NEXT, INPUT_BACK, INPUT_VIDEO };
		InputType User_Input(void);
		InputType More_Button(int x, int y);
		void Show_Page(char * text, Rect const & rect);
		MyButton * Get_Button(unsigned int id);

		ScenarioClass *Scenario;
		char BriefingText[1024];
		char * String;
		Rect StringRect;
		int CenterX;
		int CenterY;
		MSFont * Font;
		ConvertClass * Drawer;
		GadgetClass * ButtonList;
		DynamicVectorClass<MyButton *> Buttons;
		// Under the controller scheme the buttons give way to console prompts along the bottom.
		bool Padded;
		MSFont * PromptFont;
		enum PromptType { PROMPT_NONE, PROMPT_MORE, PROMPT_FINAL } Prompt;
		int Page;
		void Draw_Prompts(Surface * surface);
		bool Video_Offered(void) const;
};

enum {
	BUTTON_RESUME = 1,
	BUTTON_VIDEO = 2,
	BUTTON_MORE = 3,

	BUTTON_COUNT = 3,
};

struct RestateButtonStruct {
	int ID;
	int Text;
	Rect Area;

	RestateButtonStruct(int id, int text, Rect rect) :
		Text(text),
		Area(rect),
		ID(id)
	{
	}
};

RestateButtonStruct _buttons[BUTTON_COUNT] = {
	RestateButtonStruct(BUTTON_RESUME, TXT_RESUME_MISSION, Rect(0, 360, 150, 24)),
	RestateButtonStruct(BUTTON_VIDEO, TXT_VIDEO, Rect(0, 360, 150, 24)),
	RestateButtonStruct(BUTTON_MORE, TXT_MORE, Rect(0, 340, 150, 24)),
};


static bool _ResumesMission = false;	// The briefing was opened from within the mission.


/***********************************************************************************************
 * Restate_Mission -- Handles restating the mission objective.                                 *
 *                                                                                             *
 *    This routine will display the mission objective (as text). It will also give the         *
 *    option to redisplay the mission briefing video.                                          *
 *                                                                                             *
 * INPUT:   name  -- The scenario name. This is the unique identifier for the scenario         *
 *                   briefing text as it appears in the "MISSION.INI" file.                    *
 *                                                                                             *
 * OUTPUT:  Returns the response from the dialog. This will either be 1 if the video was       *
 *          requested, or 0 if the return to game options button was selected.                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/23/1995 JLB : Created.                                                                 *
 *   08/06/1995 JLB : Uses preloaded briefing text.                                            *
 *=============================================================================================*/
void Restate_Mission(ScenarioClass * scen)
{
	bool save_started = ScenarioActive;
	ScenarioActive = false;
	_ResumesMission = save_started;

	// Under the controller scheme the page is shown at the shell's size, like the console screens.
	bool padded = Options.ControlScheme == CONTROL_CONTROLLER;
	bool from_shell = padded && Shell_Display_Mode_Active();
	if (padded) {
		Shell_Display_Mode();
	}

	if (RestateMission().Presentation(scen) == true) {
		ThemeType theme = Theme.What_Is_Playing();
		Theme.Stop();
		Play_Movie(scen->BriefMovie, THEME_NONE, 1, 1);
		Theme.Play_Song(theme);
	}

	if (padded && !from_shell) {
		Play_Display_Mode();
	}
	ScenarioActive = save_started;
	Keyboard->Clear();
}


/// <summary>
/// Constructs the mission restatement object.
/// The object holds no scenario, artwork or buttons until Init supplies them.
/// </summary>
RestateMission::RestateMission(void) :
	Scenario(NULL),
	String(NULL),
	Font(NULL),
	Drawer(NULL),
	ButtonList(nullptr),
	Padded(false),
	PromptFont(nullptr),
	Prompt(PROMPT_NONE)
{
	Buttons.Clear();
	BriefingText[0] = '\0';
}


/// <summary>
/// Destroys the mission restatement object.
/// Any artwork and buttons still held are released as the object goes away.
/// </summary>
RestateMission::~RestateMission(void)
{
	Cleanup();
}


/// <summary>
/// Handles the mission restatement presentation.
/// This routine puts up the score backdrop, types the briefing out a page at a time with
/// a "more" button between the pages, and then waits for the player to either resume the
/// mission or ask to see the briefing video again.
/// </summary>
/// <param name="scen">The scenario whose briefing is to be restated.</param>
/// <returns>bool; Did the player ask to see the briefing video?</returns>
bool RestateMission::Presentation(ScenarioClass * scen)
{
	bool result = false;
	if (scen != NULL && AlternateSurface != NULL && HiddenSurface != NULL) {
		if (Init(scen) == true) {
			Keyboard->Clear();
			MouseCursor->Release_Mouse();
			Hide_Mouse();

			Load_Title_Screen("SCORE.PCX", AlternateSurface, &CCPalette);
			HiddenSurface->Blit_From(*AlternateSurface);
			Add_Update_Rect(HiddenSurface->Get_Rect());
			Blit_All(HiddenSurface);

			std::vector<char *> pages;
			if (strlen(BriefingText) != 0) {
				Rect rect(CenterX + 110, CenterY + 60, 420, 280);
				MSPrintAnim::Word_Wrap(BriefingText, Font, 420);
				Font->Get_String_Rect(BriefingText, StringRect);
				StringRect.X = rect.X + (rect.Width - StringRect.Width) / 2;
				StringRect.Y = rect.Y + (rect.Height - Font->Get_Font_Height() * (StringRect.Height / Font->Get_Font_Height())) / 2;
				StringRect = Intersect(rect, StringRect);
				MSPrintAnim::Paginate(BriefingText, Font, rect.Height);

				for (char * token = strtok(BriefingText, "\f"); token != NULL; token = strtok(NULL, "\f")) {
					pages.push_back(token);
				}
			}

			// The pad's back button turns to the page before, so the pages are kept.
			Page = 0;
			Rect rect(CenterX + 110, CenterY + 60, 420, 280);
			while (true) {
				if (!pages.empty()) {
					Show_Page(pages[Page], rect);
				}
				if (Page + 1 < int(pages.size())) {
					Show_Mouse();
					InputType input = More_Button(StringRect.X + StringRect.Width / 2, StringRect.Y + StringRect.Height);
					Hide_Mouse();
					Page += (input == INPUT_BACK) ? (Page > 0 ? -1 : 0) : 1;
					continue;
				}

				MyButton *resume_button = Get_Button(BUTTON_RESUME);
				if (resume_button != NULL) {
					resume_button->Enable();
				}
				if (scen->BriefMovie != VQ_NONE) {
					MyButton *video_button = Get_Button(BUTTON_VIDEO);
					if (video_button != NULL) {
						video_button->Enable();
					}
				}

				Prompt = PROMPT_FINAL;
				Add_Update_Rect(HiddenSurface->Get_Rect());
				Show_Mouse();
				InputType input = User_Input();
				Hide_Mouse();
				Prompt = PROMPT_NONE;
				if (input == INPUT_BACK && Page > 0) {
					Page--;
					continue;
				}
				result = input == INPUT_VIDEO;
				break;
			}

			HiddenSurface->Fill(0);
			Add_Update_Rect(HiddenSurface->Get_Rect());
			Blit_All(HiddenSurface);

			Cleanup();
			Keyboard->Clear();

			Show_Mouse();
			MouseCursor->Capture_Mouse();
		}
		return(result);
	}
	return(result);
}


/// <summary>
/// Prepares the mission restatement presentation.
/// This routine fetches the briefing text -- the scenario's own, and the mission INI
/// database's entry for it when the scenario carries none -- creates the font and
/// drawer the animations need, and lays out the buttons the player will be offered.
/// </summary>
/// <param name="scen">The scenario whose briefing is to be restated.</param>
/// <returns>bool; Was the presentation successfully prepared?</returns>
bool RestateMission::Init(ScenarioClass * scen)
{
	CCFileClass file;
	CCINIClass ini;
	char buffer[32];

	Cleanup();

	if (scen == NULL) {
		return(false);
	}

	Scenario = scen;
	CenterX = (HiddenSurface->Get_Width() - 640) / 2;
	CenterY = (HiddenSurface->Get_Height() - 400) / 2;
	file.Close();

	if (strlen(Scenario->BriefingText)) {
		DebugString("Restate: Fetching breifing text from %s\n", Scenario->ScenarioName);
		strcpy(BriefingText, Scenario->BriefingText);

	} else {
	if (Scenario->RequiredAddOn > ADDON_BASE_GAME) {
		sprintf(buffer, "MISSION%1d.INI",  Scenario->RequiredAddOn);
		file.Set_Name(buffer);
	} else {
		file.Set_Name("MISSION.INI");
	}

	if (file.Is_Available() == true) {
		ini.Load(file, false);
		DebugString("Restate: Fetching breifing text from Mission.ini\n");

		if (ini.Is_Present(Scenario->ScenarioName, "Briefing")) {
			ini.Get_String(Scenario->ScenarioName, "Briefing", "", buffer, sizeof(buffer));
			if (strlen(buffer)) {
				ini.Get_TextBlock(buffer, BriefingText, sizeof(BriefingText));
			}
			}
		}
	}

	// The presentation lays the text out itself, so the breaks the briefing carries are folded away.
			char * string = BriefingText;
			while (*string) {
				if (*string != '\n' && *string != '@') {
					string++;
				} else {
					char * next = ++string;
					if (*next != '\0') {
						while (*next != '\0' && *next == ' ') {
							next++;
						}
					}
					if (next != string) {
						strcpy(string, next);
					}
				}
			}

	Font = new MSFont;
	if (Font == NULL) {
		DebugString("Restate: Unable to create font!\n");
		return(false);
	}
	Padded = Options.ControlScheme == CONTROL_CONTROLLER;
	Page = 0;
	if (Padded) {
		PromptFont = new MSFont(false);
	}

	Drawer = Create_Drawer("MAPSEL.PAL");
	if (Drawer == NULL) {
		DebugString("Restate: Unable to create animation drawer!\n");
		return(false);
	}

	/*
	**	Other inits.
	*/
	LogicalSurface = HiddenSurface;

	/*
	**	Initialize the button structures. All are initialized, even though one (or none) may
	**	actually be added to the button list.
	*/
	int i;
	for (i = 0; i < BUTTON_COUNT; i++) {
		MyButton *btn = new MyButton(this,
									_buttons[i].ID,
									_buttons[i].Text,
									TPF_BUTTON,
									_buttons[i].Area.X, _buttons[i].Area.Y, _buttons[i].Area.Width, _buttons[i].Area.Height
									);

		if (btn == NULL) {
			DebugString("Restate: Unable to create button!\n");
			return(false);
		}

		btn->Disable();
		Buttons.Add(btn);
	}

	/*
	**	Add and initialize the buttons to the button list.
	*/
	if (!Padded) {
		ButtonList = Buttons[0];
		for (i = 1; i < Buttons.Count(); i++) {
			Buttons[i]->Add(*ButtonList);
		}
	}

	MyButton *resume = Get_Button(BUTTON_RESUME);
	MyButton *video = Get_Button(BUTTON_VIDEO);

	if (scen->BriefMovie == VQ_NONE) {
		resume->X = CenterX + (640 - resume->Width) / 2;
		resume->Y += CenterY;
	} else {
		int width = std::max(resume->Width, video->Width);
		int xx = (2 * (320 - width) / 4);

		resume->X = xx + CenterX;
		resume->Y += CenterY - resume->Height / 2;
		resume->Width = width;

		video->X = resume->X + width + 2 * xx;
		video->Y += CenterY - video->Height / 2;
		video->Width = width;
	}

	AlternateSurface->Fill(0);
	HiddenSurface->Fill(0);

	return(true);
}


/// <summary>
/// Frees the resources held by the mission restatement.
/// This routine is used when the presentation is finished with, and again before a fresh
/// one is set up, so it is always safe to call.
/// </summary>
void RestateMission::Cleanup(void)
{
	String = NULL;
	if (Font != NULL) {
		delete Font;
		Font = NULL;
	}
	if (PromptFont != nullptr) {
		delete PromptFont;
		PromptFont = nullptr;
	}
	Prompt = PROMPT_NONE;
	if (Drawer != NULL) {
		delete Drawer;
		Drawer = NULL;
	}
	ButtonList = NULL;
	for (int i = 0; i < Buttons.Count(); i++) {
		MyButton *btn = Buttons[i];
		if (btn != NULL) {
			delete btn;
		}
	}
	Buttons.Clear();
}


// Clears the text area and types a page out, leaving it as the page the redraw prints.
void RestateMission::Show_Page(char * text, Rect const & rect)
{
	String = NULL;
	HiddenSurface->Blit_From(rect, *AlternateSurface, rect);
	Add_Update_Rect(rect);
	MSPrintAnim * anim = new MSWordAnim(text, StringRect.X, StringRect.Y, Font, StringRect, 5);
	Add_Animation(anim);
	Wait_For_Anim(anim);
	String = text;
}


/// <summary>
/// Draws the parts of the presentation the animation engine does not own.
/// The engine calls this routine as it updates, so that the briefing page and the
/// buttons are laid over the top of whatever animation is running.
/// </summary>
void RestateMission::Do_Custom_Draw(Surface *surface)
{
	if (String != NULL) {
		Font->Draw_String(surface, String, StringRect.X, StringRect.Y, 2);
	}
	if (ButtonList != NULL) {
		ButtonList->Draw_All();
	}
	Draw_Prompts(surface);
}


void RestateMission::Draw_Prompts(Surface * surface)
{
	if (!Padded || PromptFont == nullptr || Prompt == PROMPT_NONE) {
		return;
	}
	int y = CenterY + CONSOLE_PROMPT_Y;
	int used = Pad_Prompt_Inset(PromptFont->Get_Font_Height());
	// Before the mission starts there is nothing to resume, so the last page just closes.
	char const * accept = Fetch_String(Prompt == PROMPT_MORE ? TXT_MORE : _ResumesMission ? TXT_RESUME_MISSION : TXT_OK);
	int x = CenterX + CONSOLE_SHELL_WIDTH - CONSOLE_PROMPT_INSET - used - PromptFont->Get_String_Width(accept);
	Draw_Pad_Prompt(*surface, *PromptFont, PAD_BUTTON_ACCEPT, accept, x, y);
	x = CenterX + CONSOLE_PROMPT_INSET;
	if (Page > 0) {
		char const * back = Fetch_String(TXT_BACK);
		Draw_Pad_Prompt(*surface, *PromptFont, PAD_BUTTON_BACK, back, x, y);
		x += used + PromptFont->Get_String_Width(back) + CONSOLE_PROMPT_INSET;
	}
	if (Video_Offered()) {
		Draw_Pad_Prompt(*surface, *PromptFont, PAD_BUTTON_THIRD, Fetch_String(TXT_VIDEO), x, y);
	}
}


bool RestateMission::Video_Offered(void) const
{
	return(Prompt == PROMPT_FINAL && Scenario != nullptr && Scenario->BriefMovie != VQ_NONE);
}


/// <summary>
/// Handles the player's input during the mission restatement.
/// This routine polls the button list until the player picks one of the offered choices,
/// or dismisses the page with the space bar or the escape key.
/// </summary>
/// <returns>What the player asked for: the next page or the mission, the page before, or the video.</returns>
RestateMission::InputType RestateMission::User_Input(void)
{
	unsigned input = KN_NONE;
	bool running = true;
	bool back = false;
	Keyboard->Clear();
	if (ButtonList != NULL) {
		ButtonList->Draw_All();
	}
	if (Padded) {
		Draw_Prompts(HiddenSurface);
		Rect line(CenterX, CenterY + 360, 640, 40);
		Add_Update_Rect(line);
		Blit_All(HiddenSurface);
	}
	// A button still held from the movie or the screen before must not count as a press here.
	GamepadStateType previous = Gamepad_Read();
	do {
		Wait_For_Focus();
		if (ButtonList != NULL) {
			input = ButtonList->Input();
		} else {
			if (Keyboard->Check() != KN_NONE) {
				input = Keyboard->Get();
			}
		}
		// The pad's accept carries on, as Space does; back turns to the page before, or closes
		// a single page; the third button plays the video when one is offered.
		GamepadStateType pad = Gamepad_Read();
		if (pad.Accept && !previous.Accept) {
			input = KN_SPACE;
		} else if (pad.Back && !previous.Back) {
			if (Page > 0) {
				back = true;
				input = KN_SPACE;
			} else if (Prompt == PROMPT_FINAL) {
				input = KN_SPACE;
			}
		} else if (pad.Third && !previous.Third && Video_Offered()) {
			input = BUTTON_VIDEO|KN_BUTTON;
		}
		previous = pad;
		switch (input) {
			case (BUTTON_RESUME|KN_BUTTON):
			case (BUTTON_VIDEO|KN_BUTTON):
				running = false;
				break;

			case (BUTTON_MORE|KN_BUTTON):
			case (KN_SPACE):
			case (KN_RETURN):
			case (KN_ESC):
				running = false;
				break;
		}
		Wait_Delay(1);

	} while (running == true);
	Keyboard->Clear();

	if (input == (BUTTON_VIDEO|KN_BUTTON)) {
		return(INPUT_VIDEO);
	}
	return(back ? INPUT_BACK : INPUT_NEXT);
}


/// <summary>
/// Presents the "more" button and waits for the player.
/// This routine is used between the pages of a long briefing. It parks the button under
/// the text just printed and does not return until the player asks for the next page.
/// </summary>
/// <param name="x">The horizontal center for the button.</param>
/// <param name="y">The top edge for the button.</param>
RestateMission::InputType RestateMission::More_Button(int x, int y)
{
	InputType input = INPUT_NEXT;
	MyButton *btn = Get_Button(BUTTON_MORE);
	if (btn) {
		btn->X = x - (btn->Width / 2);
		btn->Y = y;
		btn->Enable();
		Prompt = PROMPT_MORE;
		Add_Update_Rect(HiddenSurface->Get_Rect());
		input = User_Input();
		Prompt = PROMPT_NONE;
		if (Padded) {
			Rect line(CenterX, CenterY + 360, 640, 40);
			HiddenSurface->Blit_From(line, *AlternateSurface, line);
			Add_Update_Rect(line);
		} else {
			btn->Draw();
		}
		btn->Disable();
	}
	return(input);
}


/// <summary>
/// Fetches the button that carries the specified identifier.
/// </summary>
/// <returns>Returns with a pointer to the button. Otherwise, NULL is returned.</returns>
MyButton *RestateMission::Get_Button(unsigned int id)
{
	for (int i = 0; i < Buttons.Count(); i++) {
		if (Buttons[i]->ID == id) {
			return(Buttons[i]);
		}
	}
	return(NULL);
}