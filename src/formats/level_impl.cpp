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

#include "level_impl.h"

bool game_world::is_selected(object_id id) const {
	return std::find(selection.begin(), selection.end(), id) != selection.end();
}

void game_world::read(stream* src) {
	auto header = src->read<fmt::header>(0);
	
	// Read game strings.
	auto read_language = [=](uint32_t offset) {
		std::vector<game_string> language;
	
		auto table = src->read<fmt::string_table_header>(offset);
		std::vector<fmt::string_table_entry> entries(table.num_strings);
		src->read_v(entries);
		
		for(fmt::string_table_entry& entry : entries) {
			src->seek(offset + entry.string.value);
			language.push_back({ entry.id, src->read_string() });
		}
		
		return language;
	};
	_languages["English"] = read_language(header.english_strings);
	_languages["French" ] = read_language(header.french_strings);
	_languages["German" ] = read_language(header.german_strings);
	_languages["Spanish"] = read_language(header.spanish_strings);
	_languages["Italian"] = read_language(header.italian_strings);
	
	// Read point objects.
	auto tie_table = src->read<fmt::object_table>(header.ties);
	_ties.resize(tie_table.num_elements);
	src->read_v(_ties);

	auto shrub_table = src->read<fmt::object_table>(header.shrubs);
	_shrubs.resize(shrub_table.num_elements);
	src->read_v(_shrubs);
	
	auto moby_table = src->read<fmt::object_table>(header.mobies);
	_mobies.resize(moby_table.num_elements);
	src->read_v(_mobies);
	
	// Read splines.
	auto spline_table = src->read<fmt::spline_table_header>(header.splines);
	for(std::size_t i = 0; i < spline_table.num_splines; i++) {
		uint32_t spline_offset = src->read<uint32_t>(header.splines + 0x10 + i * 4);
		uint32_t num_vertices = src->read<uint32_t>
			(header.splines + spline_table.data_offset + spline_offset);
		
		spline object;
		
		src->seek(src->tell() + 0xc);
		for(std::size_t i = 0; i < num_vertices; i++) {
			float x = src->read<float>();
			float y = src->read<float>();
			float z = src->read<float>();
			object.emplace_back(x, y, z);
			src->seek(src->tell() + 4);
		}
		
		_splines.push_back(object);
	}
}

void game_world::write() {
	// TODO
}

level::level(iso_stream* iso, std::size_t offset, std::size_t size, std::string display_name)
	: offset(offset),
	  _backing(iso, offset, size) {
	
	auto file_header = _backing.read<fmt::file_header>(0);
	
	uint32_t primhdr_offset = file_header.primary_header.bytes();
	auto primary_header = _backing.read<fmt::primary_header>(primhdr_offset);

	_moby_stream = iso->get_decompressed(offset + file_header.moby_segment.bytes());
	world.read(_moby_stream);
	
	stream* asset_seg = iso->get_decompressed
		(offset + primhdr_offset + primary_header.asset_wad.value, true);
	
	uint32_t snd_base = file_header.primary_header.bytes() + primary_header.snd_header.value;
	auto snd_header = _backing.read<fmt::secondary_header>(snd_base);
	
	uint32_t mdl_base = snd_base + snd_header.models;
	_backing.seek(mdl_base);
	
	packed_struct(model_entry,
		uint32_t offset_in_asset_wad;
		uint32_t class_num;
		uint32_t unknown_8;
		uint32_t unknown_c;
		uint32_t unknown_10;
		uint32_t unknown_14;
		uint32_t unknown_18;
		uint32_t unknown_1c;
	)
	
	for(std::size_t i = 0; i < snd_header.num_models; i++) {
		auto entry = _backing.read<model_entry>(mdl_base + sizeof(model_entry) * i);
		if(entry.offset_in_asset_wad == 0) {
			continue;
		}
		
		packed_struct(asset_mdl_hdr,
			uint32_t rel_offset;
			uint8_t unknown_4;
			uint8_t unknown_5;
			uint8_t unknown_6;
			uint8_t num_submodels;
			uint32_t unknown_8;
			uint32_t unknown_c;
		)
		
		auto model_header = asset_seg->read<asset_mdl_hdr>(entry.offset_in_asset_wad);
		uint32_t rel_offset = model_header.rel_offset;
		uint32_t abs_offset = entry.offset_in_asset_wad + rel_offset;
		moby_models.emplace_back(asset_seg, abs_offset, 0, model_header.num_submodels);
		
		uint32_t class_num = entry.class_num;
		moby_class_to_model.emplace(class_num, moby_models.size() - 1);
	}
	
	packed_struct(texture_entry,
		uint32_t ptr;
		uint16_t width;
		uint16_t height;
		uint32_t palette;
		uint32_t field_c;
	);
	
	auto load_texture_table = [=](stream& backing, std::size_t offset, std::size_t count) {
		std::vector<texture> textures;
		backing.seek(snd_base + offset);
		for(std::size_t i = 0; i < count; i++) {
			auto entry = backing.read<texture_entry>();
			auto ptr = snd_header.tex_data_in_asset_wad + entry.ptr;
			textures.emplace_back(asset_seg, ptr, ptr, vec2i { entry.width, entry.height });
		}
		return textures;
	};
	
	terrain_textures = load_texture_table(_backing, snd_header.terrain_texture_offset, snd_header.terrain_texture_count);
	tie_textures = load_texture_table(_backing, snd_header.tie_texture_offset, snd_header.tie_texture_count);
	sprite_textures = load_texture_table(_backing, snd_header.sprite_texture_offset, snd_header.sprite_texture_count);

	packed_struct(tfrag_header,
		uint32_t entry_list_offset; //0x00
		uint32_t count; //0x04
		uint32_t unknown_8; //0x08
		uint32_t count2; //0x0c
	// 0x30 padding
	);

	packed_struct(tfrag_entry,
		uint32_t unknown_0; //0x00
		uint32_t unknown_4; //0x04
		uint32_t unknown_8; //0x08
		uint32_t unknown_c; //0x0c
		uint32_t offset; //0x10 offset from start of tfrag_entry list
		uint16_t unknown_14;
		uint16_t unknown_16;
		uint32_t unknown_18;
		uint16_t unknown_1c;
		uint16_t color_offset;
		uint32_t unknown_20;
		uint32_t unknown_24;
		uint32_t unknown_28;
		uint16_t vertex_count;
		uint16_t vertex_offset;
		uint16_t unknown_30;
		uint16_t unknown_32;
		uint32_t unknown_34;
		uint32_t unknown_38;
		uint8_t color_count;
		uint8_t unknown_3d;
		uint8_t unknown_3e;
		uint8_t unknown_3f;
	);

	auto tfrag_head = asset_seg->read<tfrag_header>(0);
	asset_seg->seek(tfrag_head.entry_list_offset);

	for (int i = 0; i < tfrag_head.count; i++) {
		auto entry = asset_seg->read<tfrag_entry>();
		tfrag frag = tfrag(asset_seg, tfrag_head.entry_list_offset + entry.offset, entry.vertex_offset, entry.vertex_count);
		tfrags.emplace_back(frag);
	}
}

stream* level::moby_stream() {
	return _moby_stream;
}
