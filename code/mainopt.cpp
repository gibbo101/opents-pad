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
#include "_mixfile.h"
#include "_rect.h"
#include "_surface.h"
#include "_tactica.h"
#include "convert.h"
#include "data.h"
#include "dbgprint.h"
#include "dsaudio.h"
#include "dsurface.h"
#include "gamedlg.h"
#include "gamepad.h"
#include "globals.h"
#include "init.h"
#include "language/language.h"
#include "misc.h"
#include "video.h"
#include "mixfile.h"
#include "msgbox.h"
#include "newmenu.h"
#include "ownrdraw.h"
#include "padglyph.h"
#include "shapeset.h"
#include "sidebar.h"
#include "sounddlg.h"
#include "stimer.h"
#include "surface.h"
#include "tactical.h"
#include "wwmouse.h"

#include "color.hh"

#include <algorithm>


BOOL CALLBACK Main_Options_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
BOOL CALLBACK Display_Options_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
bool Test_Display_Mode_Dialog(int width, int height);
BOOL CALLBACK Test_Display_Mode_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

GameOptionsClass TempOptions;

enum { TAB_BAR_HEIGHT = 16 };		// The top bar's rows, the tab art's height.


/// <summary>
/// Brings up the main options dialog.
/// This routine drives the options menu, dispatching to the sound, display, network,
/// keyboard and game settings dialogs until the player backs out. A resolution change is
/// offered as a trial first, and the settings are written out when the player leaves.
/// </summary>
/// <remarks>Game logic is suspended for the duration of this routine.</remarks>
void Main_Options_Dialog(void)
{
	// The console screen leaves when the game goes inactive, so it runs before that flag drops.
	if (Options.ControlScheme == CONTROL_CONTROLLER) {
		Console_Options_Screen();
		return;
	}

	bool old_game_active = GameActive;
	GameActive = false;

	HWND main_handle;
	LONG main_rc;

	HWND in_handle;
	LONG in_rc;

	while (true) {
		do {
			main_rc = -1;
			main_handle = OwnerDraw::Begin_Dialog(IDD_OPT_MAIN, Main_Options_Dialog_Proc);
		} while (main_handle == 0);
		SetWindowLong(main_handle, DWL_USER, (LONG)&main_rc);

		OwnerDraw::Move_Dialog(main_handle, -1, (HiddenSurface->Get_Height() - 400) / 2 + 147);
		OwnerDraw::Display_Dialog(main_handle);

		while (main_rc < 0) {
			if (OwnerDraw::Dialog_Message_Handler() == true) {
				break;
			}
			Title_Screen_Restore();
		}

		OwnerDraw::End_Dialog(main_handle);

		switch (main_rc) {
			case IDC_OPTMAIN_SOUND:
				SoundControlsClass().Dialog();
				break;

			case IDC_OPTMAIN_DISPLAY: {
				while (true) {
					do {
						TempOptions = Options;
						in_rc = -1;
						in_handle = OwnerDraw::Begin_Dialog(IDD_OPT_DISPLAY, Display_Options_Dialog_Proc);
					} while (in_handle == 0);
					SetWindowLong(in_handle, DWL_USER, (LONG)&in_rc);
					OwnerDraw::Display_Dialog(in_handle);

					while (in_rc < 0) {
						if (OwnerDraw::Dialog_Message_Handler() == true) {
							break;
						}
						Title_Screen_Restore();
					}

					OwnerDraw::End_Dialog(in_handle);

					if (in_rc != 1) {
						break;
					}
					if (TempOptions.ScreenWidth == Options.ScreenWidth && TempOptions.ScreenHeight == Options.ScreenHeight) {
						break;
					}

						if (WWMessageBox().Process(TXT_ABOUT_TO_TRY_MODE, TXT_OK, TXT_CANCEL) == 0) {
							if (!Test_Display_Mode_Dialog(TempOptions.ScreenWidth, TempOptions.ScreenHeight)) {
								continue;
							}
							Options.ScreenWidth = TempOptions.ScreenWidth;
							Options.ScreenHeight = TempOptions.ScreenHeight;
						}

					break;
				}
			}
			break;

			// The shell takes the other set of screens as soon as the driver returns.
			case IDC_OPTMAIN_CONTROLLER:
				Options.ControlScheme = CONTROL_CONTROLLER;
				Options.ControlSchemeAuto = false;
				Options.Save_Settings();
				GameActive = old_game_active;
				return;

			case IDC_OPTMAIN_KEYBOARD:
				Options.Hotkey_Dialog();
				break;

			case IDC_OPTMAIN_GAME_SETTINGS:
				GameControlsClass().Dialog();
				break;

			default:
				Options.Save_Settings();
				GameActive = old_game_active;
				return;
		}
	}
}


