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

#include "command_line.h"

#include <sstream>
#include <iostream>

#include "util.h"
#include "config.h"

cxxopts::ParseResult parse_command_line_args(int argc, char** argv, cxxopts::Options options) {
	options.add_options()
		("h,help",    "Display help text.")
		("v,version", "Print version and licensing information.");
	options.show_positional_help();
		
	auto args = options.parse(argc, argv);

	if(args.count("help")) {
		std::cout << options.help();
		std::exit(0);
	}

	if(args.count("version")) {
		std::cout << "wrench " WRENCH_VERSION_STR << std::endl;
		std::cout << "Copyright (c) 2020 chaoticgd.\n"
		          << "License GPLv3+: GNU GPL version 3 <http://gnu.org/licenses/gpl.html>.\n"
		          << "This is free software: you are free to change and redistribute it.\n"
		          << "There is NO WARRANTY, to the extent permitted by law." << std::endl;
		std::exit(0);
	}
	
	return args;
}

int run_cli_converter(
	int argc, char** argv,
	const char* help_text,
	std::map<std::string, stream_op> commands) {

	std::string command_description =
		"The operation to perform. Possible values are:";
	for(auto command : commands) {
		command_description += " " + command.first + ",";
	}
	command_description.back() = '.';

	cxxopts::Options options("wrench", help_text);
	options.add_options()
		("c,command", command_description.c_str(),
			cxxopts::value<std::string>())
		("s,src", "The input file.",
			cxxopts::value<std::string>())
		("d,dest", "The output file.", cxxopts::value<std::string>())
		("o,offset", "The offset in the input file where the header begins.",
			cxxopts::value<std::string>()->default_value("0"));

	options.parse_positional({
		"command", "src", "dest"
	});

	auto args = parse_command_line_args(argc, argv, options);
	std::string command = cli_get(args, "command");
	std::string src_path = cli_get(args, "src");
	std::string dest_path = cli_get(args, "dest");
	std::size_t offset = parse_number(cli_get_or(args, "offset", "0"));

	file_stream src(src_path);
	file_stream dest(dest_path, std::ios::in | std::ios::out | std::ios::trunc);
	
	proxy_stream src_proxy(&src, offset, src.size() - offset);

	auto op = commands.find(command);
	if(op == commands.end()) {
		std::cerr << "Invalid command.\n";
		return 1;
	}

	(*op).second(dest, src_proxy);

	return 0;
}

std::string cli_get(const cxxopts::ParseResult& result, const char* arg) {
	if(!result.count(arg)) {
		std::cout << "Argument --" << arg << " required but not provided." << std::endl;
		std::exit(1);
	}
	return result[arg].as<std::string>();
}

std::string cli_get_or(const cxxopts::ParseResult& result, const char* arg, const char* default_value) {
	if(!result.count(arg)) {
		return default_value;
	}
	return result[arg].as<std::string>();
}
