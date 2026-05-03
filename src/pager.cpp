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
 */

#include "pager.hpp"

#include <cstdlib>
#include <unistd.h>
#include <string>

FILE* Pager::pager = nullptr;
int Pager::fd = -1;
int Pager::old_stdout = -1;
bool Pager::active = false;

void Pager::open() {
	if (active) return;

	const char* env = std::getenv("PAGER");
	std::string cmd;
	if (env) {
		cmd = std::string(env) + " -RFX";
	} else {
		cmd = "less -RFX";
	}

	pager = popen(cmd.c_str(), "w");
	if (!pager) return;

	fd = fileno(pager);

	old_stdout = dup(STDOUT_FILENO);
	dup2(fd, STDOUT_FILENO);

	active = true;
}

void Pager::close() {
	if (!active) return;

	fflush(stdout);

	dup2(old_stdout, STDOUT_FILENO);
	::close(old_stdout);

	pclose(pager);

	pager = nullptr;
	fd = -1;
	old_stdout = -1;
	active = false;
}

bool Pager::isActive() {
	return active;
}

