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
