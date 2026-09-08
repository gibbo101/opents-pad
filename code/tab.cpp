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

/* $Header: /CounterStrike/TAB.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : TAB.CPP                                                      *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 12/15/94                                                     *
 *                                                                                             *
 *                  Last Update : September 20, 1995 [JLB]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   TabClass::AI -- Handles player I/O with the tab buttons.                                  *
 *   TabClass::Draw_It -- Displays the tab buttons as necessary.                               *
 *   TabClass::One_Time -- Performs one time initialization of tab handler class.              *
 *   TabClass::Set_Active -- Activates a "filefolder tab" button.                              *
 *   TabClass::TabClass -- Default construct for the tab button class.                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "tab.h"

#include "_convert.h"
#include "_map.h"
#include "_mixfile.h"
#include "_rules.h"
#include "_surface.h"
#include "dialog.h"
#include "draw.h"
#include "_font.h"
#include "data.h"
#include "font.h"
#include "goptions.h"
#include "padglyph.h"
#include "language/language.h"
#include "mixfile.h"
#include "queue.h"
#include "rules.h"
#include "savestream.h"
#include "scenario.h"
#include "scheme.h"
#include "shapeset.h"
#include "surface.h"
#include "video.h"

ShapeSet const * TabClass::TabShape = NULL;


/***********************************************************************************************
 * TabClass::TabClass -- Default construct for the tab button class.                           *
 *                                                                                             *
 *    The default constructor merely sets the tab buttons to default non-selected state.       *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/15/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
TabClass::TabClass(void) :
	FlasherTimer(0),
	IsToRedraw(false),
	MoneyFlashTimer(0)
{
}


/// <summary>
/// Lists the members the tab bar holds.
/// </summary>
/// <param name="stream">The stream carrying the members.</param>
void TabClass::Serialize(SaveStreamClass & stream)
{
	BASECLASS::Serialize(stream);

	stream.Serialize(Credits);
	stream.Serialize(FlasherTimer);

	// IsToRedraw -- a redraw flag; the load asks for a complete draw anyway.
	stream.Serialize(MoneyFlashTimer);
	// TabShape -- artwork fetched by One_Time.
}


/***********************************************************************************************
 * TabClass::Draw_It -- Displays the tab buttons as necessary.                                 *
 *                                                                                             *
 *    This routine is called whenever the display is being redrawn (in some fashion). The      *
 *    parameter can be used to force the tab buttons to redraw completely. The default action  *
 *    is to only redraw if the tab buttons have been explicitly flagged to be redraw. The      *
 *    result of this is the elimination of unnecessary redraws.                                *
 *                                                                                             *
 * INPUT:   complete -- bool; Force redraw of the entire tab button graphics?                  *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/15/1994 JLB : Created.                                                                 *
 *   05/19/1995 JLB : New EVA style.                                                           *
 *=============================================================================================*/
