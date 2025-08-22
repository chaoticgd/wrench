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

#ifndef CORE_ERROR_UTIL_H
#define CORE_ERROR_UTIL_H

#include <core/util/basic_util.h>

// This should contain a string describing what the program is currently doing
// so that it can be printed out if there's an error.
extern const char* UTIL_ERROR_CONTEXT_STRING;

struct RuntimeError : std::exception
{
	RuntimeError(const char* f, int l, const char* format, ...);
	virtual const char* what() const noexcept;
	void print() const;
	const char* file;
	int line;
	std::string context;
	std::string message;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"

#define verify_fatal(condition) \
	if(!(condition)) { \
		fflush(stdout); \
		fflush(stderr); \
		fprintf(stderr, "[%s:%d] assert%s: %s\n", __FILE__, __LINE__, UTIL_ERROR_CONTEXT_STRING, #condition); \
		fflush(stderr); \
		abort(); \
	}
#define verify_not_reached_fatal(error_message) \
	fflush(stdout); \
	fflush(stderr); \
	fprintf(stderr, "[%s:%d] assert%s: ", __FILE__, __LINE__, UTIL_ERROR_CONTEXT_STRING); \
	fprintf(stderr, "%s", error_message); \
	fprintf(stderr, "\n"); \
	fflush(stderr); \
	abort();

// All these ugly _impl function templates are necessary so we can pass zero
// varargs without getting a syntax error because of the additional comma.
template <typename... Args>
void verify_impl(const char* file, int line, const char* error_message, Args... args)
{
	throw RuntimeError(file, line, error_message, args...);
}
#define verify(condition, ...) \
	if(!(condition)) { \
		verify_impl(__FILE__, __LINE__, __VA_ARGS__); \
	}
template <typename... Args>
[[noreturn]] void verify_not_reached_impl(const char* file, int line, const char* error_message, Args... args)
{
	throw RuntimeError(file, line, error_message, args...);
}
#define verify_not_reached(...) \
	verify_not_reached_impl(__FILE__, __LINE__, __VA_ARGS__)

struct ErrorContext
{
	ErrorContext(const char* format, ...);
	~ErrorContext();
};

// This will push a string onto the error context stack and pop it off again
// when it goes out of scope. These strings are appended together and printed
// out when there is an error.
#define ERROR_CONTEXT(...) ErrorContext _error_context(__VA_ARGS__)

#pragma GCC diagnostic pop

template <typename Dest, typename Src>
static Dest checked_int_cast(Src src)
{
	verify(src >= std::numeric_limits<Dest>::min()
		&& src <= std::numeric_limits<Dest>::max(),
		"Value unrepresentable due to a narrowing conversion.");
	return static_cast<Dest>(src);
}

#endif
