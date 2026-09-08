/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// The engine's side of the presenter. The game draws its frame into the visible surface
// as it always has; this decides when that frame reaches the screen and where in the
// window it lands, and hands it to the renderer behind video.h.

#include "always.h"

#include "video.h"

#include "_surface.h"
#include "bgfxbackend.h"
#include "dbgprint.h"
#include "dsurface.h"
#include "globals.h"
#include "goptions.h"
#include "misc.h"
#include "surface.h"
#include "wincursor.h"

#include <algorithm>
#include <cstdlib>


/*
 * The size of the frame the game renders into. It is not tied to the window, which may be
 * any size, nor to the desktop, whose mode the game no longer changes.
 */
int VideoModeWidth = 0;
int VideoModeHeight = 0;

/*
 * Is the game running in a framed, resizable window rather than in a borderless one
 * covering the whole screen? The display mode is never changed either way.
 */
bool WindowedMode = false;

static bool _Initialized = false;
static VideoScaleInfo _ScaleInfo;

// The sidebar the presenter draws beside the frame, or nothing while it is in the frame.
static int _SidebarWidth = 0;
static int _SidebarHeight = 0;
static bool _SidebarOnRight = true;
static int _BarHeight = 0;
static bool _SidebarOverlay = false;

// The overlay sidebar's slide: where it is going, where it set out from, and when.
static float _SlideTo = 0.0f;
static float _SlideFrom = 0.0f;
static unsigned long _SlideStart = 0;
static unsigned long _SlideMs = 0;

// Set whenever the visible surface is written to, and cleared once that frame has been
// presented. A frame that is skipped for pacing stays marked, so the next present shows
// the newest content rather than a stale one.
static bool _FrameIsDirty = false;
static unsigned int _LastPresentTime = 0;
static unsigned int _PresentInterval = 16;

// Presents can nest, because a dialog repainting itself presents from inside the paint
// that the engine's own present provoked.
static bool _Presenting = false;


/// <summary>
/// Works out the shortest sensible gap between presents from the display's refresh rate.
/// </summary>
static void Update_Present_Interval(int refreshrate)
{
	if (refreshrate <= 1) {
		refreshrate = 60;
	}

	_PresentInterval = (unsigned int)(1000 / refreshrate);
	if (_PresentInterval < 3) {
		_PresentInterval = 3;
	}
	if (_PresentInterval > 100) {
		_PresentInterval = 100;
	}
}


static double Fit_Scale(int width, int height, int intowidth, int intoheight)
{
	double scalex = (double)intowidth / (double)width;
	double scaley = (double)intoheight / (double)height;
	double scale = (scalex < scaley) ? scalex : scaley;

	if (Options.IntegerScaling && scale >= 1.0) {
		scale = (double)(int)scale;
	}
	return(scale);
}


