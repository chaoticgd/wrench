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

#include "subtitles.h"

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
	/* 0x4 */ s16 text_offset_e;
	/* 0x6 */ s16 text_offset_f;
	/* 0x8 */ s16 text_offset_g;
	/* 0xa */ s16 text_offset_s;
	/* 0xc */ s16 text_offset_i;
	/* 0xc */ s16 text_offset_j;
	/* 0xc */ s16 text_offset_k;
)

void unpack_subtitles(CollectionAsset& dest, InputStream& src, BuildConfig config)
{
	std::vector<u8> bytes = src.read_multiple<u8>(0, src.size());
	Buffer buffer(bytes);
	if (config.game() == Game::GC) {
		for (s32 i = 0;; i++) {
			GcSubtitleHeader header = buffer.read<GcSubtitleHeader>(i * sizeof(GcSubtitleHeader), "subtitle");
			if (header.start_frame > -1 && header.stop_frame > -1) {
				SubtitleAsset& subtitle = dest.child<SubtitleAsset>(i);
				// TODO: Convert to seconds.
				subtitle.set_start_time(header.start_frame / config.half_framerate());
				subtitle.set_stop_time(header.stop_frame / config.half_framerate());
				subtitle.set_text_e(buffer.read_string(header.text_offset_e));
				subtitle.set_text_f(buffer.read_string(header.text_offset_f));
				subtitle.set_text_g(buffer.read_string(header.text_offset_g));
				subtitle.set_text_s(buffer.read_string(header.text_offset_s));
				subtitle.set_text_i(buffer.read_string(header.text_offset_i));
				subtitle.set_encoding_e("raw");
				subtitle.set_encoding_f("raw");
				subtitle.set_encoding_g("raw");
				subtitle.set_encoding_i("raw");
				subtitle.set_encoding_s("raw");
			} else {
				break;
			}
		}
	} else if (config.game() == Game::UYA || config.game() == Game::DL) {
		s64 table_end = src.size();
		for (s32 i = 0; i * sizeof(UyaDlSubtitleHeader) < table_end; i++) {
			UyaDlSubtitleHeader header = buffer.read<UyaDlSubtitleHeader>(i * sizeof(UyaDlSubtitleHeader), "subtitle");
			// TODO: Convert to seconds.
			SubtitleAsset& subtitle = dest.child<SubtitleAsset>(i);
			subtitle.set_start_time(header.start_frame / config.half_framerate());
			subtitle.set_stop_time(header.stop_frame / config.half_framerate());
			if (header.text_offset_e > 0) {
				subtitle.set_text_e(buffer.read_string(header.text_offset_e));
			}
			if (header.text_offset_f > 0) {
				subtitle.set_text_f(buffer.read_string(header.text_offset_f));
			}
			if (header.text_offset_g > 0) {
				subtitle.set_text_g(buffer.read_string(header.text_offset_g));
			}
			if (header.text_offset_s > 0) {
				subtitle.set_text_s(buffer.read_string(header.text_offset_s));
			}
			if (header.text_offset_i > 0) {
				subtitle.set_text_i(buffer.read_string(header.text_offset_i));
			}
			if (header.text_offset_j > 0) {
				subtitle.set_text_j(buffer.read_string(header.text_offset_j));
			}
			if (header.text_offset_k > 0) {
				subtitle.set_text_k(buffer.read_string(header.text_offset_k));
			}
			subtitle.set_encoding_e("raw");
			subtitle.set_encoding_f("raw");
			subtitle.set_encoding_g("raw");
			subtitle.set_encoding_i("raw");
			subtitle.set_encoding_s("raw");
			subtitle.set_encoding_j("raw");
			subtitle.set_encoding_k("raw");
			if (i == 0) {
				table_end = header.text_offset_e;
			}
		}
	} else {
		verify_fatal("Invalid game.");
	}
}

