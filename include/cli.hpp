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

#include <unordered_map>
#include <functional>
#include <vector>
#include <string>

using CommandFn = std::function<int(const std::vector<std::string>& args)>;

namespace Command {
	int help(const std::vector<std::string>& args);
	int init(const std::vector<std::string>& args);
	int version(const std::vector<std::string>& args);
	int config(const std::vector<std::string>& args);
	int man(const std::vector<std::string>& args);
	int add(const std::vector<std::string>& args);
	int commit(const std::vector<std::string>& args);
	int log(const std::vector<std::string>& args);
	int status(const std::vector<std::string>& args);
	int diff(const std::vector<std::string>& args);
	int branch(const std::vector<std::string>& args);
	int switchBranch(const std::vector<std::string>& args);
	int restore(const std::vector<std::string>& args);
	int stash(const std::vector<std::string>& args);
	int merge(const std::vector<std::string>& args);
	int unstage(const std::vector<std::string>& args);
	int tag(const std::vector<std::string>& args);
	int show(const std::vector<std::string>& args);
	int checkout(const std::vector<std::string>& args);
	int release(const std::vector<std::string>& args);
	int pack(const std::vector<std::string>& args);
	int unpack(const std::vector<std::string>& args);
	int ci(const std::vector<std::string>& args);
	int submodule(const std::vector<std::string>& args);
}

static const std::unordered_map<std::string, CommandFn> commands = {
	{"help", Command::help},
	{"init", Command::init}, {"i", Command::init},
	{"version", Command::version}, {"v", Command::version},
	{"config", Command::config},
	{"man", Command::man},
	{"add", Command::add}, {"a", Command::add},
	{"commit", Command::commit}, {"c", Command::commit},
	{"log", Command::log}, {"l", Command::log},
	{"status", Command::status}, {"s", Command::status},
	{"diff", Command::diff}, {"d", Command::diff},
	{"branch", Command::branch}, {"b", Command::branch},
	{"switch", Command::switchBranch}, {"sw", Command::switchBranch},
	{"restore", Command::restore}, {"r", Command::restore},
	{"stash", Command::stash}, {"st", Command::stash},
	{"merge", Command::merge}, {"mg", Command::merge},
	{"unstage", Command::unstage}, {"u", Command::unstage},
	{"tag", Command::tag}, {"t", Command::tag},
	{"show", Command::show},
	{"checkout", Command::checkout}, {"co", Command::checkout},
	{"release", Command::release},
	{"pack", Command::pack},
	{"unpack", Command::unpack},
	{"ci", Command::ci},
	{"submodule", Command::submodule}, {"sm", Command::submodule}
};