/// <summary>
/// Works out where the game's frame sits inside the window.
/// The frame keeps its shape, so it is grown by whichever of the two axes runs out first
/// and centered in what is left over. A split sidebar is fitted to the drawable height
/// first and the frame's other columns are fitted beside it.
/// </summary>
static void Update_Scale_Info(void)
{
	_ScaleInfo.GameWidth = VideoModeWidth;
	_ScaleInfo.GameHeight = VideoModeHeight;
	_ScaleInfo.SidebarWidth = 0;
	_ScaleInfo.SidebarHeight = 0;
	_ScaleInfo.SidebarOnRight = _SidebarOnRight;
	_ScaleInfo.SidebarDestX = 0;
	_ScaleInfo.SidebarDestY = 0;
	_ScaleInfo.SidebarDestWidth = 0;
	_ScaleInfo.SidebarDestHeight = 0;
	_ScaleInfo.SidebarScale = 1.0f;
	_ScaleInfo.BarHeight = 0;
	_ScaleInfo.BarWidth = 0;
	_ScaleInfo.BarDestX = 0;
	_ScaleInfo.BarDestY = 0;
	_ScaleInfo.BarDestWidth = 0;
	_ScaleInfo.BarDestHeight = 0;
	_ScaleInfo.SidebarOverlay = false;

	if (_ScaleInfo.GameWidth <= 0 || _ScaleInfo.GameHeight <= 0 || _ScaleInfo.DrawableWidth <= 0 || _ScaleInfo.DrawableHeight <= 0) {
		_ScaleInfo.DestX = 0;
		_ScaleInfo.DestY = 0;
		_ScaleInfo.DestWidth = _ScaleInfo.DrawableWidth;
		_ScaleInfo.DestHeight = _ScaleInfo.DrawableHeight;
		_ScaleInfo.ScaleX = 1.0f;
		_ScaleInfo.ScaleY = 1.0f;
		return;
	}

	int intox = 0;
	int intoy = 0;
	int intowidth = _ScaleInfo.DrawableWidth;
	int intoheight = _ScaleInfo.DrawableHeight;
	int framewidth = _ScaleInfo.GameWidth;
	int frameheight = _ScaleInfo.GameHeight;

	if (_SidebarWidth > 0 && _SidebarHeight > 0 && _SidebarWidth < _ScaleInfo.GameWidth) {
		double scale = Fit_Scale(_SidebarWidth, _SidebarHeight, _ScaleInfo.DrawableWidth, _ScaleInfo.DrawableHeight);
		_ScaleInfo.SidebarWidth = _SidebarWidth;
		_ScaleInfo.SidebarHeight = _SidebarHeight;
		_ScaleInfo.SidebarDestWidth = (int)((double)_SidebarWidth * scale);
		_ScaleInfo.SidebarDestHeight = (int)((double)_SidebarHeight * scale);
		_ScaleInfo.SidebarDestY = (_ScaleInfo.DrawableHeight - _ScaleInfo.SidebarDestHeight) / 2;
		_ScaleInfo.SidebarScale = (float)scale;
		_ScaleInfo.SidebarOverlay = _SidebarOverlay;

		framewidth = _ScaleInfo.Tactical_Width();
		if (_SidebarOnRight) {
			_ScaleInfo.SidebarDestX = _ScaleInfo.DrawableWidth - _ScaleInfo.SidebarDestWidth;
		} else {
			_ScaleInfo.SidebarDestX = 0;
		}
		// Beside the frame the sidebar takes its share of the width; over it, none.
		if (!_SidebarOverlay) {
			intowidth = std::max(_ScaleInfo.DrawableWidth - _ScaleInfo.SidebarDestWidth, 1);
			if (!_SidebarOnRight) {
				intox = _ScaleInfo.SidebarDestWidth;
			}
		}

		// The bar takes its rows off the top of the frame and is drawn at the sidebar's
		// scale across the whole drawable; its own width follows from that.
		if (_BarHeight > 0 && _BarHeight < _ScaleInfo.GameHeight) {
			_ScaleInfo.BarHeight = _BarHeight;
			_ScaleInfo.BarWidth = std::max((int)((double)_ScaleInfo.DrawableWidth / scale), 1);
			_ScaleInfo.BarDestX = 0;
			_ScaleInfo.BarDestY = 0;
			_ScaleInfo.BarDestWidth = (int)((double)_ScaleInfo.BarWidth * scale);
			_ScaleInfo.BarDestHeight = (int)((double)_BarHeight * scale);
			intoy = _ScaleInfo.BarDestHeight;
			intoheight = std::max(_ScaleInfo.DrawableHeight - _ScaleInfo.BarDestHeight, 1);
			frameheight = _ScaleInfo.Tactical_Height();
		}
	}

	double scale = Fit_Scale(framewidth, frameheight, intowidth, intoheight);

	_ScaleInfo.DestWidth = (int)((double)framewidth * scale);
	_ScaleInfo.DestHeight = (int)((double)frameheight * scale);
	_ScaleInfo.DestX = intox + (intowidth - _ScaleInfo.DestWidth) / 2;
	_ScaleInfo.DestY = intoy + (intoheight - _ScaleInfo.DestHeight) / 2;
	_ScaleInfo.ScaleX = (float)((double)_ScaleInfo.DestWidth / (double)framewidth);
	_ScaleInfo.ScaleY = (float)((double)_ScaleInfo.DestHeight / (double)frameheight);
}


// Sizes the renderer's textures to the frame columns that are shown and to the split
// sidebar, if there is one, then lays both out in the window.
static bool Apply_Layout(void)
{
	Update_Scale_Info();

	if (!Backend_Set_Frame_Size(_ScaleInfo.Tactical_Width(), _ScaleInfo.Tactical_Height())) {
		return(false);
	}
	if (!Backend_Set_Sidebar_Size(_ScaleInfo.SidebarWidth, _ScaleInfo.SidebarHeight)) {
		return(false);
	}
	if (!Backend_Set_Bar_Size(_ScaleInfo.BarWidth, _ScaleInfo.BarHeight)) {
		return(false);
	}

	Win_Cursor_Refresh();
	_FrameIsDirty = true;
	return(true);
}