void pack_subtitles(OutputStream& dest, const CollectionAsset& src, BuildConfig config)
{
	s32 subtitle_count = 0;
	for (s32 i = 0; i < 1024; i++) {
		if (src.has_child(i)) {
			subtitle_count++;
		} else {
			break;
		}
	}
	
	if (config.game() == Game::GC) {
		s64 table_ofs = dest.alloc_multiple<GcSubtitleHeader>(subtitle_count);
		dest.write<GcSubtitleHeader>({-1, -1});
		
		for (s32 i = 0; i < 1024; i++) {
			if (src.has_child(i)) {
				const SubtitleAsset& subtitle = src.get_child(i).as<SubtitleAsset>();
				GcSubtitleHeader header = {};
				header.start_frame = (s16) roundf(subtitle.start_time() * config.half_framerate());
				header.stop_frame = (s16) roundf(subtitle.stop_time() * config.half_framerate());
				
				std::string empty;
				
				dest.pad(4, 0);
				header.text_offset_e = (s16) dest.tell();
				std::string text_e = subtitle.text_e(empty);
				dest.write_n((u8*) text_e.c_str(), text_e.size() + 1);
				
				dest.pad(4, 0);
				header.text_offset_f = (s16) dest.tell();
				std::string text_f = subtitle.text_f(empty);
				dest.write_n((u8*) text_f.c_str(), text_f.size() + 1);
				
				dest.pad(4, 0);
				header.text_offset_g = (s16) dest.tell();
				std::string text_g = subtitle.text_g(empty);
				dest.write_n((u8*) text_g.c_str(), text_g.size() + 1);
				
				dest.pad(4, 0);
				header.text_offset_s = (s16) dest.tell();
				std::string text_s = subtitle.text_s(empty);
				dest.write_n((u8*) text_s.c_str(), text_s.size() + 1);
				
				dest.pad(4, 0);
				header.text_offset_i = (s16) dest.tell();
				std::string text_i = subtitle.text_i(empty);
				dest.write_n((u8*) text_i.c_str(), text_i.size() + 1);
				
				dest.write<GcSubtitleHeader>(table_ofs + i * sizeof(GcSubtitleHeader), header);
			} else {
				break;
			}
		}
	} else if (config.game() == Game::UYA || config.game() == Game::DL) {
		s64 table_ofs = dest.alloc_multiple<UyaDlSubtitleHeader>(subtitle_count);
		
		for (s32 i = 0; i < 1024; i++) {
			if (src.has_child(i)) {
				const SubtitleAsset& subtitle = src.get_child(i).as<SubtitleAsset>();
				UyaDlSubtitleHeader header = {};
				header.start_frame = (s16) roundf(subtitle.start_time() * config.half_framerate());
				header.stop_frame = (s16) roundf(subtitle.stop_time() * config.half_framerate());
				
				if (subtitle.has_text_e()) {
					header.text_offset_e = (s16) dest.tell();
					std::string text_e = subtitle.text_e();
					dest.write_n((u8*) text_e.c_str(), text_e.size() + 1);
				} else {
					header.text_offset_e = 0xcccc;
				}
				
				if (subtitle.has_text_f()) {
					header.text_offset_f = (s16) dest.tell();
					std::string text_f = subtitle.text_f();
					dest.write_n((u8*) text_f.c_str(), text_f.size() + 1);
				} else {
					header.text_offset_f = 0xcccc;
				}
				
				if (subtitle.has_text_g()) {
					header.text_offset_g = (s16) dest.tell();
					std::string text_g = subtitle.text_g();
					dest.write_n((u8*) text_g.c_str(), text_g.size() + 1);
				} else {
					header.text_offset_g = 0xcccc;
				}
				
				if (subtitle.has_text_s()) {
					header.text_offset_s = (s16) dest.tell();
					std::string text_s = subtitle.text_s();
					dest.write_n((u8*) text_s.c_str(), text_s.size() + 1);
				} else {
					header.text_offset_s = 0xcccc;
				}
				
				if (subtitle.has_text_i()) {
					header.text_offset_i = (s16) dest.tell();
					std::string text_i = subtitle.text_i();
					dest.write_n((u8*) text_i.c_str(), text_i.size() + 1);
				} else {
					header.text_offset_i = 0xcccc;
				}
				
				if (subtitle.has_text_j()) {
					header.text_offset_j = (s16) dest.tell();
					std::string text_j = subtitle.text_j();
					dest.write_n((u8*) text_j.c_str(), text_j.size() + 1);
				} else {
					header.text_offset_j = 0xcccc;
				}
				
				if (subtitle.has_text_k()) {
					header.text_offset_k = (s16) dest.tell();
					std::string text_k = subtitle.text_k();
					dest.write_n((u8*) text_k.c_str(), text_k.size() + 1);
				} else {
					header.text_offset_k = 0xcccc;
				}
				
				dest.write<UyaDlSubtitleHeader>(table_ofs + i * sizeof(UyaDlSubtitleHeader), header);
			} else {
				break;
			}
		}
	} else {
		verify_fatal("Invalid game.");
	}
}
