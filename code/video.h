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
//
// When the sidebar is split off, the frame's sidebar columns are never shown: the
// destination covers the frame's other columns, and the sidebar surface, at its own size,
// is drawn beside them at its own scale so it fills the drawable height.
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

	bool Is_Split(void) const { return(SidebarWidth > 0 && SidebarHeight > 0); }
	int Sidebar_X(void) const { return(SidebarOnRight ? GameWidth - SidebarWidth : 0); }
	int Tactical_X(void) const { return(SidebarOnRight ? 0 : SidebarWidth); }
	int Tactical_Width(void) const { return(GameWidth - SidebarWidth); }
};


bool Video_Init(NativeWindow const & window, int drawablewidth, int drawableheight, int refreshrate);
void Video_Shutdown(void);

bool Video_Set_Mode(int width, int height);
bool Video_Set_Sidebar(int width, int height, bool onright);
bool Video_Sidebar_Is_Split(void);
void Video_On_Resize(int drawablewidth, int drawableheight);
void Video_Set_Refresh_Rate(int refreshrate);

void Video_Mark_Dirty(void);
void Video_Present(void);
void Video_Present_If_Dirty(void);

VideoScaleInfo const & Video_Get_Scale_Info(void);

int * EnumDisplayModes(int minwidth, int minheight, int maxwidth, int maxheight);