/// <summary>
/// Converts the configured filter into the one the renderer names.
/// </summary>
static BackendScaleMode Backend_Scale_Mode(void)
{
	switch (Options.ScaleMode) {
		case VIDEO_SCALE_LINEAR:
			return(BACKEND_SCALE_LINEAR);

		case VIDEO_SCALE_NEAREST:
			return(BACKEND_SCALE_NEAREST);

		default:
			return(BACKEND_SCALE_PIXELART);
	}
}


/// <summary>
/// Starts the presenter on the game's window.
/// </summary>
/// <param name="window">The native window whose drawable area receives the frame.</param>
/// <param name="drawablewidth">The drawable area's width in physical pixels.</param>
/// <param name="drawableheight">The drawable area's height in physical pixels.</param>
/// <param name="refreshrate">The display refresh rate in hertz, or zero when unknown.</param>
/// <returns>bool; Did the presenter start? A false return is fatal to the game.</returns>
bool Video_Init(NativeWindow const & window, int drawablewidth, int drawableheight, int refreshrate)
{
	if (_Initialized) {
		return(true);
	}

	if (window.Handle == nullptr || drawablewidth <= 0 || drawableheight <= 0) {
		return(false);
	}

	_ScaleInfo.DrawableWidth = drawablewidth;
	_ScaleInfo.DrawableHeight = drawableheight;

	BackendRenderer renderer = (BackendRenderer)Options.Renderer;
	if (!Backend_Init(window, drawablewidth, drawableheight, renderer, Options.VSync)) {
		return(false);
	}

	DebugString("Video: renderer is %s\n", Backend_Renderer_Name());

	_Initialized = true;

	if (!Backend_Set_Frame_Size(VideoModeWidth, VideoModeHeight)) {
		Backend_Shutdown();
		_Initialized = false;
		return(false);
	}

	Update_Scale_Info();
	Update_Present_Interval(refreshrate);
	return(true);
}


/// <summary>
/// Stops the presenter and releases the renderer.
/// </summary>
void Video_Shutdown(void)
{
	if (!_Initialized) {
		return;
	}

	Win_Cursor_Shutdown();
	Backend_Shutdown();
	_Initialized = false;
	_FrameIsDirty = false;
}


/// <summary>
/// Moves the game to a different render resolution, with the sidebar back in the frame.
/// The caller replaces the surfaces afterwards; this only resizes what the frame is
/// presented from and leaves the previous mode untouched when it fails.
/// </summary>
/// <param name="width">The new frame width.</param>
/// <param name="height">The new frame height.</param>
/// <returns>bool; Was the mode changed?</returns>
bool Video_Set_Mode(int width, int height)
{
	if (!_Initialized || width <= 0 || height <= 0) {
		return(false);
	}

	if (!Backend_Set_Frame_Size(width, height)) {
		return(false);
	}

	VideoModeWidth = width;
	VideoModeHeight = height;
	_SidebarWidth = 0;
	_SidebarHeight = 0;
	_BarHeight = 0;
	_SidebarOverlay = false;

	Update_Scale_Info();
	Win_Cursor_Refresh();
	_FrameIsDirty = true;
	return(true);
}


/// <summary>
/// Splits the sidebar off the frame: the frame's sidebar columns are no longer shown and
/// the sidebar surface, at the given size, is presented beside the rest at whatever scale
/// fills the drawable height. A zero size puts the sidebar back in the frame. A bar height
/// splits the top bar off too: that many of the frame's top rows are no longer shown, and
/// a bar surface at the sidebar's scale is presented above the rest; its width is reported
/// as BarWidth in the scale info once this returns.
/// As an overlay the sidebar is drawn over the frame and the bar instead, slid in as far
/// as Video_Slide_Sidebar has taken it, and the frame's columns take the whole width.
/// The sidebar and bar surfaces the caller allocates must match the sizes given here.
/// </summary>
/// <returns>bool; Is the layout in place? On failure the previous layout stands.</returns>
bool Video_Set_Sidebar(int width, int height, bool onright, int barheight, bool overlay)
{
	if (!_Initialized) {
		return(false);
	}

	int oldwidth = _SidebarWidth;
	int oldheight = _SidebarHeight;
	bool oldright = _SidebarOnRight;
	int oldbar = _BarHeight;
	bool oldoverlay = _SidebarOverlay;

	_SidebarWidth = std::max(width, 0);
	_SidebarHeight = std::max(height, 0);
	_SidebarOnRight = onright;
	_BarHeight = std::max(barheight, 0);
	_SidebarOverlay = overlay;

	if (!Apply_Layout()) {
		_SidebarWidth = oldwidth;
		_SidebarHeight = oldheight;
		_SidebarOnRight = oldright;
		_BarHeight = oldbar;
		_SidebarOverlay = oldoverlay;
		Apply_Layout();
		return(false);
	}
	return(true);
}


