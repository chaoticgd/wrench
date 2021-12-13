/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#ifndef ISO_LEGACY_STREAM_H
#define ISO_LEGACY_STREAM_H

#include <bitset>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stddef.h>
#include <optional>
#include <stdexcept>
#include <type_traits>

#include <core/util.h>

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!! DO NOT USE FOR NEW CODE !!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define offsetof32(x, y) static_cast<uint32_t>(offsetof(x, y))

packed_struct(sector_range,
	Sector32 offset;
	Sector32 size;
)

packed_struct(byte_range,
	uint32_t offset;
	uint32_t size;
)

struct stream_error : public std::runtime_error {
	stream_error(const char* what)
		: std::runtime_error(what) {}
	std::string stack_trace;
};

// I/O error e.g. tried to read past end.
struct stream_io_error : public stream_error {
	using stream_error::stream_error;
};

// The content of the stream is of the wrong format e.g. failed decompression.
struct stream_format_error : public stream_error {
	using stream_error::stream_error;
};

class stream {
public:
	stream(stream* parent_);
	stream(const stream& rhs) = delete;
	stream(stream&& rhs);
	stream& operator=(stream&& rhs);
	virtual ~stream();
	
	stream* parent;
	std::vector<stream*> children;
	std::string name; // Displayed in the string viewer.

	virtual std::size_t size() const = 0;
	virtual void seek(std::size_t offset) = 0;
	virtual std::size_t tell() const = 0;

	virtual void read_n(char* dest, std::size_t size) = 0;
	virtual void write_n(const char* data, std::size_t size) = 0;

	// A resource path is a string that specifies how the resource loaded is
	// stored on disc. For example, "wad(file(LEVEL4.WAD)+0x1000)+0x10"  would
	// indicate the resource is stored in a WAD compressed segment starting at
	// 0x1000 in LEVEL4.WAD at offset 0x10 within the decompressed data.
	//
	// This is very useful for debugging as it enables me to easily locate
	// various structures in a hex editor.
	virtual std::string resource_path() const = 0;

	template <typename T>
	T read() {
		static_assert(std::is_default_constructible<T>::value);
		T result;
		read_n(reinterpret_cast<char*>(&result), sizeof(T));
		return result;
	}

	template <typename T>
	T read(std::size_t offset) {
		seek(offset);
		return read<T>();
	}

	std::string read_string() {
		std::string result;
		char c;
		while((c = read<char>()) != '\0') {
			result += c;
		}
		return result;
	}

	template <typename T>
	void write(const T& value) {
		static_assert(std::is_default_constructible<T>::value);
		write_n(reinterpret_cast<const char*>(&value), sizeof(T));
	}

	template <typename T>
	void write(std::size_t offset, const T& value) {
		seek(offset);
		write(value);
	}

	void peek_n(char* dest, std::size_t pos, std::size_t size) const {
		stream* this_ = const_cast<stream*>(this);
		std::size_t whence_you_came = this_->tell();
		this_->seek(pos);
		this_->read_n(dest, size);
		this_->seek(whence_you_came);
	}

	template <typename T, typename... T_args>
	T peek(T_args... args) const {
		stream* this_ = const_cast<stream*>(this);
		std::size_t whence_you_came = this_->tell();
		T value = this_->read<T>(args...);
		this_->seek(whence_you_came);
		return value;
	}
	
	template <typename T>
	std::vector<T> read_multiple(size_t count) {
		std::vector<T> buffer(count);
		read_n(reinterpret_cast<char*>(buffer.data()), buffer.size() * sizeof(T));
		return buffer;
	}
	
	template <typename T>
	void read_v(std::vector<T>& buffer) {
		read_n(reinterpret_cast<char*>(buffer.data()), buffer.size() * sizeof(T));
	}
	
	template <typename T>
	void write_v(const std::vector<T>& buffer) {
		write_n(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(T));
	}
	
	void align(std::size_t alignment, uint8_t padding) {
		size_t pos = tell();
		seek(pos + ((pos % alignment != 0) ? (alignment - (pos % alignment)) : 0));
	}
	
	void pad(std::size_t alignment, uint8_t padding) {
		while(tell() % alignment != 0) {
			write(padding);
		}
	}

	// The dest and src streams should be different.
	static void copy_n(stream& dest, stream& src, std::size_t size) {
		// Copy a megabyte at a time.
		static const std::size_t chunk_size = 1024 * 1024;
		std::vector<char> buffer(chunk_size);
		for(std::size_t i = 0; i < size / chunk_size; i++) {
			src.read_n(buffer.data(), chunk_size);
			dest.write_n(buffer.data(), chunk_size);
		}
		std::size_t last_chunk_size = size % chunk_size;
		src.read_n(buffer.data(), last_chunk_size);
		dest.write_n(buffer.data(), last_chunk_size);
	}
	
	// Check if the stream tree contains a given stream object. This is useful
	// for checking if a pointer to a stream is still valid.
	bool contains(stream* needle) {
		if(needle == this) {
			return true;
		}
		for(stream* child : children) {
			if(child->contains(needle)) {
				return true;
			}
		}
		return false;
	}

protected:
	stream()
		: _last_printed(0) {}

private:

	std::size_t _last_printed;
};

class file_stream final: public stream {
public:
	file_stream(std::string path);
	file_stream(std::string path, std::ios_base::openmode mode);
	
	std::size_t size() const;
	void seek(std::size_t offset);
	std::size_t tell() const;
	void read_n(char* dest, std::size_t size);
	void write_n(const char* data, std::size_t size);
	std::string resource_path() const;
	void check_error();
	
private:
	std::fstream _file;
	std::string _path;
};

class array_stream : public stream {
public:
	array_stream();
	array_stream(stream* parent_);
	
	std::size_t size() const;
	void seek(std::size_t offset);
	std::size_t tell() const;
	void read_n(char* dest, std::size_t size);
	void write_n(const char* data, std::size_t size);
	std::string resource_path() const;
	
	char* data();
	static bool compare_contents(array_stream& a, array_stream& b);

	std::vector<char> buffer;
	std::size_t pos = 0;
};

#endif
