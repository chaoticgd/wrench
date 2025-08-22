/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2024 chaoticgd

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
#include <platform/fileio.h>
#include <core/filesystem.h>

// Binary streams that can either be backed by memory or a file. For stuff that
// can always fit in memory consider using Buffer and OutBuffer instead.

class InputStream;
class OutputStream;

class Stream
{
protected:
	Stream() = default;
public:
	Stream(const Stream&) = delete;
	Stream(Stream&&) = delete;
	virtual ~Stream();
	
	static void copy(OutputStream& dest, InputStream& src, s64 size);
	
	virtual bool seek(s64 offset) = 0;
	virtual s64 tell() const = 0;
	virtual s64 size() const = 0;
	
	mutable std::string last_error;
};

class InputStream : public Stream
{
public:
	virtual bool read_n(u8* dest, s64 size) = 0;
	
	template <typename T>
	T read() {
		T result;
		read_n(reinterpret_cast<u8*>(&result), sizeof(T));
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
		read_n(reinterpret_cast<u8*>(buffer.data()), buffer.size() * sizeof(T));
		return buffer;
	}
	
	template <typename T>
	std::vector<T> read_multiple(s64 offset, s64 count) {
		seek(offset);
		std::vector<T> buffer(count);
		read_n(reinterpret_cast<u8*>(buffer.data()), buffer.size() * sizeof(T));
		return buffer;
	}
	
	template <typename T>
	std::vector<T> read_multiple(ArrayRange range) {
		seek(range.offset);
		std::vector<T> buffer(range.count);
		read_n(reinterpret_cast<u8*>(buffer.data()), buffer.size() * sizeof(T));
		return buffer;
	}
};

class OutputStream : public Stream
{
public:
	virtual bool write_n(const u8* src, s64 size) = 0;

	template <typename T>
	void write(const T& value) {
		write_n(reinterpret_cast<const u8*>(&value), sizeof(T));
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
		write_n(reinterpret_cast<const u8*>(buffer.data()), buffer.size() * sizeof(T));
	}
	
	template <typename T>
	s64 alloc() {
		static_assert(sizeof(T) <= sizeof(m_zeroes));
		s64 ofs = tell();
		write_n(m_zeroes, sizeof(T));
		return ofs;
	}
	
	template <typename T>
	s64 alloc_multiple(s64 count) {
		static_assert(sizeof(T) <= sizeof(m_zeroes));
		s64 ofs = tell();
		for(s64 i = 0; i < count; i++) {
			write_n(m_zeroes, sizeof(T));
		}
		return ofs;
	}
	
	void pad(s64 alignment, u8 padding);

private:
	static const constexpr u8 m_zeroes[4096] = {0};
};

class BlackHoleOutputStream : public OutputStream
{
public:
	BlackHoleOutputStream();
	
	bool seek(s64 offset) override;
	s64 tell() const override;
	s64 size() const override;
	
	bool write_n(const u8* src, s64 size) override;
	
private:
	s64 m_ofs = 0;
	s64 m_top = 0;
};

class MemoryInputStream : public InputStream
{
public:
	MemoryInputStream(const u8* begin_, const u8* end_);
	MemoryInputStream(const std::vector<u8>& bytes);
	
	bool seek(s64 offset) override;
	s64 tell() const override;
	s64 size() const override;
	
	bool read_n(u8*, s64 size) override;
	
private:
	const u8* m_begin;
	const u8* m_end;
	s64 m_ofs = 0;
};

class MemoryOutputStream : public OutputStream
{
public:
	MemoryOutputStream(std::vector<u8>& backing_);
	
	bool seek(s64 offset) override;
	s64 tell() const override;
	s64 size() const override;
	
	bool write_n(const u8* src, s64 size) override;
	
private:
	std::vector<u8>& m_backing;
	s64 m_ofs = 0;
};

class FileInputStream : public InputStream
{
public:
	FileInputStream();
	~FileInputStream() override;
	
	bool open(const fs::path& path);
	
	bool seek(s64 offset) override;
	s64 tell() const override;
	s64 size() const override;
	
	bool read_n(u8* dest, s64 size) override;

private:
	WrenchFileHandle* m_file = nullptr;
	std::string m_error_message;
};

class FileOutputStream : public OutputStream
{
public:
	FileOutputStream();
	~FileOutputStream() override;
	
	bool open(const fs::path& path);
	
	bool seek(s64 offset) override;
	s64 tell() const override;
	s64 size() const override;
	
	bool write_n(const u8* src, s64 size) override;
	
	WrenchFileHandle* m_file = nullptr;
};

class SubInputStream : public InputStream
{
public:
	SubInputStream(InputStream& stream_, ByteRange64 range_);
	SubInputStream(InputStream& stream_, s64 base_, s64 bytes_);
	
	bool seek(s64 offset) override;
	s64 tell() const override;
	s64 size() const override;
	
	bool read_n(u8* dest, s64 size) override;
	
	s64 offset_relative_to(InputStream* outer) const;

private:
	InputStream& m_stream;
	ByteRange64 m_range;
};

class SubOutputStream : public OutputStream
{
public:
	SubOutputStream(OutputStream& stream_, s64 zero_);
	
	bool seek(s64 offset) override;
	s64 tell() const override;
	s64 size() const override;
	
	bool write_n(const u8* src, s64 size) override;
	
private:
	OutputStream& m_stream;
	s64 m_zero;
};

#endif