/// <summary>
/// Starts the overlay sidebar sliding fully in or fully out from wherever it is, over the
/// given time; zero puts it there at once.
/// </summary>
void Video_Slide_Sidebar(bool in, int milliseconds)
{
	_SlideFrom = Video_Sidebar_Slide();
	_SlideTo = in ? 1.0f : 0.0f;
	_SlideStart = timeGetTime();
	_SlideMs = (unsigned long)std::max(milliseconds, 0);
	_FrameIsDirty = true;
}


/// <summary>
/// How far in the overlay sidebar is right now, 0 fully out to 1 fully in. Beside the
/// frame the sidebar is always fully in.
/// </summary>
float Video_Sidebar_Slide(void)
{
	if (!_SidebarOverlay) {
		return(1.0f);
	}
	if (_SlideMs == 0) {
		return(_SlideTo);
	}
	unsigned long elapsed = timeGetTime() - _SlideStart;
	if (elapsed >= _SlideMs) {
		return(_SlideTo);
	}
	return(_SlideFrom + (_SlideTo - _SlideFrom) * (float)elapsed / (float)_SlideMs);
}


/// <summary>
/// Is the overlay sidebar still on its way in or out?
/// </summary>
bool Video_Sidebar_Sliding(void)
{
	return(_SidebarOverlay && Video_Sidebar_Slide() != _SlideTo);
}


/// <summary>
/// Is the sidebar presented from its own surface rather than as part of the frame?
/// </summary>
bool Video_Sidebar_Is_Split(void)
{
	return(_ScaleInfo.Is_Split());
}


/// <summary>
/// Tells the presenter the drawable area changed size.
/// </summary>
void Video_On_Resize(int drawablewidth, int drawableheight)
{
	if (!_Initialized || drawablewidth <= 0 || drawableheight <= 0) {
		return;
	}

	_ScaleInfo.DrawableWidth = drawablewidth;
	_ScaleInfo.DrawableHeight = drawableheight;
	Backend_On_Resize(drawablewidth, drawableheight);
	Update_Scale_Info();
	Win_Cursor_Refresh();
	Video_Mark_Dirty();
}


/// <summary>
/// Sets the refresh rate used to pace presentation.
/// </summary>
void Video_Set_Refresh_Rate(int refreshrate)
{
	if (!_Initialized) {
		return;
	}

	Update_Present_Interval(refreshrate);
	Video_Mark_Dirty();
}


/// <summary>
/// Records that the visible surface has been drawn to since the last present.
/// </summary>
void Video_Mark_Dirty(void)
{
	_FrameIsDirty = true;
}


/// <summary>
/// Puts the visible surface on the screen whatever its state.
/// </summary>
void Video_Present(void)
{
	if (!_Initialized || _Presenting || VisibleSurface == NULL) {
		return;
	}

	DSurface * surface = (DSurface *)VisibleSurface;
	void * pixels = surface->Get_Buffer();

	if (pixels == NULL) {
		return;
	}

	BackendQuad frame;
	frame.Pixels = (char const *)pixels + _ScaleInfo.Tactical_X() * surface->Bytes_Per_Pixel() + _ScaleInfo.BarHeight * surface->Stride();
	frame.Pitch = surface->Stride();
	frame.DestX = _ScaleInfo.DestX;
	frame.DestY = _ScaleInfo.DestY;
	frame.DestWidth = _ScaleInfo.DestWidth;
	frame.DestHeight = _ScaleInfo.DestHeight;

	BackendQuad sidebar;
	BackendQuad const * sidebarquad = NULL;
	float slide = Video_Sidebar_Slide();
	if (_ScaleInfo.Is_Split() && SidebarSurface != NULL && slide > 0.0f) {
		DSurface * side = (DSurface *)SidebarSurface;
		if (side->Get_Buffer() != NULL && side->Get_Width() == _ScaleInfo.SidebarWidth && side->Get_Height() == _ScaleInfo.SidebarHeight) {
			sidebar.Pixels = side->Get_Buffer();
			sidebar.Pitch = side->Stride();
			// An overlay on its way in is drawn short of its place by the share still to come.
			int away = (int)((double)_ScaleInfo.SidebarDestWidth * (1.0 - slide));
			sidebar.DestX = _ScaleInfo.SidebarDestX + (_SidebarOnRight ? away : -away);
			sidebar.DestY = _ScaleInfo.SidebarDestY;
			sidebar.DestWidth = _ScaleInfo.SidebarDestWidth;
			sidebar.DestHeight = _ScaleInfo.SidebarDestHeight;
			sidebarquad = &sidebar;
		}
	}

	BackendQuad bar;
	BackendQuad const * barquad = NULL;
	if (_ScaleInfo.Bar_Is_Split() && TabSurface != NULL) {
		DSurface * top = (DSurface *)TabSurface;
		if (top->Get_Buffer() != NULL && top->Get_Width() == _ScaleInfo.BarWidth && top->Get_Height() == _ScaleInfo.BarHeight) {
			bar.Pixels = top->Get_Buffer();
			bar.Pitch = top->Stride();
			bar.DestX = _ScaleInfo.BarDestX;
			bar.DestY = _ScaleInfo.BarDestY;
			bar.DestWidth = _ScaleInfo.BarDestWidth;
			bar.DestHeight = _ScaleInfo.BarDestHeight;
			barquad = &bar;
		}
	}

	_Presenting = true;
	Backend_Present(frame, sidebarquad, barquad, Backend_Scale_Mode());
	_Presenting = false;

	// A sliding sidebar wants the next frame too, however still the game is.
	_FrameIsDirty = Video_Sidebar_Sliding();
	_LastPresentTime = timeGetTime();
}


