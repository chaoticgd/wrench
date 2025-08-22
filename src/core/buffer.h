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

#ifndef BUFFER_H
#define BUFFER_H

#include "util.h"

template <typename T>
struct BufferArray
{
	const T* lo = nullptr;
	const T* hi = nullptr;
	const T& operator[](s64 i) const { return lo[i]; }
	const T* begin() { return lo; }
	const T* end() { return hi; }
	s64 size() const { return hi - lo; }
	std::vector<T> copy()
	{
		std::vector<T> vec(hi - lo);
		memcpy(vec.data(), lo, (hi - lo) * sizeof(T));
		return vec;
	}
};

template <typename T, s64 N>
packed_struct(FixedArray,
	T array[N];
	using value_type = T;
	static const constexpr s64 element_count = N;
	FixedArray(const BufferArray<T>& src)
	{
		verify_fatal(src.size() == N);
		memcpy(array, src.lo, src.size() * sizeof(T));
	}
	const T* data() const { return array; }
	s64 size() const { return N; }
)

struct Buffer
{
	const u8* lo = nullptr;
	const u8* hi = nullptr;
	
	Buffer() {}
	Buffer(const u8* l, const u8* h) : lo(l), hi(h) {}
	template <typename T>
	Buffer(const std::vector<T>& src) : lo((u8*) src.data()), hi((u8*) (src.data() + src.size())) {}
	Buffer(const std::string& src) : lo((u8*) src.data()), hi((u8*) src.data() + src.size()) {}
	const u8& operator[](s64 i) const { return lo[i]; }
	s64 size() const { return hi - lo; }
	bool in_bounds(s64 offset) { return offset >= 0 && lo + offset < hi; }
	
	Buffer subbuf(s64 offset) const;
	Buffer subbuf(s64 offset, s64 new_size) const;
	
	template <typename T>
	const T& read(s64 offset, const char* subject = "buffer") const
	{
		verify(offset >= 0, "Failed to read %s: Offset cannot be negative.", subject);
		verify(lo + offset + sizeof(T) <= hi, "Failed to read %s: Attempted to read past end of buffer.", subject);
		return *(const T*) (lo + offset);
	}
	
	template <typename T>
	BufferArray<T> read_multiple(s64 offset, s64 count, const char* subject = "buffer") const
	{
		verify(offset >= 0, "Failed to read %s: Offset cannot be negative.", subject);
		verify(count >= 0, "Failed to read %s: Count cannot be negative.", subject);
		verify(lo + offset + count * sizeof(T) <= hi, "Failed to read %s: Attempted to read past end of buffer.", subject);
		const T* iter_lo = (const T*) (lo + offset);
		return {iter_lo, iter_lo + count};
	}
	
	template <typename T>
	BufferArray<T> read_all(s64 offset = 0) const
	{
		s64 buffer_size = hi - (lo + offset);
		s64 element_count = buffer_size / sizeof(T);
		s64 data_size = element_count * sizeof(T);
		return {(const T*) (lo + offset), (const T*) (lo + offset + data_size)};
	}
	
	template <typename T>
	BufferArray<T> read_multiple(ArrayRange range, const char* subject) const
	{
		return read_multiple<T>(range.offset, range.count, subject);
	}
	
	std::vector<u8> read_bytes(s64 offset, s64 size, const char* subject) const;
	std::string read_string(s64 offset, bool is_korean = false) const;
	std::string read_fixed_string(s64 offset, s64 size) const;
	void hexdump(FILE* file, s64 column, const char* ansi_colour_code = "0") const;
};

#define DIFF_REST_OF_BUFFER -1

bool diff_buffers(Buffer lhs, Buffer rhs, s64 offset, s64 size, bool print_diff, const std::vector<ByteRange64>* ignore_list = nullptr);

struct OutBuffer
{
	std::vector<u8>& vec;
	
	OutBuffer(std::vector<u8>& v) : vec(v) {}
	s64 tell() const { return vec.size(); }
	
	template <typename T>
	s64 alloc()
	{
		s64 write_ofs = vec.size();
		vec.resize(vec.size() + sizeof(T));
		return write_ofs;
	}
	
	template <typename T>
	s64 alloc_multiple(s64 count, u8 fill = 0)
	{
		size_t write_ofs = vec.size();
		vec.resize(vec.size() + count * sizeof(T), fill);
		return write_ofs;
	}
	
	template <typename T>
	s64 write(const T& thing)
	{
		size_t write_ofs = vec.size();
		vec.resize(vec.size() + sizeof(T));
		*(T*) &vec[write_ofs] = thing;
		return write_ofs;
	}
	
	template <typename T>
	s64 write(s64 offset, const T& thing)
	{
		verify_fatal(offset >= 0);
		verify_fatal(offset + sizeof(T) <= vec.size());
		*(T*) &vec[offset] = thing;
		return offset;
	}
	
	template <typename T>
	s64 write_multiple(const T& things)
	{
		size_t write_ofs = vec.size();
		vec.resize(vec.size() + things.size() * sizeof(typename T::value_type));
		memcpy(&vec[write_ofs], things.data(), things.size() * sizeof(typename T::value_type));
		return write_ofs;
	}
	
	template <typename T>
	s64 write_multiple(s64 offset, const T& things)
	{
		verify_fatal(offset >= 0);
		verify_fatal(offset + things.size() * sizeof(typename T::value_type) <= vec.size());
		memcpy(&vec[offset], things.data(), things.size() * sizeof(typename T::value_type));
		return offset;
	}
	
	void pad(s64 align, u8 padding = 0);
	void writesf(s32 indent_level, const char* format, va_list args);
	void writelf(s32 indent_level, const char* format, va_list args);
	void writesf(s32 indent_level, const char* format, ...);
	void writelf(s32 indent_level, const char* format, ...);
	void writesf(const char* format, ...);
	void writelf(const char* format, ...);
};

#endif
