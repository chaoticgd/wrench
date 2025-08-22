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

#ifndef CORE_BINARY_UTIL_H
#define CORE_BINARY_UTIL_H

#include <core/util/basic_util.h>
#include <core/util/error_util.h>

#ifdef _MSC_VER
	#define packed_struct(name, ...) \
		__pragma(pack(push, 1)) struct name { __VA_ARGS__ } __pragma(pack(pop));
		
	#define packed_nested_struct(name, ...) \
		struct { __VA_ARGS__ } name;
	
	#define packed_nested_anon_union(...) \
		union { __VA_ARGS__ };
#else
	#define packed_struct(name, ...) \
		struct __attribute__((__packed__)) name { __VA_ARGS__ };
	
	#define packed_nested_struct(name, ...) \
		struct __attribute__((__packed__)) { __VA_ARGS__ } name;
	
	#define packed_nested_anon_union(...) \
		union __attribute__((__packed__)) { __VA_ARGS__ };
#endif

static const s64 SECTOR_SIZE = 0x800;

struct ByteRange64
{
	ByteRange64(s64 o, s64 s) : offset(o), size(s) {}
	s64 offset;
	s64 size;
};

packed_struct(ByteRange,
	s32 offset;
	s32 size;
	
	ByteRange64 bytes()
	{
		return {offset, size};
	}
	
	bool empty() const
	{
		return size <= 0;
	}
	
	static ByteRange from_bytes(s64 offset, s64 size)
	{
		return {(s32) offset, (s32) size};
	}
)

packed_struct(ArrayRange,
	s32 count;
	s32 offset;
)

packed_struct(Sector32,
	s32 sectors;
	
	Sector32() : sectors(0) {}
	Sector32(s32 s) : sectors(s) {}
	
	s64 bytes() const
	{
		return sectors * SECTOR_SIZE;
	}
	
	bool empty() const
	{
		return sectors <= 0;
	}
	
	static Sector32 size_from_bytes(s64 size_in_bytes)
	{
		if(size_in_bytes % SECTOR_SIZE != 0) {
			size_in_bytes += SECTOR_SIZE - (size_in_bytes % SECTOR_SIZE);
		}
		s32 size_in_sectors = (s32) (size_in_bytes / SECTOR_SIZE);
		// If this check ever fails then hello from the distant past.
		verify_fatal(size_in_sectors == (s64) size_in_bytes / SECTOR_SIZE);
		return { size_in_sectors };
	}
	
	static Sector32 from_bytes(s64 offset, s64)
	{
		return size_from_bytes(offset);
	}
)

packed_struct(SectorRange,
	Sector32 offset;
	Sector32 size;
	
	Sector32 end() const
	{
		return {offset.sectors + size.sectors};
	}
	
	ByteRange64 bytes() const
	{
		return {offset.bytes(), size.bytes()};
	}
	
	bool empty() const
	{
		return size.sectors <= 0;
	}
	
	static SectorRange from_bytes(s64 offset, s64 size)
	{
		return SectorRange{Sector32::size_from_bytes(offset), Sector32::size_from_bytes(size)};
	}
)

packed_struct(SectorByteRange,
	Sector32 offset;
	s32 size_bytes;
	
	Sector32 end() const
	{
		return {offset.sectors + Sector32::size_from_bytes(size_bytes).sectors};
	}
	
	ByteRange64 bytes() const
	{
		return {offset.bytes(), size_bytes};
	}
	
	bool empty() const
	{
		return size_bytes <= 0;
	}
	
	static SectorByteRange from_bytes(s64 offset, s64 size)
	{
		return SectorByteRange{Sector32::size_from_bytes(offset), (s32) size};
	}
)

// We can't pass around references to fields as we're using packed structs so
// instead of std::swap we have to use this macro.
#define SWAP_PACKED(inmem, packed) \
	{ \
		auto p = packed; \
		packed = inmem; \
		inmem = p; \
	}

#endif
