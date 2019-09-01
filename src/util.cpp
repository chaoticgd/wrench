#include "util.h"

#include <sstream>

std::string int_to_hex(std::size_t x) {
	std::stringstream ss;
	ss << std::hex << x;
	return ss.str();
}

std::size_t hex_to_int(std::string x) {
	std::stringstream ss;
	ss << std::hex << x;
	std::size_t result;
	ss >> result;
	return result;
}

std::size_t parse_number(std::string x) {
	std::stringstream ss;
	if(x.size() >= 2 && x[0] == '0' && x[1] == 'x') {
		ss << std::hex << x.substr(2);
	} else {
		ss << x;
	}
	std::size_t result;
	ss >> result;
	return result;
}
