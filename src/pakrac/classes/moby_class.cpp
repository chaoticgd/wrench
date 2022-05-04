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

#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>
#include <engine/moby.h>

static void unpack_moby_class(MobyClassAsset& dest, InputStream& src, Game game);
static void pack_moby_class(OutputStream& dest, MobyClassAsset& src, Game game);

on_load(MobyClass, []() {
	MobyClassAsset::funcs.unpack_rac1 = wrap_unpacker_func<MobyClassAsset>(unpack_moby_class);
	MobyClassAsset::funcs.unpack_rac2 = wrap_unpacker_func<MobyClassAsset>(unpack_moby_class);
	MobyClassAsset::funcs.unpack_rac3 = wrap_unpacker_func<MobyClassAsset>(unpack_moby_class);
	MobyClassAsset::funcs.unpack_dl = wrap_unpacker_func<MobyClassAsset>(unpack_moby_class);
	
	MobyClassAsset::funcs.pack_rac1 = wrap_packer_func<MobyClassAsset>(pack_moby_class);
	MobyClassAsset::funcs.pack_rac2 = wrap_packer_func<MobyClassAsset>(pack_moby_class);
	MobyClassAsset::funcs.pack_rac3 = wrap_packer_func<MobyClassAsset>(pack_moby_class);
	MobyClassAsset::funcs.pack_dl = wrap_packer_func<MobyClassAsset>(pack_moby_class);
})

static void unpack_moby_class(MobyClassAsset& dest, InputStream& src, Game game) {
	src.seek(0);
	std::vector<u8> buffer = src.read_multiple<u8>(src.size());
	MobyClassData data = read_moby_class(buffer, game);
	ColladaScene scene = recover_moby_class(data, -1, 0);
	std::vector<u8> xml = write_collada(scene);
	dest.file().write_text_file("mesh.dae", (char*) xml.data());
}

static void pack_moby_class(OutputStream& dest, MobyClassAsset& src, Game game) {
	
}
