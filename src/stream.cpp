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

#include "stream.h"

/*
	file_stream
*/

file_stream::file_stream(std::string path)
	: file_stream(path, std::ios::in) {}

file_stream::file_stream(std::string path, std::ios_base::openmode mode)
	: _file(path, mode | std::ios::binary),
	  _path(path) {
	if(_file.fail()) {
		throw stream_io_error("Failed to open file.");
	}
}

std::size_t file_stream::size() const {
	auto& file = const_cast<std::fstream&>(_file);
	auto pos = tell();
	file.seekg(0, std::ios_base::end);
	std::size_t size = tell();
	file.seekg(pos);
	return size;
}

void file_stream::seek(std::size_t offset) {
	_file.seekg(offset);
	check_error();
}

std::size_t file_stream::tell() const {
	return const_cast<std::fstream&>(_file).tellg();
}

void file_stream::read_n(char* dest, std::size_t size) {
	_file.read((char*) dest, size);
	check_error();
}

void file_stream::write_n(const char* data, std::size_t size) {
	_file.write((char*) data, size);
	check_error();
}

std::string file_stream::resource_path() const {
	return std::string("file(") + _path + ")";
}

void file_stream::check_error() {
	if(_file.fail()) {
		throw stream_io_error("Bad stream."); 
	}
}

/*
	array_stream
*/

array_stream::array_stream()
	: _offset(0) {}

std::size_t array_stream::size() const {
	return _allocation.size();
}

void array_stream::seek(std::size_t offset) {
	_offset = offset;
}
std::size_t array_stream::tell() const {
	return _offset;
}

void array_stream::read_n(char* dest, std::size_t size) {
	std::size_t required_size = _offset + size;
	if(required_size > _allocation.size()) {
		throw stream_io_error("Tried to read past end of array_stream!");
	}
	std::memcpy(dest, _allocation.data() + _offset, size);
	_offset += size;
}

void array_stream::write_n(const char* data, std::size_t size) {
	std::size_t required_size = _offset + size;
	if(_offset + size > _allocation.size()) {
		_allocation.resize(required_size);
	}
	std::memcpy(_allocation.data() + _offset, data, size);
	_offset += size;
}

std::string array_stream::resource_path() const {
	return "arraystream";
}

/*
	proxy_stream
*/

proxy_stream::proxy_stream(stream* backing, std::size_t zero, std::size_t size)
	: _backing(backing),
	  _zero(zero),
	  _size(size) {}

std::size_t proxy_stream::size() const {
	return std::min(_size, _backing->size() - _zero);
}

void proxy_stream::seek(std::size_t offset) {
	_backing->seek(offset + _zero);
}

std::size_t proxy_stream::tell() const {
	return _backing->tell() - _zero;
}

void proxy_stream::read_n(char* dest, std::size_t size) {
	_backing->read_n(dest, size);
}

void proxy_stream::write_n(const char* data, std::size_t size) {
	_backing->write_n(data, size);
}
	
std::string proxy_stream::resource_path() const {
	std::stringstream to_hex;
	to_hex << std::hex << _zero;
	return _backing->resource_path() + "+0x" + to_hex.str();
}
