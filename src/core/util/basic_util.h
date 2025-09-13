/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2025 chaoticgd

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

#ifndef CORE_BASIC_UTIL_H
#define CORE_BASIC_UTIL_H

#include <array>
#include <cmath>
#include <string>
#include <vector>
#include <cstring>
#include <ctype.h>
#include <stdio.h>
#include <optional>
#include <stdarg.h>
#include <stdint.h>
#include <algorithm>
#include <exception>

using u8 = unsigned char;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = signed char;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using f32 = float;
using f64 = double;

#define WRENCH_PI 3.14159265358979323846


#define BEGIN_END(container) (container).begin(), (container).end()

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#define ARRAY_PAIR(array) array, ARRAY_SIZE(array)

std::string string_format(const char* format, va_list args);
std::string stringf(const char* format, ...);

u16 byte_swap_16(u16 val);
u32 byte_swap_32(u32 val);

std::size_t parse_number(std::string x);

template <typename T>
using Opt = std::optional<T>;

template <typename T>
const T& opt_iterator(const Opt<T>& opt)
{
	if(opt.has_value()) {
		return *opt;
	} else {
		static const T empty;
		return empty;
	}
}

template <typename T>
const size_t opt_size(const Opt<std::vector<T>>& opt_vec)
{
	if(opt_vec.has_value()) {
		return opt_vec->size();
	} else {
		return 0;
	}
}

template <typename T>
T& opt_iterator(Opt<T>& opt)
{
	if(opt.has_value()) {
		return *opt;
	} else {
		static T empty;
		return empty;
	}
}

template <typename T>
T opt_or_zero(Opt<T>& opt)
{
	if(opt.has_value()) {
		return *opt;
	} else {
		return 0;
	}
}

template <typename>
struct IsVector : std::false_type {};
template <typename Element>
struct IsVector<std::vector<Element>> : std::true_type {};

// Implements a way to delay the execution of a block of code until the
// enclosing scope ends. This lets us write statements in a more logical order.
template <typename F>
struct _deferer
{
	F callback;
	_deferer(F cb) : callback(cb) {}
	~_deferer() { callback(); }
};
#define CONCAT_TOKEN_IMPL(x, y) x##y
#define CONCAT_TOKEN(x, y) CONCAT_TOKEN_IMPL(x, y)
#define defer(...) _deferer CONCAT_TOKEN(_deferer_object_, __COUNTER__)(__VA_ARGS__);

std::string md5_to_printable_string(uint8_t in[16]);

#define on_load(tag, ...) \
	struct OnLoadType ##tag { OnLoadType ##tag() { __VA_ARGS__(); } }; \
	static OnLoadType ##tag _on_load_ctor;

f32 lerp(f32 min, s32 max, f32 value);

// Extract a value from a bitfield e.g.
//  bit_range(0bAABBCCDD, 2, 3) => 0bCC
s32 bit_range(u64 val, s32 lo, s32 hi);

std::string to_snake_case(const char* src);

#define ALIGN(value, alignment) ((value) + (-(value) & ((alignment) - 1)))

s32 align32(s32 value, s32 alignment);
s64 align64(s64 value, s64 alignment);

template <typename T>
bool contains(const T& container, const typename T::value_type& value)
{
	for(const auto& element : container) {
		if(element == value) {
			return true;
		}
	}
	return false;
}

bool find_case_insensitive_substring(const char* haystack, const char* needle);

#endif
