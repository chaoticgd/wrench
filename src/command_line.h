#ifndef COMMAND_LINE_ARGS_H
#define COMMAND_LINE_ARGS_H

#include <boost/program_options.hpp>

namespace po = boost::program_options;

bool parse_command_line_args(
	int argc, char** argv,
	po::options_description desc = po::options_description(""),
	po::positional_options_description pd = po::positional_options_description());

#endif
