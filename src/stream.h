/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#ifndef STREAM_H
#define STREAM_H

#include <vector>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <type_traits>

/*
	A set of utility classes and macros for working with binary files.
*/

#ifdef _MSC_VER
	#define packed_struct(name, body \
		__pragma(pack(push, 1)) struct name { body } __pragma(pack(pop))
#else
	#define packed_struct(name, body) \
		struct __attribute__((__packed__)) name { body };
#endif

template <typename T>
packed_struct(file_ptr,
	file_ptr()           : value(0) {}
	file_ptr(uint32_t v) : value(v) {}

	uint32_t value;

	template <typename T_rhs>
	friend file_ptr<T_rhs> operator+(file_ptr<T>& lhs, file_ptr<T_rhs>& rhs) {
		return lhs.value + rhs.value;
	}

	template <typename T_result>
	file_ptr<T_result> next() {
		return value + sizeof(T);
	}
)

struct stream_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
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
	virtual uint32_t size() = 0;
	virtual void seek(uint32_t offset) = 0;
	virtual uint32_t tell() = 0;

	virtual void read_n(char* dest, uint32_t size) = 0;
	virtual void write_n(const char* data, uint8_t size) = 0;

	template <typename T>
	T read() {
		static_assert(std::is_default_constructible<T>::value);
		T result;
		read_n(reinterpret_cast<char*>(&result), sizeof(T));
		return result;
	}

	template <typename T>
	T read(uint32_t offset) {
		seek(offset);
		return read<T>();
	}

	template <typename T>
	void write(const T& value) {
		static_assert(std::is_default_constructible<T>::value);
		write_n(reinterpret_cast<const char*>(&value), sizeof(T));
	}

	template <typename T>
	void write(uint32_t offset, const T& value) {
		seek(offset);
		write(value);
	}

	template <typename T, typename... T_args>
	T peek(T_args... args) {
		uint32_t whence_you_came = tell();
		T result = read<T>(args...);
		seek(whence_you_came);
		return result;
	}

	// The dest and src streams should be different.
	static void copy_n(stream& dest, stream& src, uint32_t size) {
		std::vector<char> buffer(size);
		src.read_n(buffer.data(), size);
		dest.write_n(buffer.data(), size);
	}

	// Pretty print new data that has been written to the end of the buffer.
	// Compare said data to an 'expected' data file.
	void print_diff(std::optional<stream*> expected) {

		if(tell() < _last_printed) {
			return;
		}

		bool is_bad = false;
		std::cout << std::hex << (_last_printed | 0x1000000000000000) << " >>>> ";
		for(uint32_t i = _last_printed; i < tell(); i++) {
			auto val = peek<uint8_t>(i);
			if(expected.has_value()) {
				auto expected_val = (*expected)->peek<uint8_t>(i);
				if(val == expected_val) {
					std::cout << "\033[1;32m"; // Green.
				} else {
					std::cout << "\033[1;31m"; // Red.
					is_bad = true;
				}
			} else {
				std::cout << "\033[1;33m"; // Yellow.
			}
			if(val < 0x10) std::cout << "0";
			std::cout << std::hex << (val & 0xff);
			std::cout << "\033[0m"; // Reset colours.
			if((i - _last_printed) % 32 == 31) {
				std::cout << "\n" << std::hex << ((i + 1) | 0x1000000000000000) << " >>>> ";
			} else {
				std::cout << " ";
			}
		}
		if(is_bad) {
			std::cout << "\nEXPECTED:\n";
			(*expected)->_last_printed = _last_printed;
			(*expected)->seek(tell());
			(*expected)->print_diff({});
			std::cout << std::endl;
			throw stream_format_error("Data written to stream did not match expected stream.");
		}
		std::cout << "\n";
		_last_printed = tell();
	}

protected:
	stream() : _last_printed(0) {}

private:

	uint32_t _last_printed;
};

class file_stream : public stream {
public:
	file_stream(std::string path)
		: file_stream(path, std::ios::in) {}

	file_stream(std::string path, std::ios_base::openmode mode)
		: _file(path, mode | std::ios::binary) {
		if(_file.fail()) {
			throw stream_io_error("Failed to open file.");
		}
	}

	uint32_t size() {
		auto pos = tell();
		_file.seekg(0, std::ios_base::end);
		uint32_t size = tell();
		seek(pos);
		return size;
	}

	void seek(uint32_t offset) {
		_file.seekg(offset);
		check_error();
	}

	uint32_t tell() {
		return _file.tellg();
	}

	void read_n(char* dest, uint32_t size) {
		_file.read(dest, size);
		check_error();
	}

	void write_n(const char* data, uint8_t size) {
		_file.write(data, size);
		check_error();
	}

	void check_error() {
		if(_file.fail()) {
			throw stream_io_error("Bad stream."); 
		}
	}

private:
	std::fstream _file;
};

class array_stream : public stream {
public:
	array_stream() : _offset(0) {}
	
	uint32_t size() {
		return _allocation.size();
	}

	void seek(uint32_t offset) {
		_offset = offset;
	}

	uint32_t tell() {
		return _offset;
	}

	void read_n(char* dest, uint32_t size) {
		std::size_t required_size = _offset + size;
		if(required_size > _allocation.size()) {
			throw stream_io_error("Tried to read past end of array_buffer!");
		}
		std::memcpy(dest, _allocation.data() + _offset, size);
		_offset += size;
	}

	void write_n(const char* data, uint8_t size) {
		std::size_t required_size = _offset + size;
		if(_offset + size > _allocation.size()) {
			_allocation.resize(required_size);
		}
		std::memcpy(_allocation.data() + _offset, data, size);
		_offset += size;
	}

private:
	std::vector<uint8_t> _allocation;
	uint32_t _offset;
};

// Point to a data segment within a larger stream. For example, you could create
// a stream to allow for more convenient access a texture within a disk image.
class proxy_stream : public stream {
public:
	proxy_stream(stream* backing, uint32_t zero)
		: _backing(backing), _zero(zero) {}

	uint32_t size() {
		return _backing->size() - _zero;
	}

	void seek(uint32_t offset) {
		_backing->seek(offset + _zero);
	}

	uint32_t tell() {
		return _backing->tell() - _zero;
	}

	void read_n(char* dest, uint32_t size) {
		_backing->read_n(dest, size);
	}

	void write_n(const char* data, uint8_t size) {
		_backing->write_n(data, size);
	}

private:
	stream* _backing;
	uint32_t _zero;
};

#endif