/// <summary>
/// Handles the main options dialog.
/// This routine reports the button the player pressed back to the options dialog driver so
/// that it can bring up the appropriate sub dialog. The sound button is disabled when there
/// is no audio hardware to talk to.
/// </summary>
BOOL CALLBACK Main_Options_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	int *result;
	HWND handle;

	int rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);
	if (rc == 0) {
		result = (int *)GetWindowLong(window, DWL_USER);
		switch (message) {

			case WM_COMMAND:
				*result = LOWORD(wparam);
				break;

			case WM_INITDIALOG:
				handle = GetDlgItem(window, IDC_OPTMAIN_SOUND);
				if (handle) {
					EnableWindow(handle, Audio_Available());
				}
				handle = GetDlgItem(window, IDC_OPTMAIN_CONTROLLER);
				if (handle) {
					EnableWindow(handle, Gamepad_Read().Connected);
				}
				break;

		}
		return(0);
	}
	return(rc);
}


/// <summary>
/// Switches the game over to a new render resolution with the sidebar in the frame.
/// </summary>
bool Change_Display_Mode(int width, int height)
{
	return(Change_Display_Mode(width, height, 0));
}


/// <summary>
/// Switches the game over to a new render resolution.
/// Every drawing surface is destroyed and recreated at the new size, so any pointer held
/// across this call is stale.
/// </summary>
/// <param name="width">The width to render at, the sidebar's columns included.</param>
/// <param name="height">The height to render at.</param>
/// <param name="sidebarheight">A height for the sidebar's own surface, which the presenter
/// then shows beside the frame at its own scale; zero keeps the sidebar in the frame.</param>
/// <returns>bool; Was the mode changed? If not, nothing has been disturbed.</returns>
bool Change_Display_Mode(int width, int height, int sidebarheight)
{
	DebugString("About to set video mode\n");

	Hide_Mouse();

	if (!Video_Set_Mode(width, height)) {
		DebugString("Video_Set_Mode failed.\n");
		Show_Mouse();
		return(false);
		}

	// The bar goes with the sidebar: both are drawn at the sidebar's scale, so neither
	// changes size with the zoom.
	if (sidebarheight > 0 && !Video_Set_Sidebar(SidebarClass::SIDE_WIDTH, sidebarheight, Options.IsSidebarOnRight, TAB_BAR_HEIGHT)) {
		DebugString("Video_Set_Sidebar failed; sidebar stays in the frame.\n");
		sidebarheight = 0;
	}

	if (TabSurface != NULL) {
		delete TabSurface;
		TabSurface = NULL;
	}
	VideoScaleInfo const & layout = Video_Get_Scale_Info();
	if (sidebarheight > 0 && layout.Bar_Is_Split()) {
		TabSurface = new DSurface(layout.BarWidth, layout.BarHeight);
		TabSurface->Fill(0);
		DebugString("TabSurface (%dx%d)\n", layout.BarWidth, layout.BarHeight);
	}

	VisibleRect = Rect(0, 0, width, height);
	DebugString("VisibleRect: %dx%d, sidebar %s\n", width, height, sidebarheight > 0 ? "split" : "in frame");

	if (VisibleSurface != NULL) {
		delete VisibleSurface;
		VisibleSurface = NULL;
	}

	if (AlternateSurface != NULL) {
		delete AlternateSurface;
		AlternateSurface = NULL;
	}

	if (HiddenSurface != NULL) {
		delete HiddenSurface;
		HiddenSurface = NULL;
	}

	if (TileSurface != NULL) {
		delete TileSurface;
		TileSurface = NULL;
	}

	if (SidebarSurface != NULL) {
		delete SidebarSurface;
		SidebarSurface = NULL;
	}

	if (CompositeSurface != NULL) {
		delete CompositeSurface;
		CompositeSurface = NULL;
	}

	VisibleSurface = DSurface::Create_Primary();
	if (VisibleSurface == NULL) {
		Show_Mouse();
		return(false);
	}

	/*
	 * A window that is tracking the frame follows it to the new size. One the player
	 * sized themselves, and a window covering the screen, both stay as they are and the
	 * frame is scaled into them instead. Under the controller scheme the window is the
	 * panel the zoom fits, so it never follows the frame.
	 */
	if (WindowedMode && Options.WindowWidth <= 0 && Options.WindowHeight <= 0 && Options.ControlScheme != CONTROL_CONTROLLER) {
		RECT windowrect;
		SetRect(&windowrect, 0, 0, width, height);
		AdjustWindowRectEx(&windowrect, GetWindowLong(MainWindow, GWL_STYLE), FALSE, GetWindowLong(MainWindow, GWL_EXSTYLE));

		int newwidth = windowrect.right - windowrect.left;
		int newheight = windowrect.bottom - windowrect.top;

		/*
		 * The window grows about its middle rather than its corner, so the picture stays
		 * where the player was looking.
		 */
		RECT current;
		GetWindowRect(MainWindow, &current);
		int x = current.left + (((current.right - current.left) - newwidth) / 2);
		int y = current.top + (((current.bottom - current.top) - newheight) / 2);

		/*
		 * Growing about the middle can push the window past the edges of the screen, and a
		 * title bar above the top of it cannot be grabbed to bring the window back.
		 */
		MONITORINFO monitor;
		monitor.cbSize = sizeof(monitor);
		if (GetMonitorInfo(MonitorFromWindow(MainWindow, MONITOR_DEFAULTTONEAREST), &monitor)) {
			if (x + newwidth > monitor.rcWork.right) x = monitor.rcWork.right - newwidth;
			if (y + newheight > monitor.rcWork.bottom) y = monitor.rcWork.bottom - newheight;
			if (x < monitor.rcWork.left) x = monitor.rcWork.left;
			if (y < monitor.rcWork.top) y = monitor.rcWork.top;
		}

		SetWindowPos(MainWindow, NULL, x, y, newwidth, newheight, SWP_NOZORDER);
	}

	Rect temp = VisibleRect;
	temp.X = ((Options.IsSidebarOnRight || Debug_Map) ? 0 : SidebarClass::SIDE_WIDTH);
	temp.Y = TAB_BAR_HEIGHT;
	temp.Width -= SidebarClass::SIDE_WIDTH;
	temp.Height -= TAB_BAR_HEIGHT;

	Allocate_Surfaces(VisibleRect, Rect(0, 0, temp.Width, VisibleRect.Height), Rect(0, 0, temp.Width, VisibleRect.Height), Rect(0, 0, SidebarClass::SIDE_WIDTH, sidebarheight > 0 ? sidebarheight : VisibleRect.Height));
	LogicalSurface = HiddenSurface;

	if (MouseCursor != NULL) {
		((WWMouseClass*)MouseCursor)->Calc_Confining_Rect();
	}

	Map.Set_View_Dimensions(temp);

	Map.Init_IO();
	Map.Activate(
#ifdef _DEBUG
		Debug_Map == true ? 1 : 0
#else
		1
#endif
	);
	Map.Reposition_Sidebar();
	Map.Flag_To_Redraw(GS_REDRAW_ALL);
	Show_Mouse();

	DebugString("Mode change complete.\n");

	return(true);
}


