/*
 * Version tracking program
 * Copyright (C) 2026  mank
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <string_view>

void setColor(int r, int g, int b);
void resetColor();

namespace ansi {
	// ── Styles ──────────────────────────────────
	inline constexpr std::string_view RESET     = "\033[0m";
	inline constexpr std::string_view BOLD      = "\033[1m";
	inline constexpr std::string_view DIM       = "\033[2m";
	inline constexpr std::string_view UNDERLINE = "\033[4m";
	inline constexpr std::string_view BLINK     = "\033[5m";
	inline constexpr std::string_view REVERSE   = "\033[7m";

	// ── Foreground ────────────
	inline constexpr std::string_view FG_BLACK   = "\033[30m";
	inline constexpr std::string_view FG_RED     = "\033[31m";
	inline constexpr std::string_view FG_GREEN   = "\033[32m";
	inline constexpr std::string_view FG_YELLOW  = "\033[33m";
	inline constexpr std::string_view FG_BLUE    = "\033[34m";
	inline constexpr std::string_view FG_MAGENTA = "\033[35m";
	inline constexpr std::string_view FG_CYAN    = "\033[36m";
	inline constexpr std::string_view FG_WHITE   = "\033[37m";

	// Bright foreground
	inline constexpr std::string_view FG_BRIGHT_BLACK   = "\033[90m";
	inline constexpr std::string_view FG_BRIGHT_RED     = "\033[91m";
	inline constexpr std::string_view FG_BRIGHT_GREEN   = "\033[92m";
	inline constexpr std::string_view FG_BRIGHT_YELLOW  = "\033[93m";
	inline constexpr std::string_view FG_BRIGHT_BLUE    = "\033[94m";
	inline constexpr std::string_view FG_BRIGHT_MAGENTA = "\033[95m";
	inline constexpr std::string_view FG_BRIGHT_CYAN    = "\033[96m";
	inline constexpr std::string_view FG_BRIGHT_WHITE   = "\033[97m";

	// ── Background colors ────────────
	inline constexpr std::string_view BG_BLACK   = "\033[40m";
	inline constexpr std::string_view BG_RED     = "\033[41m";
	inline constexpr std::string_view BG_GREEN   = "\033[42m";
	inline constexpr std::string_view BG_YELLOW  = "\033[43m";
	inline constexpr std::string_view BG_BLUE    = "\033[44m";
	inline constexpr std::string_view BG_MAGENTA = "\033[45m";
	inline constexpr std::string_view BG_CYAN    = "\033[46m";
	inline constexpr std::string_view BG_WHITE   = "\033[47m";

	// Bright backgrounds
	inline constexpr std::string_view BG_BRIGHT_BLACK   = "\033[100m";
	inline constexpr std::string_view BG_BRIGHT_RED     = "\033[101m";
	inline constexpr std::string_view BG_BRIGHT_GREEN   = "\033[102m";
	inline constexpr std::string_view BG_BRIGHT_YELLOW  = "\033[103m";
	inline constexpr std::string_view BG_BRIGHT_BLUE    = "\033[104m";
	inline constexpr std::string_view BG_BRIGHT_MAGENTA = "\033[105m";
	inline constexpr std::string_view BG_BRIGHT_CYAN    = "\033[106m";
	inline constexpr std::string_view BG_BRIGHT_WHITE   = "\033[107m";
}
