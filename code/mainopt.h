/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

bool Change_Display_Mode(int width, int height);
bool Shell_Display_Mode(void);
bool Play_Display_Mode(void);
void Pad_Zoom_Size(int & width, int & height);		// The controller's play size, settled to the panel.
int Pad_Zoom_Neighbour(int height, int steps);		// The ladder height the given number of steps in from one.
int Pad_Zoom_Width(int height);						// The width the panel's shape gives a ladder height.
void Pad_Zoom_Name(int width, int height, char * buffer, int size);		// "1.33x, 960 x 600".
bool Pad_Zoom_Step(int steps);						// Rezooms play live, keeping the view's centre.
bool Console_Options_Screen(bool in_game = false);	// In game the display rows are left out.
bool Console_Audio_Screen(bool in_game = false);	// In game the track rows are offered too.
void Console_Controls_Screen(void);					// Lists the controller's bindings.
void Main_Options_Dialog(void);
