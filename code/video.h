/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "nativewindow.hh"


// How the presented frame is filtered when the window is larger than it.
enum VideoScaleMode {
	VIDEO_SCALE_NEAREST,
	VIDEO_SCALE_LINEAR,
	VIDEO_SCALE_PIXELART,
};


// Where the game's frame lands inside the window. The frame keeps its aspect ratio, so
// the destination is centered and the window may show bars on two of its sides.
// Drawable dimensions and the destination rectangle are measured in physical pixels.
// Split layout: the sidebar and bar present from their own textures at SidebarScale;
// Dest* covers only the frame's tactical region.
struct VideoScaleInfo
{
	int GameWidth;
	int GameHeight;
	int DrawableWidth;
	int DrawableHeight;
	int DestX;
	int DestY;
	int DestWidth;
	int DestHeight;
	float ScaleX;
	float ScaleY;

	int SidebarWidth;			// Zero while the sidebar is presented as part of the frame.
	int SidebarHeight;
	bool SidebarOnRight;
	int SidebarDestX;
	int SidebarDestY;
	int SidebarDestWidth;
	int SidebarDestHeight;
	float SidebarScale;

	int BarHeight;				// Frame rows given to the split bar; zero while it is in the frame.
	int BarWidth;				// The bar surface's own width, fixed when the bar is split off.
	int BarDestX;
	int BarDestY;
	int BarDestWidth;
	int BarDestHeight;

	bool SidebarOverlay;		// The sidebar slides over the frame rather than sitting beside it.

	bool Is_Split(void) const { return(SidebarWidth > 0 && SidebarHeight > 0); }
	bool Sidebar_Is_Beside(void) const { return(Is_Split() && !SidebarOverlay); }
	bool Bar_Is_Split(void) const { return(Is_Split() && BarHeight > 0 && BarWidth > 0); }
	int Sidebar_X(void) const { return(SidebarOnRight ? GameWidth - SidebarWidth : 0); }
	int Tactical_X(void) const { return(SidebarOnRight ? 0 : SidebarWidth); }
	int Tactical_Width(void) const { return(GameWidth - SidebarWidth); }
	int Tactical_Height(void) const { return(GameHeight - BarHeight); }

	// The frame column a split bar's column stands over, and back. Both are identity
	// while the bar is in the frame.
	int Bar_To_Frame_X(int x) const { return(Bar_Is_Split() ? Tactical_X() + x * Tactical_Width() / BarWidth : x); }
	int Frame_To_Bar_X(int x) const { return(Bar_Is_Split() && Tactical_Width() > 0 ? (x - Tactical_X()) * BarWidth / Tactical_Width() : x); }
};


bool Video_Init(NativeWindow const & window, int drawablewidth, int drawableheight, int refreshrate);
void Video_Shutdown(void);

bool Video_Set_Mode(int width, int height);
bool Video_Set_Sidebar(int width, int height, bool onright, int barheight, bool overlay);
void Video_Slide_Sidebar(bool in, int milliseconds);
bool Video_Sidebar_Sliding(void);
float Video_Sidebar_Slide(void);
bool Video_Sidebar_Is_Split(void);
void Video_On_Resize(int drawablewidth, int drawableheight);
void Video_Set_Refresh_Rate(int refreshrate);

void Video_Set_Movie(bool on);
void Video_Mark_Dirty(void);
void Video_Present(void);
void Video_Present_If_Dirty(void);

VideoScaleInfo const & Video_Get_Scale_Info(void);

int * EnumDisplayModes(int minwidth, int minheight, int maxwidth, int maxheight);
