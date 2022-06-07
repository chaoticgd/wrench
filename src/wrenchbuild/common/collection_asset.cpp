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

static void unpack_collection_asset(CollectionAsset& dest, InputStream& src, Game game, const char* hint);
static void pack_collection_asset(OutputStream& dest, const CollectionAsset& src, Game game, const char* hint);
static void unpack_texture_list(CollectionAsset& dest, InputStream& src, Game game, const char* hint);
static void pack_texture_list(OutputStream& dest, const CollectionAsset& src, Game game, const char* hint);
static void unpack_subtitles(CollectionAsset& dest, InputStream& src, Game game);
static void pack_subtitles(OutputStream& dest, const CollectionAsset& src, Game game);

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

static void unpack_collection_asset(CollectionAsset& dest, InputStream& src, Game game, const char* hint) {
	const char* type = next_hint(&hint);
	if(strcmp(type, "texlist") == 0) {
		unpack_texture_list(dest, src, game, hint);
	} else if(strcmp(type, "subtitles") == 0) {
		unpack_subtitles(dest, src, game);
	} else {
		verify_not_reached("Invalid hint passed to collection asset unpacker.");
	}
}

static void pack_collection_asset(OutputStream& dest, const CollectionAsset& src, Game game, const char* hint) {
	const char* type = next_hint(&hint);
	if(strcmp(type, "texlist") == 0) {
		pack_texture_list(dest, src, game, hint);
	} else if(strcmp(type, "subtitles") == 0) {
		pack_subtitles(dest, src, game);
	} else {
		verify_not_reached("Invalid hint passed to collection asset packer.");
	}
}

static void unpack_texture_list(CollectionAsset& dest, InputStream& src, Game game, const char* hint) {
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
		unpack_asset(dest.child<TextureAsset>(i), src, ByteRange{offset, size}, game, hint);
	}
}

static void pack_texture_list(OutputStream& dest, const CollectionAsset& src, Game game, const char* hint) {
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
			offsets[i] = pack_asset<ByteRange>(dest, src.get_child(i), game, 0x10, hint).offset;
		} else {
			break;
		}
	}
	
	dest.seek(4);
	dest.write_v(offsets);
}

packed_struct(GcSubtitleHeader,
	/* 0x0 */ s16 start_frame;
	/* 0x2 */ s16 stop_frame;
	/* 0x4 */ s16 text_offset_e;
	/* 0x6 */ s16 text_offset_f;
	/* 0x8 */ s16 text_offset_g;
	/* 0xa */ s16 text_offset_s;
	/* 0xc */ s16 text_offset_i;
	/* 0xe */ s16 pad;
)

packed_struct(UyaDlSubtitleHeader,
	/* 0x0 */ s16 start_frame;
	/* 0x2 */ s16 stop_frame;
	/* 0x4 */ s16 text_offsets[7];
)

static void unpack_subtitles(CollectionAsset& dest, InputStream& src, Game game) {
	std::vector<u8> bytes = src.read_multiple<u8>(0, src.size());
	Buffer buffer(bytes);
	if(game == Game::RAC2) {
		for(s32 i = 0;; i++) {
			GcSubtitleHeader header = buffer.read<GcSubtitleHeader>(i * sizeof(GcSubtitleHeader), "subtitle");
			if(header.start_frame > -1 && header.stop_frame > -1) {
				SubtitleAsset& subtitle = dest.child<SubtitleAsset>(i);
				// TODO: Convert to seconds.
				subtitle.set_start_time(header.start_frame);
				subtitle.set_stop_time(header.stop_frame);
				subtitle.set_text_e(buffer.read_string(header.text_offset_e));
				subtitle.set_text_f(buffer.read_string(header.text_offset_f));
				subtitle.set_text_g(buffer.read_string(header.text_offset_g));
				subtitle.set_text_i(buffer.read_string(header.text_offset_i));
				subtitle.set_text_s(buffer.read_string(header.text_offset_s));
				subtitle.set_encoding_e("raw");
				subtitle.set_encoding_f("raw");
				subtitle.set_encoding_g("raw");
				subtitle.set_encoding_i("raw");
				subtitle.set_encoding_s("raw");
			} else {
				break;
			}
		}
	} else if(game == Game::RAC3 || game == Game::DL) {
		
	} else {
		assert(0);
	}
}

static void pack_subtitles(OutputStream& dest, const CollectionAsset& src, Game game) {
	if(game == Game::RAC2) {
		s32 subtitle_count = 0;
		for(s32 i = 0; i < 1024; i++) {
			if(src.has_child(i)) {
				subtitle_count++;
			} else {
				break;
			}
		}
		
		s64 table_ofs = dest.alloc_multiple<GcSubtitleHeader>(subtitle_count);
		dest.write<GcSubtitleHeader>({-1, -1});
		
		for(s32 i = 0; i < 1024; i++) {
			if(src.has_child(i)) {
				const SubtitleAsset& subtitle = src.get_child(i).as<SubtitleAsset>();
				GcSubtitleHeader header = {};
				header.start_frame = subtitle.start_time();
				header.stop_frame = subtitle.stop_time();
				
				dest.pad(4, 0);
				header.text_offset_e = (s16) dest.tell();
				std::string text_e = subtitle.text_e();
				dest.write_n((u8*) text_e.c_str(), text_e.size() + 1);
				
				dest.pad(4, 0);
				header.text_offset_f = (s16) dest.tell();
				std::string text_f = subtitle.text_f();
				dest.write_n((u8*) text_f.c_str(), text_f.size() + 1);
				
				dest.pad(4, 0);
				header.text_offset_g = (s16) dest.tell();
				std::string text_g = subtitle.text_g();
				dest.write_n((u8*) text_g.c_str(), text_g.size() + 1);
				
				dest.pad(4, 0);
				header.text_offset_s = (s16) dest.tell();
				std::string text_s = subtitle.text_s();
				dest.write_n((u8*) text_s.c_str(), text_s.size() + 1);
				
				dest.pad(4, 0);
				header.text_offset_i = (s16) dest.tell();
				std::string text_i = subtitle.text_i();
				dest.write_n((u8*) text_i.c_str(), text_i.size() + 1);
				
				dest.write<GcSubtitleHeader>(table_ofs + i * sizeof(GcSubtitleHeader), header);
			}
		}
	} else if(game == Game::RAC3 || game == Game::DL) {
		
	} else {
		assert(0);
	}
}
