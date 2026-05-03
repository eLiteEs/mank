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

/*
 * Define some functions for text decoration,
 * underline, bold and other styles in the .hpp file
 */

#include "decorations.hpp"
#include <iostream> 

void setColor(int r, int g, int b) {
	std::cout << "\033[38;2;" << r << ";" << g << ";" << b << "m" << std::flush;
}

void resetColor() {
	std::cout << "\033[0m";
}

