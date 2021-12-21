/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#ifndef UTIL_H
#define UTIL_H

#include <map>
#include <array>
#include <cmath>
#include <vector>
#include <cstring>
#include <ctype.h>
#include <stdio.h>
#include <optional>
#include <stdarg.h>
#include <stdint.h>
#include <exception>
#include <functional>
#include <type_traits>

#include "../version_check/version_check.h"

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using f32 = float;
using f64 = double;

#ifdef _MSC_VER
	#define fseek _fseeki64
	#define ftell _ftelli64
#endif

#define BEGIN_END(container) container.begin(), container.end()

// *****************************************************************************
// Error handling
// *****************************************************************************

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"

// This should contain a string describing what the program is currently doing
// so that it can be printed out if there's an error.
extern const char* UTIL_ERROR_CONTEXT_STRING;

void assert_impl(const char* file, int line, const char* arg_str, bool condition);
#undef assert
#define assert(condition) \
	if(!(condition)) { \
		fprintf(stderr, "[%s:%d] assert%s: %s\n", __FILE__, __LINE__, UTIL_ERROR_CONTEXT_STRING, #condition); \
		abort(); \
	}
#define assert_not_reached(error_message) \
	fprintf(stderr, "[%s:%d] assert%s: ", __FILE__, __LINE__, UTIL_ERROR_CONTEXT_STRING); \
	fprintf(stderr, "%s", error_message); \
	fprintf(stderr, "\n"); \
	abort();

// All these ugly _impl function templates are necessary so we can pass zero
// varargs without getting a syntax error because of the additional comma.
template <typename... Args>
void verify_impl(const char* file, int line, bool condition, const char* error_message, Args... args) {
	if(!condition) {
		fprintf(stderr, "[%s:%d] error%s: ", file, line, UTIL_ERROR_CONTEXT_STRING);
		fprintf(stderr, error_message, args...);
		fprintf(stderr, "\n");
		abort();
	}
}
// Like assert, but for things that could be user errors e.g. bad input files.
#define verify(condition, ...) \
	verify_impl(__FILE__, __LINE__, condition, __VA_ARGS__)
template <typename... Args>
[[noreturn]] void verify_not_reached_impl(const char* file, int line, const char* error_message, Args... args) {
	fprintf(stderr, "[%s:%d] error%s: ", file, line, UTIL_ERROR_CONTEXT_STRING);
	fprintf(stderr, error_message, args...);
	fprintf(stderr, "\n");
	abort();
}
#define verify_not_reached(...) \
	verify_not_reached_impl(__FILE__, __LINE__, __VA_ARGS__)

std::string string_format(const char* format, va_list args);

struct ErrorContext {
	ErrorContext(const char* format, ...);
	~ErrorContext();
};

// This will push a string onto the error context stack and pop it off again
// when it goes out of scope. These strings are appended together and printed
// out when there is an error.
#define ERROR_CONTEXT(...) ErrorContext _error_context(__VA_ARGS__)

#pragma GCC diagnostic pop

struct ParseError : std::exception {
	ParseError(const char* format, ...) {
		va_list args;
		va_start(args, format);
		message = string_format(format, args);
		va_end(args);
	}
	virtual const char* what() const noexcept {
		return message.c_str();
	}
	std::string message;
};

// *****************************************************************************
// Binary file I/O
// *****************************************************************************

#ifdef _MSC_VER
	#define packed_struct(name, ...) \
		__pragma(pack(push, 1)) struct name { __VA_ARGS__ } __pragma(pack(pop));
#else
	#define packed_struct(name, ...) \
		struct __attribute__((__packed__)) name { __VA_ARGS__ };
#endif

static const s64 SECTOR_SIZE = 0x800;

packed_struct(ByteRange,
	s32 offset;
	s32 size;
)

packed_struct(ArrayRange,
	s32 count;
	s32 offset;
)

packed_struct(Sector32,
	s32 sectors;
	Sector32() : sectors(0) {}
	Sector32(s32 s) : sectors(s) {}
	s64 bytes() const { return sectors * SECTOR_SIZE; }
	static Sector32 size_from_bytes(s64 size_in_bytes) {
		if(size_in_bytes % SECTOR_SIZE != 0) {
			size_in_bytes += SECTOR_SIZE - (size_in_bytes % SECTOR_SIZE);
		}
		s32 size_in_sectors = (s32) (size_in_bytes / SECTOR_SIZE);
		// If this ever asserts then hello from the distant past.
		assert(size_in_sectors == (s64) size_in_bytes / SECTOR_SIZE);
		return { size_in_sectors };
	}
)

packed_struct(SectorRange,
	Sector32 offset;
	Sector32 size;
	
	Sector32 end() const {
		return {offset.sectors + size.sectors};
	}
)

packed_struct(SectorByteRange,
	Sector32 offset;
	s32 size_bytes;
	
	Sector32 end() const {
		return {offset.sectors + Sector32::size_from_bytes(size_bytes).sectors};
	}
)

u16 byte_swap_16(u16 val);
u32 byte_swap_32(u32 val);

// We can't pass around references to fields as we're using packed structs so
// instead of std::swap we have to use this macro.
#define SWAP_PACKED(inmem, packed) \
	{ \
		auto p = packed; \
		packed = inmem; \
		inmem = p; \
	}

// *****************************************************************************
// Other stuff
// *****************************************************************************

// Kludge since C++ still doesn't have proper reflection.
#define DEF_FIELD(member) \
	{ \
		auto temp = std::move(member); \
		t.field(#member, temp); \
		member = std::move(temp); \
	}
#define DEF_PACKED_FIELD(member) \
	{ \
		auto temp = member; \
		t.field(#member, temp); \
		member = temp; \
	}
#define DEF_HEXDUMP(member) \
	{ \
		auto temp = std::move(member); \
		t.hexdump(#member, temp); \
		member = std::move(temp); \
	}

template <typename> struct MemberTraits;
template <typename Return, typename Object>
struct MemberTraits<Return (Object::*)> {
	typedef Object instance_type;
};

template <typename T>
using Opt = std::optional<T>;

template <typename T>
const T& opt_iterator(const Opt<T>& opt) {
	if(opt.has_value()) {
		return *opt;
	} else {
		static const T empty;
		return empty;
	}
}

template <typename T>
const size_t opt_size(const Opt<std::vector<T>>& opt_vec) {
	if(opt_vec.has_value()) {
		return opt_vec->size();
	} else {
		return 0;
	}
}

template <typename T>
T& opt_iterator(Opt<T>& opt) {
	if(opt.has_value()) {
		return *opt;
	} else {
		static T empty;
		return empty;
	}
}

template <typename>
struct IsVector : std::false_type {};
template <typename Element>
struct IsVector<std::vector<Element>> : std::true_type {};

[[maybe_unused]] static std::string get_application_version_string() {
	std::string raw_tag = get_git_tag();
	std::string raw_commit = get_git_commit();
	std::string tag, commit;
	for(char c : raw_tag) {
		if(isprint(c) && c != ' ') {
			tag += c;
		}
	}
	for(char c : raw_commit) {
		if(isprint(c) && c != ' ') {
			commit += c;
		}
	}
	std::string version;
	if(tag.size() > 0 && commit.size() > 0) {
		version += tag + " " + commit;
	} else if(commit.size() > 0) {
		version += commit;
	}
	if(version == "") {
		version = "error: No git in path during build or cmake problem.";
	}
	return version;
}

// Implements a way to delay the execution of a block of code until the
// enclosing scope ends. This lets us write statements in a more logical order.
template <typename F>
struct _deferer {
	F callback;
	_deferer(F cb) : callback(cb) {}
	~_deferer() { callback(); }
};
#define CONCAT_TOKEN_IMPL(x, y) x##y
#define CONCAT_TOKEN(x, y) CONCAT_TOKEN_IMPL(x, y)
#define defer(...) _deferer CONCAT_TOKEN(_deferer_object_, __COUNTER__)(__VA_ARGS__);

std::string md5_to_printable_string(uint8_t in[16]);

enum class Game {
	RAC1 = 1, RAC2 = 2, RAC3 = 3, DL = 4
};

enum class WadType {
	UNKNOWN, LEVEL
};

struct Wad {
	Game game;
	WadType type = WadType::UNKNOWN;
	virtual ~Wad() {}
};

#endif
