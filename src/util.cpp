#include "util.h"

#include <sstream>

std::string int_to_hex(std::size_t x) {
	std::stringstream ss;
	ss << std::hex << x;
	return ss.str();
}

std::size_t parse_number(std::string x) {
	std::stringstream ss;
	ss << x.substr(2);
	if(x.size() >= 2 && x[0] == '0' && x[1] == 'x') {
		ss << std::hex;
	}
	std::size_t result;
	ss >> result;
	return result;
}
