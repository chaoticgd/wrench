#include "command_line.h"

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