enum {
	SHELL_WIDTH = 640,
	SHELL_HEIGHT = 400,
};


static bool Set_Display_Mode_If_Needed(int width, int height, int sidebarheight)
{
	if (VideoModeWidth == width && VideoModeHeight == height && Video_Get_Scale_Info().SidebarHeight == sidebarheight) {
		return(true);
	}
	return(Change_Display_Mode(width, height, sidebarheight));
}


/// <summary>
/// Renders the shell menus at the size their artwork was drawn for, so they fill the
/// screen on any panel. The controller scheme renders the shell at 640x400; the keyboard
/// scheme keeps the shell at the play resolution.
/// </summary>
/// <returns>bool; Is the shell rendering at its resolution?</returns>
bool Shell_Display_Mode(void)
{
	if (Options.ControlScheme != CONTROL_CONTROLLER) {
		return(Play_Display_Mode());
	}
	return(Set_Display_Mode_If_Needed(SHELL_WIDTH, SHELL_HEIGHT, 0));
}


/// <summary>
/// Is the controller scheme's shell size the current render size?
/// </summary>
bool Shell_Display_Mode_Active(void)
{
	return(Options.ControlScheme == CONTROL_CONTROLLER && VideoModeWidth == SHELL_WIDTH && VideoModeHeight == SHELL_HEIGHT);
}


