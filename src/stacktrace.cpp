/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2020 chaoticgd

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

#include "stacktrace.h"

#include <sstream>

#ifdef __GNUC__
	#include <stdlib.h>
	#include <execinfo.h>
#endif

std::string generate_stacktrace() {
	std::stringstream result;
	
	#ifdef __GNUC__
		void* array[256];
		std::size_t size = backtrace(array, 256);
		char** strings = backtrace_symbols(array, size);

		for(std::size_t i = 0; i < size; i++) {
			result << std::string(strings[i]) + "\n";
		}

		free(strings);
	#elif
		// TODO: Produce stack traces on Windows.
		result << "Stack traces for your OS are not yet supported.";
	#endif
	
	return result.str();
}
