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

#include "../app.h"

bool game_world::is_selected(object_id id) const {
	return selection.contains(id);
}

void game_world::read(stream* src) {
	auto header = src->read<world_header>(0);
	
	ship = src->read<world_ship_data>(header.ship);
	
	// Read game strings.
	auto read_language = [&](uint32_t offset) {
		std::vector<game_string> language;
	
		auto table = src->read<world_string_table_header>(offset);
		std::vector<world_string_table_entry> entries(table.num_strings);
		src->read_v(entries);
		
		for(world_string_table_entry& entry : entries) {
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
	auto tie_table = src->read<world_object_table>(header.ties);
	_object_store.ties.resize(tie_table.num_elements);
	src->read_v(_object_store.ties);

	auto shrub_table = src->read<world_object_table>(header.shrubs);
	_object_store.shrubs.resize(shrub_table.num_elements);
	src->read_v(_object_store.shrubs);
	
	auto moby_table = src->read<world_object_table>(header.mobies);
	_object_store.mobies.resize(moby_table.num_elements);
	src->read_v(_object_store.mobies);
	
	// Read splines.
	auto spline_table = src->read<world_spline_table_header>(header.splines);
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
			object.points.emplace_back(x, y, z);
			src->seek(src->tell() + 4);
		}
		
		_object_store.splines.push_back(object);
	}
	
	// Assign internal ID's to all the objects.
	for_each_object_type([&]<typename T>() {
		for(std::size_t i = 0; i < objects_of_type<T>().size(); i++) {
			object_id id{_next_object_id++};
			member_of_type<T>(_object_mappings).id_to_index[id] = i;
			member_of_type<T>(_object_mappings).index_to_id[i] = id;
		}
	});
}

level_file_header level_read_file_header_or_throw(iso_stream* iso, std::size_t offset) {
	auto file_header = level_read_file_header(iso, offset);
	if(!file_header) {
		throw stream_format_error("Invalid level file header in ToC!");
	}
	return *file_header;
}

level::level(iso_stream* iso, toc_level index)
	: _index(index),
	  _file_header(level_read_file_header_or_throw(iso, index.main_part.bytes())),
	  _file(iso, _file_header.base_offset, index.main_part_size.bytes()) {
	_file.name = "LEVEL" + std::to_string(index.level_table_index) + ".WAD";
	
	auto primary_header = _file.read<level_primary_header>(_file_header.primary_header_offset);

	code_segment.header = _file.read<level_code_segment_header>
		(_file_header.primary_header_offset + primary_header.code_segment_offset);
	code_segment.bytes.resize(primary_header.code_segment_size - sizeof(level_code_segment_header));
	_file.read_v(code_segment.bytes);

	_world_segment = iso->get_decompressed(_file_header.base_offset + _file_header.moby_segment_offset);
	_world_segment->name = "World Segment";
	if(config::get().debug.stream_tracing) {
		// Install a tracepoint for the world segment so we can log reads.
		_world_segment_tracepoint.emplace(_world_segment);
		_world_segment = &(*_world_segment_tracepoint);
	}
	world.read(_world_segment);
	
	_asset_segment = iso->get_decompressed
		(_file_header.base_offset + _file_header.primary_header_offset + primary_header.asset_wad, true);
	_asset_segment->name = "Asset Segment";
	
	if(config::get().debug.stream_tracing) {
		// Install a tracepoint for the asset segment so we can log reads.
		_asset_segment_tracepoint.emplace(_asset_segment);
		_asset_segment = &(*_asset_segment_tracepoint);
	}
	
	uint32_t asset_base = _file_header.primary_header_offset + primary_header.asset_header;
	auto asset_header = _file.read<level_asset_header>(asset_base);
	
	uint32_t mdl_base = asset_base + asset_header.models;
	_file.seek(mdl_base);
	
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
	
	for(std::size_t i = 0; i < asset_header.num_models; i++) {
		auto entry = _file.read<model_entry>(mdl_base + sizeof(model_entry) * i);
		if(entry.offset_in_asset_wad == 0) {
			continue;
		}
		
		packed_struct(asset_model_header,
			uint32_t rel_offset;
			uint8_t unknown_4;
			uint8_t unknown_5;
			uint8_t unknown_6;
			uint8_t num_submodels;
			uint32_t unknown_8;
			uint32_t unknown_c;
		)
		
		auto model_header = _asset_segment->read<asset_model_header>(entry.offset_in_asset_wad);
		uint32_t rel_offset = model_header.rel_offset;
		uint32_t abs_offset = entry.offset_in_asset_wad + rel_offset;
		std::vector<std::size_t> submodel_counts {
			model_header.num_submodels
		};
		if(rel_offset == 0) {
			continue;
		}
		moby_model& model = moby_models.emplace_back(_asset_segment, abs_offset, 0, 0, submodel_counts);
		model.set_name("class " + std::to_string(entry.class_num));
		model.read();
		
		uint32_t class_num = entry.class_num;
		moby_class_to_model.emplace(class_num, moby_models.size() - 1);
	}
	
	_file.seek(asset_base + asset_header.mipmap_offset);
	std::size_t little_texture_base =
		_file_header.primary_header_offset + primary_header.tex_pixel_data_base;
	std::size_t last_palette_offset = 0;
	for(std::size_t i = 0; i < asset_header.mipmap_count; i++) {
		auto entry = _file.read<level_mipmap_entry>();
		auto abs_offset = little_texture_base + entry.offset_1;
		if(entry.width == 0) {
			last_palette_offset = abs_offset;
			continue;
		}
		mipmap_textures.emplace_back(&_file, abs_offset, &_file, last_palette_offset, vec2i { entry.width, entry.height });
	}
	
	auto load_texture_table = [&](stream& backing, std::size_t offset, std::size_t count) {
		std::vector<texture> textures;
		backing.seek(asset_base + offset);
		for(std::size_t i = 0; i < count; i++) {
			auto entry = backing.read<level_texture_entry>();
			auto ptr = asset_header.tex_data_in_asset_wad + entry.ptr;
			auto palette = little_texture_base + entry.palette * 0x100;
			textures.emplace_back(_asset_segment, ptr, &_file, palette, vec2i { entry.width, entry.height });
		}
		return textures;
	};
	
	terrain_textures = load_texture_table(_file, asset_header.terrain_texture_offset, asset_header.terrain_texture_count);
	tie_textures = load_texture_table(_file, asset_header.tie_texture_offset, asset_header.tie_texture_count);
	sprite_textures = load_texture_table(_file, asset_header.sprite_texture_offset, asset_header.sprite_texture_count);

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

	auto tfrag_head = _asset_segment->read<tfrag_header>(0);
	_asset_segment->seek(tfrag_head.entry_list_offset);

	for (std::size_t i = 0; i < tfrag_head.count; i++) {
		auto entry = _asset_segment->read<tfrag_entry>();
		tfrag frag = tfrag(_asset_segment, tfrag_head.entry_list_offset + entry.offset, entry.vertex_offset, entry.vertex_count);
		frag.update();
		tfrags.emplace_back(std::move(frag));
	}
}

stream* level::moby_stream() {
	return _world_segment;
}
