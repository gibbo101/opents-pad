/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "vidscale.h"

#include "video.h"
#include "win.h"

#include <cmath>


/// <summary>
/// Is the frame drawn at some size or position other than the window's own?
/// </summary>
/// <returns>bool; Do window positions need converting before the game sees them?</returns>
bool Video_Scaling_Active(void)
{
	VideoScaleInfo const & scale = Video_Get_Scale_Info();

	return(scale.Is_Split() || scale.DestX != 0 || scale.DestY != 0 || scale.DestWidth != scale.GameWidth || scale.DestHeight != scale.GameHeight);
}


// An overlay sidebar only takes positions once fully in.
static bool Sidebar_Takes_Points(VideoScaleInfo const & scale)
{
	return(scale.Is_Split() && (!scale.SidebarOverlay || Video_Sidebar_Slide() >= 1.0f));
}


// Everything past the seam counts as sidebar.
static bool Window_Point_On_Sidebar(VideoScaleInfo const & scale, POINT const & point)
{
	if (!Sidebar_Takes_Points(scale)) {
		return(false);
	}
	return(scale.SidebarOnRight ? point.x >= scale.SidebarDestX : point.x < scale.SidebarDestX + scale.SidebarDestWidth);
}


// A frame position beyond the frame's edge on the sidebar's side counts as sidebar too.
static bool Game_Point_On_Sidebar(VideoScaleInfo const & scale, POINT const & point)
{
	if (!Sidebar_Takes_Points(scale)) {
		return(false);
	}
	return(scale.SidebarOnRight ? point.x >= scale.Sidebar_X() : point.x < scale.SidebarWidth);
}


static bool Window_Point_On_Bar(VideoScaleInfo const & scale, POINT const & point)
{
	if (!scale.Bar_Is_Split()) {
		return(false);
	}
	return(point.y < scale.BarDestY + scale.BarDestHeight);
}


static bool Game_Point_On_Bar(VideoScaleInfo const & scale, POINT const & point)
{
	return(scale.Bar_Is_Split() && point.y < scale.BarHeight);
}


/// <summary>
/// Converts a position in the window's client area into one in the frame.
/// A position on one of the letterbox bars lands outside the frame rather than being
/// pulled onto its edge.
/// </summary>
/// <param name="point">The position to convert in place.</param>
void Window_Point_To_Game(POINT & point)
{
	VideoScaleInfo const & scale = Video_Get_Scale_Info();

	if (Window_Point_On_Sidebar(scale, point) && scale.SidebarScale > 0.0f) {
		point.x = scale.Sidebar_X() + (LONG)floor((point.x - scale.SidebarDestX) / (double)scale.SidebarScale);
		point.y = (LONG)floor((point.y - scale.SidebarDestY) / (double)scale.SidebarScale);
		return;
	}

	if (Window_Point_On_Bar(scale, point) && scale.BarDestWidth > 0 && scale.SidebarScale > 0.0f) {
		int barx = (int)floor((point.x - scale.BarDestX) * (double)scale.BarWidth / (double)scale.BarDestWidth);
		point.x = scale.Bar_To_Frame_X(barx);
		point.y = (LONG)floor((point.y - scale.BarDestY) / (double)scale.SidebarScale);
		if (point.y >= scale.BarHeight) point.y = scale.BarHeight - 1;
		return;
	}

	if (scale.DestWidth > 0 && scale.DestHeight > 0) {
		point.x = scale.Tactical_X() + (LONG)floor((point.x - scale.DestX) * (double)scale.Tactical_Width() / (double)scale.DestWidth);
		point.y = scale.BarHeight + (LONG)floor((point.y - scale.DestY) * (double)scale.Tactical_Height() / (double)scale.DestHeight);
	}
}


/// <summary>
/// Converts a position in the frame into one in the window's client area.
/// </summary>
/// <param name="point">The position to convert in place. It comes back at the top left
/// corner of the area the frame pixel covers on screen.</param>
void Game_Point_To_Window(POINT & point)
{
	VideoScaleInfo const & scale = Video_Get_Scale_Info();

	if (Game_Point_On_Sidebar(scale, point)) {
		point.x = scale.SidebarDestX + (LONG)floor((point.x - scale.Sidebar_X()) * (double)scale.SidebarScale);
		point.y = scale.SidebarDestY + (LONG)floor(point.y * (double)scale.SidebarScale);
		return;
	}

	if (Game_Point_On_Bar(scale, point) && scale.Tactical_Width() > 0) {
		int barx = scale.Frame_To_Bar_X(point.x);
		point.x = scale.BarDestX + (LONG)floor(barx * (double)scale.BarDestWidth / (double)scale.BarWidth);
		point.y = scale.BarDestY + (LONG)floor(point.y * (double)scale.SidebarScale);
		return;
	}

	if (scale.Tactical_Width() > 0 && scale.Tactical_Height() > 0) {
		point.x = scale.DestX + (LONG)floor((point.x - scale.Tactical_X()) * (double)scale.DestWidth / (double)scale.Tactical_Width());
		point.y = scale.DestY + (LONG)floor((point.y - scale.BarHeight) * (double)scale.DestHeight / (double)scale.Tactical_Height());
	}
}


/// <summary>
/// Converts a position on the desktop into one in the frame.
/// </summary>
/// <param name="point">The position to convert in place.</param>
void Screen_Point_To_Game(POINT & point)
{
	ScreenToClient(MainWindow, &point);
	Window_Point_To_Game(point);
}


/// <summary>
/// Converts a position in the frame into one on the desktop.
/// </summary>
/// <param name="point">The position to convert in place.</param>
void Game_Point_To_Screen(POINT & point)
{
	Game_Point_To_Window(point);
	ClientToScreen(MainWindow, &point);
}


/// <summary>
/// Pulls a position onto the frame if it lies outside it.
/// A split sidebar's columns run to the sidebar's own height rather than the frame's, so
/// a position in them is held to that.
/// </summary>
/// <param name="point">The position to clamp in place.</param>
void Clamp_To_Game(POINT & point)
{
	VideoScaleInfo const & scale = Video_Get_Scale_Info();

	if (point.x < 0) point.x = 0;
	if (point.y < 0) point.y = 0;
	if (scale.GameWidth > 0 && point.x >= scale.GameWidth) point.x = scale.GameWidth - 1;

	int height = Game_Point_On_Sidebar(scale, point) ? scale.SidebarHeight : scale.GameHeight;
	if (height > 0 && point.y >= height) point.y = height - 1;
}


/// <summary>
/// Fetches where the cursor is, in the frame.
/// </summary>
/// <param name="window">A dialog control to report the position relative to, or NULL for
/// the frame itself. The controls sit at positions in the frame rather than in the
/// window, so their own client coordinates are frame coordinates too.</param>
/// <param name="point">Receives the position, pulled onto the frame.</param>
void Get_Logical_Cursor_Pos(HWND window, POINT & point)
{
	GetCursorPos(&point);
	Screen_Point_To_Game(point);
	Clamp_To_Game(point);

	if (window != NULL && window != MainWindow) {
		RECT window_rect;
		GetWindowRect(window, &window_rect);

		POINT origin;
		origin.x = 0;
		origin.y = 0;
		ClientToScreen(MainWindow, &origin);

		point.x -= window_rect.left - origin.x;
		point.y -= window_rect.top - origin.y;
	}
}
