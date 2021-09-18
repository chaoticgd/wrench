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

#include <glm/glm.hpp>

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

namespace fs = std::filesystem;

#define BEGIN_END(container) container.begin(), container.end()

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"

template <typename... Args>
[[noreturn]] void assert_not_reached_impl(const char* file, int line, const char* error_message, Args... args) {
	fprintf(stderr, "[%s:%d] assert: ", file, line);
	fprintf(stderr, error_message, args...);
	fprintf(stderr, "\n");
	exit(1);
}
#define assert_not_reached(...) \
	assert_not_reached_impl(__FILE__, __LINE__, __VA_ARGS__)


// Like assert, but for user errors.
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
	fprintf(stderr, "[%s:%d] error: ", file, line);
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
	
	glm::vec3 unpack() const {
		return glm::vec3(x, y, z);
	}
	
	static Vec3f pack(glm::vec3 vec) {
		Vec3f result;
		result.x = vec.x;
		result.y = vec.y;
		result.z = vec.z;
		return result;
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
	
	glm::vec4 unpack() const {
		return glm::vec4(x, y, z, w);
	}
	
	static Vec4f pack(glm::vec4 vec) {
		Vec4f result;
		result.x = vec.x;
		result.y = vec.y;
		result.z = vec.z;
		result.w = vec.w;
		return result;
	}
	
	void swap(glm::vec4& vec) {
		SWAP_PACKED(x, vec.x);
		SWAP_PACKED(y, vec.y);
		SWAP_PACKED(z, vec.z);
		SWAP_PACKED(w, vec.w);
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
	
	glm::mat3x4 unpack() const {
		glm::mat3x4 result;
		result[0] = m_0.unpack();
		result[1] = m_1.unpack();
		result[2] = m_2.unpack();
		return result;
	}
	
	static Mat3 pack(glm::mat3x4 mat) {
		Mat3 result;
		result.m_0 = Vec4f::pack(mat[0]);
		result.m_1 = Vec4f::pack(mat[1]);
		result.m_2 = Vec4f::pack(mat[2]);
		return result;
	}
)

packed_struct(Mat4,
	Vec4f m_0;
	Vec4f m_1;
	Vec4f m_2;
	Vec4f m_3;
	
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
		
		temp = m_3;
		t.field("3", temp);
		m_3 = temp;
	}
	
	glm::mat4 unpack() const {
		glm::mat4 result;
		result[0] = m_0.unpack();
		result[1] = m_1.unpack();
		result[2] = m_2.unpack();
		result[3] = m_3.unpack();
		return result;
	}
	
	static Mat4 pack(glm::mat4 mat) {
		Mat4 result;
		result.m_0 = Vec4f::pack(mat[0]);
		result.m_1 = Vec4f::pack(mat[1]);
		result.m_2 = Vec4f::pack(mat[2]);
		result.m_3 = Vec4f::pack(mat[3]);
		return result;
	}
)

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
