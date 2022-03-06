/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2022 chaoticgd

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

#include "global_wads.h"

#include <assetmgr/asset_types.h>

packed_struct(MiscWadHeaderDL,
	s32 header_size;
	Sector32 sector;
	SectorRange debug_font;
	SectorRange irx;
	SectorRange savegame;
	SectorRange frontbin;
	SectorRange frontbin_net;
	SectorRange frontend;
	SectorRange exit;
	SectorRange bootwad;
	SectorRange gadget;
)

static AssetReference write_binary_lump(Asset& parent, FILE* src, SectorRange range, const char* child, fs::path path) {
	std::vector<u8> bytes = read_file(src, range.offset.bytes(), range.size.bytes());
	BinaryAsset& binary = parent.child<BinaryAsset>(child);
	binary.set_src(parent.file().write_binary_file(path, bytes));
	return binary.reference_relative_to(parent);
}

void read_misc_wad(AssetPack& pack, FILE* src, Buffer header_bytes) {
	MiscWadHeaderDL header = header_bytes.read<MiscWadHeaderDL>(0, "file header");
	
	AssetFile& asset_file = pack.asset_file("misc/misc.asset");
	MiscWadAsset& misc_wad = asset_file.root().child<MiscWadAsset>("misc");
	
	misc_wad.set_debug_font(write_binary_lump(misc_wad, src, header.debug_font, "debug_font", "debug_font.bin"));
}