#define	EVA_WIDTH		80
#define	TAB_HEIGHT		8
void TabClass::Draw_It(bool complete)
{
	if (!Debug_Map) {

		/*
		**	Redraw the top bar imagery if flagged to do so or if the entire display needs
		**	to be redrawn.
		*/
		if (complete || IsToRedraw) {

			// A split bar is drawn on its own surface at its own width and never copied
			// into the frame; the presenter draws it over the map's top edge.
			Surface & bar = TabSurface != NULL ? *TabSurface : *LogicalSurface;
			int barwidth = TabSurface != NULL ? TabSurface->Get_Width() : CompositeSurface->Get_Width();
			int width  = TabSurface != NULL ? barwidth : CompositeSurface->Get_Width() + SidebarSurface->Get_Width();
			int rightx = width - 1;
			int tab_height = TAB_HEIGHT * 2/*RESFACTOR*/;

			for (int x = TabShape->Get_Width(); x < barwidth; x += TabShape->Get_Width()) {
				Draw_Shape(bar, *SidebarDrawer, TabShape, 1, Point2D(x, 0), bar.Get_Rect());
			}

			int sidex = Options.IsSidebarOnRight ? 0 : barwidth - EVA_WIDTH * 2/*RESFACTOR*/;

			Draw_Shape(bar, *SidebarDrawer, TabShape, 0, Point2D(sidex, 0), bar.Get_Rect());
			Draw_Credits_Tab();
			bar.Draw_Line(Point2D(0, tab_height-(1* 2)), Point2D(rightx, tab_height-(1 * 2/*RESFACTOR*/)), TBLACK);

			// Under the controller scheme the pad's menu button glyph sits before the label,
			// the pair centred on the tab; a text prompt style draws no glyph.
			int centre = sidex + (EVA_WIDTH/2) * 2/*RESFACTOR*/;
			if (Options.ControlScheme == CONTROL_CONTROLLER && Metal12FontPtr != NULL) {
				enum { GLYPH = 16, GAP = 4 };
				int textwidth = Metal12FontPtr->String_Pixel_Width(Fetch_String(TXT_TAB_BUTTON_CONTROLS));
				int left = centre - (GLYPH + GAP + textwidth) / 2;
				int glyph = Draw_Pad_Glyph(bar, PAD_BUTTON_MENU, left, (tab_height - GLYPH) / 2, GLYPH);
				if (glyph > 0) {
					centre = left + glyph + GAP + textwidth / 2;
				}
			}
			Fancy_Text_Print(TXT_TAB_BUTTON_CONTROLS, bar, bar.Get_Rect(), Point2D(centre, 0), ColorSchemes[0], TBLACK, TextPrintType(TPF_USE_GRAD_PAL | TPF_CENTER | TPF_METAL12));

			if (TabSurface != NULL) {
				Video_Mark_Dirty();
			} else if (LogicalSurface != TileSurface) {
				TileSurface->Blit_From(Rect(0, 0, TileSurface->Get_Width(), tab_height), *LogicalSurface, Rect(0, 0, TileSurface->Get_Width(), tab_height));
			}
		}
	}

	if (!Debug_Map) {
		Credits.Graphic_Logic(complete || IsToRedraw);
		IsToRedraw = false;
	}

	BASECLASS::Draw_It(complete);
}


/// <summary>
/// Draws the tab backdrop for the credits and the mission timer.
/// This routine lays down the sidebar tab imagery that the credits readout is printed
/// over, and prints the mission timer alongside it whenever a timer is running. The
/// credit display calls this before it prints the new money value.
/// </summary>
// Where the timer's right edge falls on the split bar, in its own pixels: left of the
// Sidebar tab at the bar's end, or of the sidebar itself once it has slid over that tab.
int TabClass::Bar_Timer_Right(void)
{
	int width = TabSurface->Get_Width();
	int uncovered = width - int(SidebarClass::SIDE_WIDTH * Video_Sidebar_Slide());
	return(std::min(uncovered, width - TabShape->Get_Width()));
}


// The Sidebar tab: the tab art, the pad's fourth button glyph and the label, the pair
// centred, at x on the given surface.
void TabClass::Draw_Sidebar_Tab(Surface & surface, int x)
{
	enum { GLYPH = 16, GAP = 4, HEIGHT = 16 };
	Draw_Shape(surface, *SidebarDrawer, TabShape, 2, Point2D(x, 0), surface.Get_Rect());
	int centre = x + TabShape->Get_Width() / 2;
	if (Metal12FontPtr != NULL) {
		int textwidth = Metal12FontPtr->String_Pixel_Width(Fetch_String(TXT_TAB_SIDEBAR));
		int left = centre - (GLYPH + GAP + textwidth) / 2;
		int glyph = Draw_Pad_Glyph(surface, PAD_BUTTON_FOURTH, left, (HEIGHT - GLYPH) / 2, GLYPH);
		if (glyph > 0) {
			centre = left + glyph + GAP + textwidth / 2;
		}
	}
	Fancy_Text_Print(TXT_TAB_SIDEBAR, surface, surface.Get_Rect(), Point2D(centre, 0), ColorSchemes[0], TBLACK, TextPrintType(TPF_USE_GRAD_PAL | TPF_CENTER | TPF_METAL12));
}


