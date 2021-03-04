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

#include "stream.h"

#include <algorithm>

/*
	stream
*/

stream::stream(stream* parent_)
	: parent(parent_) {
	if(parent != nullptr) {
		parent->children.push_back(this);
	}
}

stream::stream(stream&& rhs)
	: parent(rhs.parent),
	  children(rhs.children),
	  name(rhs.name) {
	rhs.parent = nullptr;
	rhs.children = {};
	if(parent != nullptr) {
		for(stream*& child : parent->children) {
			if(child == &rhs) {
				child = this;
			}
		}
	}
	for(stream* child : children) {
		if(child->parent == &rhs) {
			child->parent = this;
		}
	}
}

stream::~stream() {
	if(parent != nullptr) {
		parent->children.erase(std::find(parent->children.begin(), parent->children.end(), this));
	}
	for(stream* child : children) {
		if(child->parent == this) {
			child->parent = nullptr;
		}
	}
}

/*
	file_stream
*/

file_stream::file_stream(std::string path)
	: file_stream(path, std::ios::in) {}

file_stream::file_stream(std::string path, std::ios_base::openmode mode)
	: stream(nullptr),
	  _file(path, mode | std::ios::binary),
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
	: stream(nullptr) {}

array_stream::array_stream(stream* parent_)
	: stream(parent_) {}

std::size_t array_stream::size() const {
	return buffer.size();
}

void array_stream::seek(std::size_t offset) {
	pos = offset;
}
std::size_t array_stream::tell() const {
	return pos;
}

void array_stream::read_n(char* dest, std::size_t size) {
	std::size_t required_size = pos + size;
	if(required_size > buffer.size()) {
		throw stream_io_error("Tried to read past end of array_stream!");
	}
	std::memcpy(dest, buffer.data() + pos, size);
	pos += size;
}

void array_stream::write_n(const char* data, std::size_t size) {
	std::size_t required_size = pos + size;
	if(pos + size > buffer.size()) {
		buffer.resize(required_size);
	}
	std::memcpy(buffer.data() + pos, data, size);
	pos += size;
}

std::string array_stream::resource_path() const {
	return "arraystream";
}

char* array_stream::data() {
	return buffer.data();
}

bool array_stream::compare_contents(array_stream& a, array_stream& b) {
	if(a.buffer.size() != b.buffer.size()) {
		return false;
	}
	for(size_t i = 0; i < a.buffer.size(); i++) {
		if(a.buffer[i] != b.buffer[i]) {
			return false;
		}
	}
	return true;
}

/*
	proxy_stream
*/

proxy_stream::proxy_stream(stream* parent_, std::size_t zero, std::size_t size)
	: stream(parent_),
	  _zero(zero),
	  _size(size) {}

std::size_t proxy_stream::size() const {
	return std::min(_size, parent->size() - _zero);
}

void proxy_stream::seek(std::size_t offset) {
	parent->seek(offset + _zero);
}

std::size_t proxy_stream::tell() const {
	return parent->tell() - _zero;
}

void proxy_stream::read_n(char* dest, std::size_t size) {
	parent->read_n(dest, size);
}

void proxy_stream::write_n(const char* data, std::size_t size) {
	parent->write_n(data, size);
}
	
std::string proxy_stream::resource_path() const {
	std::stringstream to_hex;
	to_hex << std::hex << _zero;
	return parent->resource_path() + "+0x" + to_hex.str();
}

/*
	trace_stream
*/

trace_stream::trace_stream(stream* parent)
	: stream(parent),
	  read_mask(parent->size(), false) {}

std::size_t trace_stream::size() const {
	return parent->size();
}

void trace_stream::seek(std::size_t offset) {
	parent->seek(offset);
}

std::size_t trace_stream::tell() const {
	return parent->tell();
}

void trace_stream::read_n(char* dest, std::size_t size_) {
	std::size_t pos = tell();
	parent->read_n(dest, size_);
	for(std::size_t i = pos; i < std::min(size(), pos + size_); i++) {
		read_mask[i] = true;
	}
}

void trace_stream::write_n(const char* data, std::size_t size) {
	parent->write_n(data, size);
}

std::string trace_stream::resource_path() const {
	return "tracepoint(" + parent->resource_path() + ")";
}
