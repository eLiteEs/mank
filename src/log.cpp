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
 * Define some funcions for logging, later add an option to optionally
 * log to a file. */

#include "decorations.hpp"
#include "log.hpp"

void Log::error(std::string message) {
	setColor(201, 32, 32);
	std::cout << message << std::endl;
	resetColor();
}

void Log::info(std::string message) {
	setColor(32, 97, 201);
	std::cout << message << std::endl;
	resetColor();
}

void Log::warning(std::string message) {
	setColor(214, 192, 26);
	std::cout << message << std::endl;
	resetColor();
}

void Log::log(std::string message) {
	resetColor();
	std::cout << message << std::endl;
}

