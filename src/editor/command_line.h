/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <functional>
#include <cxxopts/include/cxxopts.hpp>

// Utility functions to parse command line arguments.

// Will not return if "--help", "--version", "-h" or "-v" is passed.
cxxopts::ParseResult parse_command_line_args(int argc, char** argv, cxxopts::Options options);

// Get the value of an argument from a ParseResult
// object, or call std::exit(1).
std::string cli_get(const cxxopts::ParseResult& result, const char* arg);

// Get the value of an argument from a ParseResult
// object, or return a default value.
std::string cli_get_or(const cxxopts::ParseResult& result, const char* arg, const char* default_value);

#endif