/// <summary>
/// Puts the visible surface on the screen if it has changed and the display is ready for
/// another frame.
/// A skipped present leaves the frame marked, so the next one shows the newest content.
/// This never waits: the game loop is not paced by presentation.
/// </summary>
void Video_Present_If_Dirty(void)
{
	if (!_FrameIsDirty) {
		return;
	}

	unsigned int now = timeGetTime();
	if ((now - _LastPresentTime) < _PresentInterval) {
		return;
	}

	Video_Present();
}


/// <summary>
/// Reports where the game's frame is drawn inside the window.
/// </summary>
VideoScaleInfo const & Video_Get_Scale_Info(void)
{
	return(_ScaleInfo);
}


/// <summary>
/// Compares two display modes by width and then height.
/// </summary>
static int __cdecl Compare_Modes(void const * left, void const * right)
{
	int const * lhs = (int const *)left;
	int const * rhs = (int const *)right;

	if (lhs[0] != rhs[0]) {
		return(lhs[0] - rhs[0]);
	}
	return(lhs[1] - rhs[1]);
}


/// <summary>
/// Collects the display resolutions that fall within the given bounds.
/// Only the sizes matter; the desktop decides the color depth, and duplicates that differ
/// only by refresh rate are reported once.
/// </summary>
/// <param name="minwidth">The narrowest mode to report.</param>
/// <param name="minheight">The shortest mode to report.</param>
/// <param name="maxwidth">The widest mode to report.</param>
/// <param name="maxheight">The tallest mode to report.</param>
/// <returns>A caller owned array of width and height pairs ending in a zero pair, or NULL
/// when nothing matched.</returns>
int * EnumDisplayModes(int minwidth, int minheight, int maxwidth, int maxheight)
{
	DEVMODE devmode;
	int count = 0;
	int capacity = 0;
	int * modes = NULL;

	for (int pass = 0; pass < 2; pass++) {

		count = 0;

		for (int index = 0; ; index++) {
			memset(&devmode, 0, sizeof(devmode));
			devmode.dmSize = sizeof(devmode);

			if (!EnumDisplaySettings(NULL, index, &devmode)) {
				break;
			}

			int width = (int)devmode.dmPelsWidth;
			int height = (int)devmode.dmPelsHeight;

			if (width < minwidth || width > maxwidth || height < minheight || height > maxheight) {
				continue;
			}

			if (modes != NULL) {
				// The list is being filled from a second enumeration; should it have
				// grown since the one that sized the array, the extra modes are dropped.
				if (count >= capacity) {
					break;
				}
				modes[count * 2] = width;
				modes[count * 2 + 1] = height;
			}
			count++;
		}

		if (modes != NULL) {
			break;
		}

		if (count == 0) {
			return(NULL);
		}

		capacity = count;
		modes = new int[(count + 1) * 2];
	}

	qsort(modes, count, sizeof(int) * 2, Compare_Modes);

	// The same size is listed once per refresh rate and color depth it supports.
	int unique = 0;
	for (int index = 0; index < count; index++) {
		if (unique == 0 || modes[unique * 2 - 2] != modes[index * 2] || modes[unique * 2 - 1] != modes[index * 2 + 1]) {
			modes[unique * 2] = modes[index * 2];
			modes[unique * 2 + 1] = modes[index * 2 + 1];
			unique++;
		}
	}

	modes[unique * 2] = 0;
	modes[unique * 2 + 1] = 0;
	return(modes);
}
