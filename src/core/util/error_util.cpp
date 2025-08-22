/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include <core/util/error_util.h>

const char* UTIL_ERROR_CONTEXT_STRING = "";

static std::vector<std::string> error_context_stack;
static std::string error_context_alloc;
static char what_message[2048] = {};

RuntimeError::RuntimeError(const char* f, int l, const char* format, ...)
	: file(f), line(l), context(UTIL_ERROR_CONTEXT_STRING)
{
	va_list args;
	va_start(args, format);
	message = string_format(format, args);
	va_end(args);
}

const char* RuntimeError::what() const noexcept
{
	snprintf(what_message, sizeof(what_message) - 1, "[%s:%d] \033[31merror%s:\033[0m %s\n", file, line, context.c_str(), message.c_str());
	return what_message;
}

void RuntimeError::print() const
{
	fflush(stdout);
	fflush(stderr);
	fprintf(stderr, "[%s:%d] \033[31merror%s:\033[0m %s\n", file, line, context.c_str(), message.c_str());
	fflush(stderr);
}

void assert_impl(const char* file, int line, const char* arg_str, bool condition)
{
	if (!condition) {
		fprintf(stderr, "[%s:%d] assert%s: ", file, line, UTIL_ERROR_CONTEXT_STRING);
		fprintf(stderr, "%s", arg_str);
		fprintf(stderr, "\n");
		exit(1);
	}
}

static void update_error_context()
{
	error_context_alloc = "";
	for (auto& str : error_context_stack) {
		error_context_alloc += str;
	}
	UTIL_ERROR_CONTEXT_STRING = error_context_alloc.c_str();
}

ErrorContext::ErrorContext(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	error_context_stack.emplace_back(" " + string_format(format, args));
	va_end(args);
	update_error_context();
}

ErrorContext::~ErrorContext()
{
	error_context_stack.pop_back();
	update_error_context();
}
