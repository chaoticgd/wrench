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
	#define packed_struct(name, body) \
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

// I/O error e.g. tried to read past end.
class stream_io_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

// The content of the stream is of the wrong format e.g. failed decompression.
class stream_format_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

class stream {
public:
	virtual void seek_abs(uint32_t offset) = 0;
	virtual uint32_t tell_abs() = 0;

	virtual void read_n(char* dest, uint32_t size) = 0;
	virtual void write_n(const char* data, uint8_t size) = 0;

	void seek(uint32_t offset) {
		seek_abs(_zero + offset);
	}

	uint32_t tell() {
		return tell_abs() - _zero;
	}

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

	// The dest and src streams should be different.
	static void copy_n(stream& dest, stream& src, uint32_t size, uint32_t src_offset) {
		uint32_t src_tell = src.tell();
		src.seek(src_offset);
		copy_n(dest, src, size);
		src.seek(src_tell);
	}

	/*
		Handle nested data segments. For example, a level file may contain a
		number of different asset segments. First, you could push the base
		offset of the level, then pass the stream to another bit of code that
		could push the relative offset an individual asset and pass the stream
		to a third function for reading, like so:

		0x0000                                                            0xffff
		| GAME.ISO                                                             |
		^ stream::stream()
		|--------| LEVEL0                    |---------------------------------|
		         ^ push_zero(LEVEL0_OFFSET)
		|----------------| SOMETEXTURE |---------------------------------------|
		                 ^ push_zero(SOMETEXTURE_OFFSET)
		|--------| LEVEL0                    |---------------------------------|
		         ^ pop_zero()
		| GAME.ISO                                                             |
		^ pop_zero()
	*/
	void push_zero(uint32_t offset) {
		_zero += offset;
		_zero_stack.push_back(offset);
	}

	void pop_zero() {
		_zero -= _zero_stack.back();
		_zero_stack.pop_back();
	}

	// Pretty print new data that has been written to the end of the buffer.
	// Compare said data to an 'expected' data file.
	void print_diff(std::optional<stream*> expected) {

		if(tell() < _last_printed) {
			return;
		}
		bool is_bad = false;
		std::cout << std::hex << (_last_printed | 0x1000000000000000) << " >>>> ";
		for(int i = _last_printed; i < tell(); i++) {
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
		};
		if(is_bad) {
			std::cout << std::endl; // Flush the buffer.
			throw stream_format_error("Data written to stream did not match expected stream.");
		}
		std::cout << "\n";
		_last_printed = tell();
	}

protected:
	stream() : _zero(0), _last_printed(0) {}

private:

	uint32_t _zero;
	std::vector<uint32_t> _zero_stack;
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

	void seek_abs(uint32_t offset) {
		_file.seekg(offset);
		check_error();
	}

	uint32_t tell_abs() {
		return _file.tellg();
		check_error();
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
	
	void seek_abs(uint32_t offset) {
		_offset = offset;
	}

	uint32_t tell_abs() {
		return _offset;
	}

	void read_n(char* dest, uint32_t size) {
		std::size_t required_size = _offset + size;
		if(required_size >= _allocation.size()) {
			throw stream_io_error("Tried to read past end of array_buffer!");
		}
		std::memcpy(dest, _allocation.data() + _offset, size);
		_offset += size;
	}

	void write_n(const char* data, uint8_t size) {
		std::size_t required_size = _offset + size;
		if(_offset + size >= _allocation.size()) {
			_allocation.resize(required_size);
		}
		std::memcpy(_allocation.data() + _offset, data, size);
		_offset += size;
	}
private:
	std::vector<uint8_t> _allocation;
	uint32_t _offset;
};

#endif