/// <summary>
/// Returns the render resolution to the play size before a scenario starts: the keyboard
/// scheme's configured resolution, or the controller's zoom fitted to the panel.
/// </summary>
/// <returns>bool; Is the game rendering at the play resolution?</returns>
bool Play_Display_Mode(void)
{
	if (Options.ControlScheme != CONTROL_CONTROLLER) {
		return(Set_Display_Mode_If_Needed(Options.ScreenWidth, Options.ScreenHeight, 0));
	}
	int width;
	int height;
	Pad_Zoom_Size(width, height);
	return(Set_Display_Mode_If_Needed(width, height, Pad_Sidebar_Height()));
}


// The render heights the controller's zoom steps through. Width follows the panel's
// shape, so no panel shows bars; the scale never drops below one.
static int const _ZoomLadder[] = {480, 540, 600, 660, 720, 768, 840, 900, 1080, 1200, 1440};
enum {
	ZOOM_RUNGS = sizeof(_ZoomLadder) / sizeof(_ZoomLadder[0]),
	ZOOM_BASELINE = 768,
	ZOOM_BASELINE_DECK = 600,		// A seven inch panel wants bigger sprites from the start.
};


static void Panel_Size(int & width, int & height)
{
	VideoScaleInfo const & scale = Video_Get_Scale_Info();
	width = scale.DrawableWidth;
	height = scale.DrawableHeight;
	if (width <= 0 || height <= 0) {
		width = Options.ScreenWidth;
		height = Options.ScreenHeight;
	}
}


// The rungs the panel can hold, and the panel itself when it is shorter than all of them.
static int Zoom_Top_Rung(void)
{
	int width;
	int height;
	Panel_Size(width, height);
	int top = 0;
	while (top + 1 < ZOOM_RUNGS && _ZoomLadder[top + 1] <= height) {
		top++;
	}
	return(top);
}


static int Zoom_Nearest_Rung(int height)
{
	int nearest = 0;
	for (int rung = 1; rung < ZOOM_RUNGS; rung++) {
		if (abs(_ZoomLadder[rung] - height) < abs(_ZoomLadder[nearest] - height)) {
			nearest = rung;
		}
	}
	return(nearest);
}


/// <summary>
/// The height the split sidebar is drawn at under the controller scheme: the pad's own
/// panel, the radar and mode buttons over five rows of two cells and the foot plate, so
/// scaled to the screen's height the panel fills it whatever the map is zoomed to.
/// The panel's pieces are read from the loaded sidebar art; before it loads, the size
/// the game's art gives.
/// </summary>
int Pad_Sidebar_Height(void)
{
	enum { ART_HEIGHT = 512 };
	int height = ART_HEIGHT;
	if (SidebarClass::SidebarShape != NULL && SidebarClass::SidebarMiddleShape != NULL && SidebarClass::SidebarBottomShape != NULL && SidebarClass::SidebarAddonShape != NULL) {
		height = SidebarClass::SIDE_Y + SidebarClass::SidebarShape->Get_Height()
			+ SidebarClass::PAD_SECTION_ROWS * SidebarClass::SidebarMiddleShape->Get_Height()
			+ SidebarClass::SidebarBottomShape->Get_Height() + SidebarClass::SidebarAddonShape->Get_Height();
		static int _logged = 0;
		if (_logged != height) {
			DebugString("Pad sidebar panel is %d high\n", height);
			_logged = height;
		}
	}
	return(std::max(height, 1));
}


// The frame width for a ladder height: the map's columns follow the shape of the panel
// left beside the sidebar and under the bar, both scaled to fill the panel's height with
// the sidebar's panel, and the sidebar's own columns are added, so the frame carries
// everything at the sizes it is presented at.
int Pad_Zoom_Width(int height)
{
	int panel_width;
	int panel_height;
	Panel_Size(panel_width, panel_height);
	double scale = double(panel_height) / double(Pad_Sidebar_Height());
	int map_panel_width = std::max(panel_width - int(SidebarClass::SIDE_WIDTH * scale), 1);
	int map_panel_height = std::max(panel_height - int(TAB_BAR_HEIGHT * scale), 1);
	int map_height = std::max(height - TAB_BAR_HEIGHT, 1);
	int width = int(((long long)map_height * map_panel_width * 2 + map_panel_height) / (map_panel_height * 2));
	return((width & ~1) + SidebarClass::SIDE_WIDTH);
}


void Pad_Zoom_Name(int width, int height, char * buffer, int size)
{
	int panel_width;
	int panel_height;
	Panel_Size(panel_width, panel_height);
	snprintf(buffer, size, "%.2fx, %d x %d", height > 0 ? float(panel_height) / float(height) : 1.0f, width, height);
}