void TabClass::Draw_Credits_Tab(void)
{
	Draw_Shape(*SidebarSurface, *SidebarDrawer, TabShape, 2, Point2D(0, 0), SidebarSurface->Get_Rect());

	// The split bar ends in the Sidebar tab, which the sidebar covers when it is in; the
	// credits are the sidebar's alone, as on the PlayStation.
	if (TabSurface != NULL) {
		Draw_Sidebar_Tab(*TabSurface, TabSurface->Get_Width() - TabShape->Get_Width());
		Video_Mark_Dirty();
	}

	if (Scen->MissionTimer.Is_Active()) {
		bool light = ((int)Scen->MissionTimer < TICKS_PER_MINUTE * Rule->TimerWarning) || Map.FlasherTimer > 0;
		// The timer's tab sits at the bar's right end, or on the split bar short of what
		// else is there.
		Surface & bar = TabSurface != NULL ? *TabSurface : *CompositeSurface;
		int barwidth = TabSurface != NULL ? Bar_Timer_Right() : TacticalRect.Width;
		Draw_Shape(bar, *SidebarDrawer, TabShape, /*light ? 4 :*/ 2, Point2D(barwidth - TabShape->Get_Width(), 0), bar.Get_Rect());

		int time = Scen->MissionTimer;

		int seconds = time / TICKS_PER_SECOND;
		int hours = seconds / 60 / 60;
		int minutes = seconds / 60;

		seconds = seconds % 60;
		minutes = minutes % 60;

		if (hours != 0) {
			Fancy_Text_Print(TXT_TIME_FORMAT_HOURS, bar, bar.Get_Rect(),
				Point2D(barwidth - TabShape->Get_Width() / 2, 0), ColorSchemes[0], TBLACK,
				TextPrintType(TPF_METAL12 | TPF_CENTER | TPF_USE_GRAD_PAL), hours, minutes, seconds);
		} else {
			Fancy_Text_Print(TXT_TIME_FORMAT_NO_HOURS, bar, bar.Get_Rect(),
				Point2D(barwidth - TabShape->Get_Width() / 2, 0), ColorSchemes[0], TBLACK,
				TextPrintType(TPF_METAL12 | TPF_CENTER | TPF_USE_GRAD_PAL), minutes, seconds);
		}
		if (TabSurface != NULL) {
			Video_Mark_Dirty();
		}
	}
	BASECLASS::IsToBlitSidebar = true;
}


/// <summary>
/// Prints the credits readout over its tab on the sidebar's strip.
/// </summary>
void TabClass::Print_Credits(char const * text)
{
	Fancy_Text_Print(text, *SidebarSurface, SidebarSurface->Get_Rect(), Point2D(SidebarSurface->Get_Width() / 2, 0), ColorSchemes[0], TBLACK, TextPrintType(TPF_USE_GRAD_PAL | TPF_CENTER | TPF_METAL12));
}


/// <summary>
/// Draws the specified tab in its highlighted state.
/// This routine is used to give the player some feedback while a tab is being pressed.
/// The tab imagery and its label are redrawn in the highlight style.
/// </summary>
/// <param name="tab">The tab to highlight; zero for the controls tab, non-zero for the
/// sidebar tab.</param>
void TabClass::Hilite_Tab(int tab)
{
	int xpos = 0;
	int text = TXT_TAB_BUTTON_CONTROLS;
	int textx = (EVA_WIDTH/2) * 2;

	if (tab) {
		xpos = (320-EVA_WIDTH) * 2;
		//text = TXT_TAB_SIDEBAR;
		//textx = (320-(EVA_WIDTH/2)) * 2;
	} else {
		xpos = Options.IsSidebarOnRight ? 0 : LogicalSurface->Get_Rect().Width - textx*2;
	}

	Draw_Shape(*LogicalSurface, *SidebarDrawer, TabShape, 1, Point2D(xpos, 0), VisibleRect);
	Fancy_Text_Print(text, *LogicalSurface, LogicalSurface->Get_Rect(), Point2D(xpos + textx, 0), ColorSchemes[0], TBLACK, TextPrintType(TPF_METAL12 | TPF_CENTER | TPF_USE_GRAD_PAL));
}


/***********************************************************************************************
 * TabClass::AI -- Handles player I/O with the tab buttons.                                    *
 *                                                                                             *
 *    This routine is called every game tick and passed whatever key the player has supplied.  *
 *    If the input selects a tab button, then the graphic gets updated accordingly.            *
 *                                                                                             *
 * INPUT:   input -- The player's input character (might be mouse click).                      *
 *                                                                                             *
 *          x,y   -- Mouse coordinates at time of input.                                       *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/15/1994 JLB : Created.                                                                 *
 *   12/31/1994 JLB : Uses mouse coordinate parameters.                                        *
 *   05/31/1995 JLB : Fixed to handle mouse shape properly.                                    *
 *   08/25/1995 JLB : Handles new scrolling option.                                            *
 *=============================================================================================*/
