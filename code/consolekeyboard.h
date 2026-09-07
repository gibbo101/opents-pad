/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <string>

/// <summary>
/// Runs the console on-screen keyboard on the current backdrop with the text to edit in its
/// field. The pad picks keys from a grid, with X deleting and Y adding a space, and a real
/// keyboard types straight in. Returns true with the text replaced when the player finishes,
/// false with it untouched when they back out.
/// </summary>
bool Console_Keyboard(char const * title, std::string & text, int max_length);
