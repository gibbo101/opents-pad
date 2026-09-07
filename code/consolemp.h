/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "rect.h"

#include <string>
#include <vector>

class Surface;

// Helpers shared by the console multiplayer setup screens.

std::string Console_On_Off(bool value);

// Map descriptions carry stray spaces around their player counts, such as "(2-4 )".
std::string Console_Tidy_Description(char const * text);

int Console_Wrap(int value, int low, int high);

// Rebuilds the map preview for the session's current scenario, if its file is at hand.
void Console_Load_Map_Preview(void);
void Console_Draw_Map_Preview(Surface & surface, Rect const & panel);

// The lobby's small faction icons, magenta keyed, or NULL when the art is missing.
Surface * Console_Side_Icon(bool gdi);
std::vector<int> Console_Player_Swatches(void);