void TabClass::AI(KeyNumType &input, Point2D const & xy)
{
	if (!Map.IsRubberBand) {

		if (xy.Y >= 0 && xy.Y < (TAB_HEIGHT * 2/*RESFACTOR*/) && xy.X < (VisibleSurface->Get_Width() - 1) && xy.X > 0) {

			bool 	ok = false;

			/*
			**	If the mouse is at the top of the screen, then the tab bars only work
			**	in certain areas. If the special scroll modification is not active, then
			**	the tabs never work when the mouse is at the top of the screen.
			*/
			if (xy.Y > 0) {
				ok = true;
			}

			if (ok) {
				if (input == KN_LMOUSE) {
					int sel = 0;
					// A split bar's positions arrive brought onto the frame's map columns, so
					// the tab's width is brought onto them too, and the Sidebar tab at the
					// bar's far end slides the panel in.
					int tabwidth = EVA_WIDTH * 2/*RESFACTOR*/;
					if (TabSurface != NULL && TabSurface->Get_Width() > 0) {
						tabwidth = tabwidth * TacticalRect.Width / TabSurface->Get_Width();
						int sidebartab = (TabSurface->Get_Width() - TabShape->Get_Width()) * TacticalRect.Width / TabSurface->Get_Width();
						if (xy.X >= sidebartab && xy.X < TacticalRect.X + TacticalRect.Width) {
							Pad_Panel_Show(false, false);
							input = KN_NONE;
							return;
						}
					}
					if (Options.IsSidebarOnRight) {
						if (xy.X >= tabwidth) sel = -1;
					} else {
						if (xy.X <= VisibleRect.Width - tabwidth || xy.X >= VisibleRect.Width) sel = -1;
					}
					if (sel >= 0) {
						Set_Active(sel);
						input = KN_NONE;
					}
				}

				Override_Mouse_Shape(MOUSE_NORMAL, false);
			}
		}
	}

	if (MoneyFlashTimer == 1) {
		IsToRedraw = true;
		Flag_To_Redraw();
	}

	Credits.AI();
	BASECLASS::AI(input, xy);
}


/***********************************************************************************************
 * TabClass::Set_Active -- Activates a "filefolder tab" button.                                *
 *                                                                                             *
 *    This function is used to activate one of the file folder tab buttons that appear at the  *
 *    top edge of the screen.                                                                  *
 *                                                                                             *
 * INPUT:   select   -- The button to activate. 0 = left button, 1=next button, etc.           *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/15/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void TabClass::Set_Active(int select)
{
	switch (select) {
		case 0:
			Queue_Options();
			break;

		case 1:
			BASECLASS::Activate(-1);
			break;

		default:
			break;
	}
}


/***********************************************************************************************
 * TabClass::One_Time -- Performs one time initialization of tab handler class.                *
 *                                                                                             *
 *    This routine will perform any one time initializations of the tab handler class. This    *
 *    typically includes the loading of the shapes that appear on it.                          *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/20/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void TabClass::One_Time(void)
{
	BASECLASS::One_Time();
}


/// <summary>
/// Initializes the tab bar for the player's house.
/// This routine is called once the player's house has been established. It fetches the
/// tab artwork and resets the credits readout so that it will tick up from nothing.
/// </summary>
void TabClass::Init_For_House(void)
{
	BASECLASS::Init_For_House();
	TabShape = (ShapeSet const *)MixFileClass::Retrieve("TABS.SHP");
	Credits.Current = 0;
}


/// <summary>
/// Flashes the credits readout to catch the player's eye.
/// This routine is used when something has happened that the player really ought to
/// notice about the state of his funds, such as running short of cash. The tab bar is
/// flagged for redraw and the money display pulses for a moment before settling down.
/// </summary>
void TabClass::Flash_Money(void)
{
	IsToRedraw = true;
	Flag_To_Redraw();
	MoneyFlashTimer = 7;
}
