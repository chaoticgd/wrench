#ifndef STREAM_H
#define STREAM_H

#include <vector>
#include <fstream>
#include <stdexcept>
#include <sstream>
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
	void write(const T& value) {
		static_assert(std::is_default_constructible<T>::value);
		write_n(reinterpret_cast<const char*>(&value), sizeof(T));
	}

	template <typename T>
	T read(uint32_t offset) {
		seek(offset);
		return read<T>();
	}

	template <typename T>
	void write(uint32_t offset, const T& value) {
		seek(offset);
		write(value);
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

protected:
	stream() : _zero(0) {}

private:

	uint32_t _zero;
	std::vector<uint32_t> _zero_stack;
};

class file_stream : public stream {
public:
	file_stream(std::string path)
		: file_stream(path, std::ios::in) {}

	file_stream(std::string path, std::ios_base::openmode mode)
		: _file(path, mode | std::ios::binary) {}

	void seek_abs(uint32_t offset) {
		_file.seekg(offset);
	}

	uint32_t tell_abs() {
		return _file.tellg();
	}

	void read_n(char* dest, uint32_t size) {
		_file.read(dest, size);
	}

	void write_n(const char* data, uint8_t size) {
		_file.write(data, size);
	}

private:
	std::fstream _file;
};

#endif
