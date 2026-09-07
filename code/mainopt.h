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
bool Console_Options_Screen(bool in_game = false);	// In game the display rows are left out.
void Main_Options_Dialog(void);
