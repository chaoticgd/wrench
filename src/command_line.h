/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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
#include <boost/program_options.hpp>

#include "stream.h"

namespace po = boost::program_options;

bool parse_command_line_args(
	int argc, char** argv,
	po::options_description desc = po::options_description(""),
	po::positional_options_description pd = po::positional_options_description());

using stream_op = std::function<void(stream& dest, stream& src)>;

int run_cli_converter(
	int argc, char** argv,
	const char* help_text,
	std::map<std::string, stream_op> commands);

#endif