int Pad_Zoom_Neighbour(int height, int steps)
{
	int top = Zoom_Top_Rung();
	int rung = std::clamp(Zoom_Nearest_Rung(height) - steps, 0, top);
	int panel_width;
	int panel_height;
	Panel_Size(panel_width, panel_height);
	return(std::min(_ZoomLadder[rung], panel_height));
}


// A saved size counts as the panel's when its width is within a percent of the one the
// panel gives its height.
static bool Zoom_Fits_Panel(int width, int height)
{
	if (width <= 0 || height <= 0) {
		return(false);
	}
	int difference = abs(width - Pad_Zoom_Width(height));
	return(difference <= std::max(width / 100, 1));
}


void Pad_Zoom_Size(int & width, int & height)
{
	if (!Zoom_Fits_Panel(Options.PadZoomWidth, Options.PadZoomHeight)) {
		int baseline = On_Steam_Deck() ? ZOOM_BASELINE_DECK : ZOOM_BASELINE;
		Options.PadZoomHeight = Pad_Zoom_Neighbour(baseline, 0);
		Options.PadZoomWidth = Pad_Zoom_Width(Options.PadZoomHeight);
		DebugString("Pad zoom baseline %dx%d\n", Options.PadZoomWidth, Options.PadZoomHeight);
	}
	width = Options.PadZoomWidth;
	height = Options.PadZoomHeight;
}


/// <summary>
/// Steps the controller's zoom while a game is on, positive to zoom in. The view keeps
/// its centre and the new size is saved.
/// </summary>
/// <returns>bool; Did the size change?</returns>
bool Pad_Zoom_Step(int steps)
{
	int width;
	int height;
	Pad_Zoom_Size(width, height);
	int next_height = Pad_Zoom_Neighbour(height, steps);
	int next_width = Pad_Zoom_Width(next_height);
	if (next_width == width && next_height == height) {
		return(false);
	}

	Point2D centre = TacticalMap->Get_Tactical_Position() + Point2D(TacticalRect.Width / 2, TacticalRect.Height / 2);
	unsigned long started = timeGetTime();
	if (!Change_Display_Mode(next_width, next_height, Pad_Sidebar_Height())) {
		return(false);
	}
	DebugString("Pad zoom %dx%d took %lu ms\n", next_width, next_height, timeGetTime() - started);
	TacticalMap->Set_Tactical_Position(centre - Point2D(TacticalRect.Width / 2, TacticalRect.Height / 2));
	Options.PadZoomWidth = next_width;
	Options.PadZoomHeight = next_height;
	Options.Save_Settings();
	return(true);
}


/// <summary>
/// Tries a display mode out and asks the player to confirm it.
/// This routine switches to the requested mode and puts up a confirmation dialog. If the
/// player does not accept the mode -- or says nothing at all, because a bad mode may well
/// leave the screen unreadable -- the previous resolution is restored.
/// </summary>
/// <param name="width">The width of the display mode to try.</param>
/// <param name="height">The height of the display mode to try.</param>
/// <returns>bool; Was the new display mode accepted and left in place?</returns>
bool Test_Display_Mode_Dialog(int width, int height)
{
	int rc = -1;

	DebugString("Testing display mode @ %dx%d\n", width, height);
	Hide_Mouse();
	HiddenSurface->Fill(TBLACK);
	Update_Visible_Surface();

	if (!Change_Display_Mode(width, height)) {
		return(false);
	}

	HiddenSurface->Fill(TBLACK);
	Update_Visible_Surface();
	Show_Mouse();
	Draw_Menu_Background();

	HWND dialog = OwnerDraw::Begin_Dialog(IDD_OPT_CONFIRM_MODE, Test_Display_Mode_Dialog_Proc);
	if (dialog) {
		SetWindowLong(dialog, DWL_USER, (LONG)&rc);
		OwnerDraw::Display_Dialog(dialog);

		CDTimerClass<SystemTimerClass> timer = 10 * TIMER_SECOND;
		while (rc < 0) {
			if (OwnerDraw::Dialog_Message_Handler() == true) {
				break;
			}
			Title_Screen_Restore();
			if (timer <= 0) {
				PostMessage(dialog, WM_COMMAND, WM_DESTROY, 0);
				timer = 5 * TIMER_SECOND;
			}
		}

		OwnerDraw::End_Dialog(dialog);
		if (rc != IDOK) {
			DebugString("Resetting display mode @ %dx%d\n", Options.ScreenWidth, Options.ScreenHeight);
			Change_Display_Mode(Options.ScreenWidth, Options.ScreenHeight);
			LogicalSurface = HiddenSurface;
			return(false);
		}
	}

	DebugString("Keeping display mode @ %dx%d\n", width, height);
	LogicalSurface = HiddenSurface;
	return(true);
}


