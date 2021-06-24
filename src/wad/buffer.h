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

#ifndef WAD_BUFFER_H
#define WAD_BUFFER_H

#include "util.h"

template <typename T>
struct BufferArray {
	const T* lo = nullptr;
	const T* hi = nullptr;
	const T* begin() { return lo; }
	const T* end() { return hi; }
	std::vector<T> copy() {
		std::vector<T> vec(hi - lo);
		memcpy(vec.data(), lo, (hi - lo) * sizeof(T));
		return vec;
	}
};

struct Buffer {
	const uint8_t* lo = nullptr;
	const uint8_t* hi = nullptr;
	
	Buffer() {}
	Buffer(const uint8_t* l, const uint8_t* h) : lo(l), hi(h) {}
	Buffer(const std::vector<u8>& src) : lo(&(*src.begin())), hi(&(*src.end())) {}
	const uint8_t& operator[](s64 i) const { return lo[i]; }
	s64 size() const { return hi - lo; }
	bool in_bounds(s64 offset) { return offset >= 0 && lo + offset < hi; }
	
	Buffer subbuf(s64 offset) const;
	Buffer subbuf(s64 offset, s64 new_size) const;
	
	template <typename T>
	const T& read(s64 offset, const char* subject) const {
		verify(offset >= 0, "Failed to read %s: Offset cannot be negative.", subject);
		verify(lo + offset + sizeof(T) <= hi, "Failed to read %s: Attempted to read past end of buffer.", subject);
		return *(const T*) (lo + offset);
	}
	
	template <typename T>
	BufferArray<T> read_multiple(s64 offset, s64 count, const char* subject) const {
		verify(offset >= 0, "Failed to read %s: Offset cannot be negative.", subject);
		verify(lo + offset + count * sizeof(T) <= hi, "Failed to read %s: Attempted to read past end of buffer.", subject);
		const T* iter_lo = (const T*) (lo + offset);
		return {iter_lo, iter_lo + count};
	}
	
	std::string read_string(s64 offset) const;
	void hexdump(FILE* file, s64 column, const char* ansi_colour_code = "0") const;
};

void diff_buffers(Buffer lhs, Buffer rhs, s64 offset, const char* subject);

struct OutBuffer {
	std::vector<u8>& vec;
	
	OutBuffer(std::vector<u8>& v) : vec(v) {}
	s64 tell() const { return vec.size(); }
	
	template <typename T>
	s64 alloc() {
		s64 write_pos = vec.size();
		vec.resize(vec.size() + sizeof(T));
		return write_pos;
	}
	
	template <typename T>
	s64 alloc_multiple(s64 count) {
		size_t write_pos = vec.size();
		vec.resize(vec.size() + count * sizeof(T));
		return write_pos;
	}
	
	template <typename T>
	s64 write(const T& thing) {
		size_t write_pos = vec.size();
		vec.resize(vec.size() + sizeof(T));
		*(T*) &vec[write_pos] = thing;
		return write_pos;
	}
	
	template <typename T>
	s64 write(s64 offset, const T& thing) {
		assert(offset > 0);
		assert(offset + sizeof(T) <= vec.size());
		*(T*) &vec[offset] = thing;
		return offset;
	}
	
	template <typename T>
	s64 write_multiple(const std::vector<T>& things) {
		size_t write_pos = vec.size();
		vec.resize(vec.size() + things.size() * sizeof(T));
		memcpy(&vec[write_pos], things.data(), things.size() * sizeof(T));
		return write_pos;
	}
	
	void pad(s64 align, u8 padding = 0) {
		if(vec.size() % align != 0) {
			s64 pad_size = align - (vec.size() % align);
			vec.resize(vec.size() + pad_size, padding);
		}
	}
};

#endif
