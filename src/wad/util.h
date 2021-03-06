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

#ifndef WAD_UTIL_H
#define WAD_UTIL_H

#include <map>
#include <cmath>
#include <vector>
#include <cstring>
#include <stdio.h>
#include <assert.h>
#include <optional>
#include <stdint.h>
#include <filesystem>
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

namespace fs = std::filesystem;

#define BEGIN_END(container) container.begin(), container.end()

// Like assert, but for user errors.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
template <typename... Args>
void verify_impl(const char* file, int line, bool condition, const char* error_message, Args... args) {
	if(!condition) {
		fprintf(stderr, "[%s:%d] error: ", file, line);
		fprintf(stderr, error_message, args...);
		fprintf(stderr, "\n");
		exit(1);
	}
}
#define verify(condition, ...) \
	verify_impl(__FILE__, __LINE__, condition, __VA_ARGS__)
template <typename... Args>
[[noreturn]] void verify_not_reached_impl(const char* file, int line, const char* error_message, Args... args) {
	fprintf(stderr, "[%s:%d] ", file, line);
	fprintf(stderr, error_message, args...);
	fprintf(stderr, "\n");
	exit(1);
}
#define verify_not_reached(...) \
	verify_not_reached_impl(__FILE__, __LINE__, __VA_ARGS__)
#pragma GCC diagnostic pop

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
)

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

packed_struct(Vec3f,
	f32 x;
	f32 y;
	f32 z;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(x);
		DEF_PACKED_FIELD(y);
		DEF_PACKED_FIELD(z);
	}
)

packed_struct(Vec4f,
	f32 x;
	f32 y;
	f32 z;
	f32 w;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(x);
		DEF_PACKED_FIELD(y);
		DEF_PACKED_FIELD(z);
		DEF_PACKED_FIELD(w);
	}
)

packed_struct(Mat3,
	Vec4f m_0;
	Vec4f m_1;
	Vec4f m_2;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Vec4f temp;
		
		temp = m_0;
		t.field("0", temp);
		m_0 = temp;
		
		temp = m_1;
		t.field("1", temp);
		m_1 = temp;
		
		temp = m_2;
		t.field("2", temp);
		m_2 = temp;
	}
)

template <typename> struct MemberTraits;
template <typename Return, typename Object>
struct MemberTraits<Return (Object::*)> {
	typedef Object instance_type;
};

// We can't pass around references to fields as we're using packed structs so
// instead of std::swap we have to use this macro.
#define SWAP_PACKED(inmem, packed) \
	{ \
		auto p = packed; \
		packed = inmem; \
		inmem = p; \
	}

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

enum class Game {
	RAC1, RAC2, RAC3, DL
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