/// <summary>
/// Handles the mode confirmation dialog.
/// This routine records the button the player pressed so that the mode test can tell
/// whether the new resolution was accepted or rejected.
/// </summary>
BOOL CALLBACK Test_Display_Mode_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	int * result;
	int id;

	int rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);
	if (rc == 0) {
		result = (int *)GetWindowLong(window, DWL_USER);
		switch (message) {
			case WM_COMMAND:
				id = LOWORD(wparam);
				if (id > 0 && id <= IDCANCEL) {
					*result = LOWORD(wparam);
				}
				break;
		}
		return(0);
	}
	return(rc);
}


/// <summary>
/// Handles the display options dialog messages.
/// This routine fills the resolution list with the display modes the hardware reports,
/// remembers which one the player picked, and tracks the movie stretching preference. The
/// chosen resolution is staged in the temporary options so that it can be tested before
/// being made permanent.
/// </summary>
static __forceinline BOOL Display_Options_Dialog_Body(HWND window, UINT message, WPARAM wparam)
{
	enum {
		MIN_WIDTH = 640,
		MIN_HEIGHT = 400,
		MAX_WIDTH = 4096,
		MAX_HEIGHT = 4096,
	};

	static int * _modes = NULL;
	static int _current_mode = -1;
	static int _previous_mode = -1;
	static bool _initialized = true;

	int * result = (int *)GetWindowLong(window, DWL_USER);
	switch (message) {
		case WM_COMMAND:
			switch (LOWORD(wparam)) {
				default:
					return(0);

				case IDC_DISPLAY_RESLIST: {
					HWND list = GetDlgItem(window, IDC_DISPLAY_RESLIST);
					_current_mode = ListBox_GetCurSel(list);
				}
				return(0);

				case IDOK: {
					if (_previous_mode != _current_mode) {
						Center_Window_Within_Window(window, MainWindow);
						HWND list = GetDlgItem(window, IDC_DISPLAY_RESLIST);
						if (list) {
							int index = ListBox_GetItemData(list, _current_mode);
							int * modes = &_modes[2 * index];
							TempOptions.ScreenWidth = modes[0];
							TempOptions.ScreenHeight = modes[1];
						}
					}
					HWND button = GetDlgItem(window, IDC_STRETCH_MOVIES);
					if (button) {
						Options.StretchMovies = Button_GetCheck(button) == BST_CHECKED;
					}
				}
				break;

				case IDCANCEL:
					break;
			}
			delete [] _modes;
			*result = LOWORD(wparam);
			break;

		case WM_INITDIALOG: {
			HWND list = GetDlgItem(window, IDC_DISPLAY_RESLIST);
			_modes = EnumDisplayModes(MIN_WIDTH, MIN_HEIGHT, MAX_WIDTH, MAX_HEIGHT);
			int * modes = _modes;
			int item_index = 0;
			int initial_mode = -1;
			int mode_index = 0;
			if (modes != NULL) {
				while (*modes != 0) {
					int width = *modes++;
					int height = *modes++;
					if (width == TempOptions.ScreenWidth && height == TempOptions.ScreenHeight) {
						initial_mode = mode_index;
					}
					char buffer[64];
					sprintf(buffer, "%d x %d", width, height);
					int index = ListBox_AddString(list, buffer);
					ListBox_SetItemData(list, index, item_index);
					mode_index++;
					item_index++;
				}
			}
			ListBox_SetCurSel(list, initial_mode);
			_initialized = true;
			_current_mode = initial_mode;
			_previous_mode = initial_mode;

			HWND button = GetDlgItem(window, IDC_STRETCH_MOVIES);
			if (button) {
				Button_SetCheck(button, Options.StretchMovies != false);
			}
		}
		break;

	}
	return(0);
}


/// <summary>
/// Handles the display options dialog.
/// This routine gives the owner draw dialog system first refusal on the message and only
/// deals with what it leaves behind.
/// </summary>
BOOL CALLBACK Display_Options_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	int rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);
	if (rc == 0) {
		return(Display_Options_Dialog_Body(window, message, wparam));
	}
	return(rc);
}
