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

#include "command_line.h"

#include <sstream>
#include <iostream>

#include "build.h"

bool parse_command_line_args(
	int argc, char** argv,
	po::options_description desc,
	po::positional_options_description pd) {
	desc.add_options()
		("help,h",    "Display help text.")
		("version,v", "Print version and licensing information.");
	
	po::variables_map vm;
	try {
		auto parser = po::command_line_parser(argc, argv)
			.options(desc).positional(pd).run();
		po::store(parser, vm);
	} catch(boost::program_options::error& e) {
		std::cerr << e.what();
		return false;
	}

	if(vm.count("help")) {
		std::cout << desc;
		return false;
	}

	if(vm.count("version")) {
		std::cout << "wrench " WRENCH_VERSION_STR << std::endl;
		std::cout << "Copyright (c) 2019 chaoticgd.\n"
		          << "License GPLv3: GNU GPL version 3 <http://gnu.org/licenses/gpl.html>.\n"
		          << "This is free software: you are free to change and redistribute it.\n"
		          << "There is NO WARRANTY, to the extent permitted by law." << std::endl;
		return false;
	}

	po::notify(vm);
	
	return true;
}

int run_cli_converter(
	int argc, char** argv,
	const char* help_text,
	std::map<std::string, stream_op> commands) {

	std::string command;
	std::string src_path;
	std::string dest_path;
	std::string offset_hex;

	std::string command_description =
		"The operation to perform. Possible values are:";
	for(auto command : commands) {
		command_description += " " + command.first + ",";
	}
	command_description.back() = '.';

	po::options_description desc(help_text);
	desc.add_options()
		("command,c", po::value<std::string>(&command)->required(),
			command_description.c_str())
		("src,s",    po::value<std::string>(&src_path)->required(),
			"The input file.")
		("dest,d",   po::value<std::string>(&dest_path)->required(),
			"The output file.")
		("offset,o", po::value<std::string>(&offset_hex)->default_value("0"),
			"The offset in the input file where the header begins.");

	po::positional_options_description pd;
	pd.add("command", 1);
	pd.add("src", 1);
	pd.add("dest", 1);

	if(!parse_command_line_args(argc, argv, desc, pd)) {
		return 0;
	}

	file_stream src(src_path);
	file_stream dest(dest_path, std::ios::in | std::ios::out | std::ios::trunc);

	std::stringstream offset_stream;
	offset_stream << std::hex << offset_hex;
	uint32_t offset;
	offset_stream >> offset;
	
	proxy_stream src_proxy(&src, offset);

	auto op = commands.find(command);
	if(op == commands.end()) {
		std::cerr << "Invalid command.\n";
		return 1;
	}

	(*op).second(dest, src_proxy);

	return 0;
}
