/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#ifndef CORE_STREAM_H
#define CORE_STREAM_H

#include <core/util.h>
#include <core/filesystem.h>

// Binary streams that can either be backed by memory or a file. For stuff that
// can always fit in memory consider using Buffer and OutBuffer instead.

class InputStream;
class OutputStream;

class Stream {
protected:
	Stream() = default;
public:
	Stream(const Stream&) = delete;
	Stream(Stream&&) = delete;
	virtual ~Stream() = 0;
	
	static void copy(OutputStream& dest, InputStream& src, s64 size);
	
	virtual bool seek(s64 offset) = 0;
	virtual s64 tell() const = 0;
	virtual s64 size() const = 0;
};

class InputStream : public Stream {
public:
	virtual bool read(u8* dest, s64 size) = 0;
	
	template <typename T>
	T read() {
		T result;
		read(reinterpret_cast<u8*>(&result), sizeof(T));
		return result;
	}

	template <typename T>
	T read(s64 offset) {
		seek(offset);
		return read<T>();
	}
	
	template <typename T>
	std::vector<T> read_multiple(s64 count) {
		std::vector<T> buffer(count);
		read(reinterpret_cast<u8*>(buffer.data()), buffer.size() * sizeof(T));
		return buffer;
	}
};

class OutputStream : public Stream {
public:
	virtual bool write(const u8* src, s64 size) = 0;

	template <typename T>
	void write(const T& value) {
		write(reinterpret_cast<const u8*>(&value), sizeof(T));
	}

	template <typename T>
	void write(s64 offset, const T& value) {
		s64 pos = tell();
		seek(offset);
		write(value);
		seek(pos);
	}
	
	template <typename T>
	void write_v(const std::vector<T>& buffer) {
		write(reinterpret_cast<const u8*>(buffer.data()), buffer.size() * sizeof(T));
	}
	
	void pad(s64 alignment, u8 padding);
};

class MemoryInputStream : public InputStream {
public:
	MemoryInputStream(const u8* begin_, const u8* end_);
	MemoryInputStream(const std::vector<u8>& bytes);
	
	bool seek(s64 offset) override;
	s64 tell() const override;
	s64 size() const override;
	
	bool read(u8* dest, s64 size) override;

private:
	const u8* begin;
	const u8* end;
	s64 ofs = 0;
};

class MemoryOutputStream : public OutputStream {
public:
	MemoryOutputStream(std::vector<u8>& backing_);
	
	bool seek(s64 offset) override;
	s64 tell() const override;
	s64 size() const override;
	
	bool write(const u8* src, s64 size) override;

	std::vector<u8>& backing;
	s64 ofs = 0;
};

class FileInputStream : public InputStream {
public:
	FileInputStream();
	~FileInputStream() override;
	
	bool open(const fs::path& path);
	
	bool seek(s64 offset) override;
	s64 tell() const override;
	s64 size() const override;
	
	bool read(u8* dest, s64 size) override;

	FILE* file = nullptr;
};

class FileOutputStream : public OutputStream {
public:
	FileOutputStream();
	~FileOutputStream() override;
	
	bool open(const fs::path& path);
	
	bool seek(s64 offset) override;
	s64 tell() const override;
	s64 size() const override;
	
	bool write(const u8* src, s64 size) override;
	
	FILE* file = nullptr;
};

#endif
