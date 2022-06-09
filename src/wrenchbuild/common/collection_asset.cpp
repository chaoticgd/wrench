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

#include <core/png.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/common/subtitles.h>

static void unpack_collection_asset(CollectionAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_collection_asset(OutputStream& dest, const CollectionAsset& src, BuildConfig config, const char* hint);
static void unpack_texture_list(CollectionAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_texture_list(OutputStream& dest, const CollectionAsset& src, BuildConfig config, const char* hint);

on_load(Collection, []() {
	CollectionAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<CollectionAsset>(unpack_collection_asset);
	CollectionAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<CollectionAsset>(unpack_collection_asset);
	CollectionAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<CollectionAsset>(unpack_collection_asset);
	CollectionAsset::funcs.unpack_dl = wrap_hint_unpacker_func<CollectionAsset>(unpack_collection_asset);
	
	CollectionAsset::funcs.pack_rac1 = wrap_hint_packer_func<CollectionAsset>(pack_collection_asset);
	CollectionAsset::funcs.pack_rac2 = wrap_hint_packer_func<CollectionAsset>(pack_collection_asset);
	CollectionAsset::funcs.pack_rac3 = wrap_hint_packer_func<CollectionAsset>(pack_collection_asset);
	CollectionAsset::funcs.pack_dl = wrap_hint_packer_func<CollectionAsset>(pack_collection_asset);
})

static void unpack_collection_asset(CollectionAsset& dest, InputStream& src, BuildConfig config, const char* hint) {
	const char* type = next_hint(&hint);
	if(strcmp(type, "texlist") == 0) {
		unpack_texture_list(dest, src, config, hint);
	} else if(strcmp(type, "subtitles") == 0) {
		unpack_subtitles(dest, src, config);
	} else {
		verify_not_reached("Invalid hint \"%s\" passed to collection asset unpacker.", hint);
	}
}

static void pack_collection_asset(OutputStream& dest, const CollectionAsset& src, BuildConfig config, const char* hint) {
	const char* type = next_hint(&hint);
	if(strcmp(type, "texlist") == 0) {
		pack_texture_list(dest, src, config, hint);
	} else if(strcmp(type, "subtitles") == 0) {
		pack_subtitles(dest, src, config);
	} else {
		verify_not_reached("Invalid hint \"%s\" passed to collection asset packer.", hint);
	}
}

static void unpack_texture_list(CollectionAsset& dest, InputStream& src, BuildConfig config, const char* hint) {
	s32 count = src.read<s32>(0);
	verify(count < 0x1000, "texlist has too many elements and is probably corrupted.");
	src.seek(4);
	std::vector<s32> offsets = src.read_multiple<s32>(count);
	for(s32 i = 0; i < count; i++) {
		s32 offset = offsets[i];
		s32 size;
		if(i + 1 < count) {
			size = offsets[i + 1] - offsets[i];
		} else {
			size = src.size() - offsets[i];
		}
		unpack_asset(dest.child<TextureAsset>(i), src, ByteRange{offset, size}, config, hint);
	}
}

static void pack_texture_list(OutputStream& dest, const CollectionAsset& src, BuildConfig config, const char* hint) {
	s32 count;
	for(count = 0; count < 256; count++) {
		if(!src.has_child(count)) {
			break;
		}
	}
	dest.write<s32>(count);
	
	std::vector<s32> offsets(count);
	dest.write_v(offsets);
	
	for(s32 i = 0; i < 256; i++) {
		if(src.has_child(i)) {
			offsets[i] = pack_asset<ByteRange>(dest, src.get_child(i), config, 0x10, hint).offset;
		} else {
			break;
		}
	}
	
	dest.seek(4);
	dest.write_v(offsets);
}
