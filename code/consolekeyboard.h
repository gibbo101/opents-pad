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
/// Runs the on-screen keyboard over the current backdrop; a real keyboard types straight in.
/// Returns true with the text replaced when the player finishes, false with it untouched.
/// </summary>
bool Console_Keyboard(char const * title, std::string & text, int max_length);
