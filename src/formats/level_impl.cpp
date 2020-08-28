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
	
	_primary_header = _file.read<level_primary_header>(_file_header.primary_header_offset);

	code_segment.header = _file.read<level_code_segment_header>
		(_file_header.primary_header_offset + _primary_header.code_segment_offset);
	code_segment.bytes.resize(_primary_header.code_segment_size - sizeof(level_code_segment_header));
	_file.read_v(code_segment.bytes);

	_world_segment = iso->get_decompressed(_file_header.base_offset + _file_header.moby_segment_offset);
	_world_segment->name = "World Segment";
	if(config::get().debug.stream_tracing) {
		// Install a tracepoint for the world segment so we can log reads.
		_world_segment_tracepoint.emplace(_world_segment);
		_world_segment = &(*_world_segment_tracepoint);
	}
	
	auto world_hdr = _world_segment->read<world_header>(0);
	properties = _world_segment->read<world_properties>(world_hdr.properties);
	read_strings(world_hdr);
	read_ties(world_hdr.ties);
	read_shrubs(world_hdr.shrubs);
	read_mobies(world_hdr.mobies);
	read_pvars(world_hdr.pvar_table, world_hdr.pvar_data);
	read_splines(world_hdr.splines);
	
	_asset_segment = iso->get_decompressed
		(_file_header.base_offset + _file_header.primary_header_offset + _primary_header.asset_wad, true);
	_asset_segment->name = "Asset Segment";
	
	if(config::get().debug.stream_tracing) {
		// Install a tracepoint for the asset segment so we can log reads.
		_asset_segment_tracepoint.emplace(_asset_segment);
		_asset_segment = &(*_asset_segment_tracepoint);
	}
	
	uint32_t asset_offset = _file_header.primary_header_offset + _primary_header.asset_header;
	auto asset_header = _file.read<level_asset_header>(asset_offset);
	
	read_moby_models(asset_offset, asset_header);
	read_textures(asset_offset, asset_header);
	read_tfrags();
	
	read_hud_banks(iso);
}

void level::read_strings(world_header header) {
	auto read_language = [&](uint32_t offset) {
		std::vector<game_string> language;
	
		auto table = _world_segment->read<world_string_table_header>(offset);
		std::vector<world_string_table_entry> entries(table.num_strings);
		_world_segment->read_v(entries);
		
		for(world_string_table_entry& entry : entries) {
			_world_segment->seek(offset + entry.string.value);
			language.push_back({ entry.id, _world_segment->read_string() });
		}
		
		return language;
	};
	game_strings[0] = read_language(header.english_strings);
	game_strings[1] = read_language(header.french_strings);
	game_strings[2] = read_language(header.german_strings);
	game_strings[3] = read_language(header.spanish_strings);
	game_strings[4] = read_language(header.italian_strings);
}

void level::read_ties(std::size_t offset) {
	auto tie_table = _world_segment->read<world_object_table>(offset);
	ties.resize(tie_table.count);
	for(tie_entity& tie : ties) {
		world_tie data = _world_segment->read<world_tie>();
		// entity
		tie.id = { _next_entity_id++ };
		tie.selected = false;
		// matrix_entity
		tie.local_to_world = data.local_to_world();
		// tie_entity
		tie.unknown_0 = data.unknown_0;
		tie.unknown_4 = data.unknown_4;
		tie.unknown_8 = data.unknown_8;
		tie.unknown_c = data.unknown_c;
		tie.unknown_50 = data.unknown_50;
		tie.uid = data.uid;
		tie.unknown_58 = data.unknown_58;
		tie.unknown_5c = data.unknown_5c;
	}
}

void level::read_shrubs(std::size_t offset) {
	auto shrub_table = _world_segment->read<world_object_table>(offset);
	shrubs.resize(shrub_table.count);
	for(shrub_entity& shrub : shrubs) {
		world_shrub data = _world_segment->read<world_shrub>();
		// entity
		shrub.id = { _next_entity_id++ };
		shrub.selected = false;
		// matrix_entity
		shrub.local_to_world = data.local_to_world();
		// shrub_entity
		shrub.unknown_0 = data.unknown_0;
		shrub.unknown_4 = data.unknown_4;
		shrub.unknown_8 = data.unknown_8;
		shrub.unknown_c = data.unknown_c;
		shrub.unknown_50 = data.unknown_50;
		shrub.unknown_54 = data.unknown_54;
		shrub.unknown_58 = data.unknown_58;
		shrub.unknown_5c = data.unknown_5c;
		shrub.unknown_60 = data.unknown_60;
		shrub.unknown_64 = data.unknown_64;
		shrub.unknown_68 = data.unknown_68;
		shrub.unknown_6c = data.unknown_6c;
	}
}

void level::read_mobies(std::size_t offset) {
	auto moby_table = _world_segment->read<world_object_table>(offset);
	mobies.resize(moby_table.count);
	for(moby_entity& moby : mobies) {
		world_moby data = _world_segment->read<world_moby>();
		// entity
		moby.id = { _next_entity_id++ };
		moby.selected = false;
		// euler_entity
		moby.position = data.position();
		moby.rotation = data.rotation();
		// moby_entity
		moby.size = data.size;
		moby.unknown_4 = data.unknown_4;
		moby.unknown_8 = data.unknown_8;
		moby.unknown_c = data.unknown_c;
		moby.uid = data.uid;
		moby.unknown_14 = data.unknown_14;
		moby.unknown_18 = data.unknown_18;
		moby.unknown_1c = data.unknown_1c;
		moby.unknown_20 = data.unknown_20;
		moby.unknown_24 = data.unknown_24;
		moby.class_num = data.class_num;
		moby.scale = data.scale;
		moby.unknown_30 = data.unknown_30;
		moby.unknown_34 = data.unknown_34;
		moby.unknown_38 = data.unknown_38;
		moby.unknown_3c = data.unknown_3c;
		moby.unknown_58 = data.unknown_58;
		moby.unknown_5c = data.unknown_5c;
		moby.unknown_60 = data.unknown_60;
		moby.unknown_64 = data.unknown_64;
		moby.pvar_index = data.pvar_index;
		moby.unknown_6c = data.unknown_6c;
		moby.unknown_70 = data.unknown_70;
		moby.unknown_74 = data.unknown_74;
		moby.unknown_78 = data.unknown_78;
		moby.unknown_7c = data.unknown_7c;
		moby.unknown_80 = data.unknown_80;
		moby.unknown_84 = data.unknown_84;
	}
}

void level::read_pvars(std::size_t table_offset, std::size_t data_offset) {
	int32_t pvar_count = 0;
	for(moby_entity& moby : mobies) {
		pvar_count = std::max(pvar_count, moby.pvar_index + 1);
	}
	
	std::vector<pvar_table_entry> table(pvar_count);
	_world_segment->seek(table_offset);
	_world_segment->read_v(table);
	
	for(pvar_table_entry& entry : table) {
		uint32_t size = entry.size;
		std::vector<uint8_t>& pvar = pvars.emplace_back(size);
		_world_segment->seek(data_offset + entry.offset);
		_world_segment->read_v(pvar);
	}
}


void level::read_splines(std::size_t offset) {
	auto spline_table = _world_segment->read<world_spline_table>(offset);
	
	std::vector<uint32_t> spline_offsets(spline_table.spline_count);
	_world_segment->read_v(spline_offsets);
	
	for(uint32_t spline_offset : spline_offsets) {
		auto spline_header = _world_segment->read<world_spline_header>
			(offset + spline_table.data_offset + spline_offset);
		
		spline_entity& spline = splines.emplace_back();
		spline.id = { _next_entity_id++ };
		spline.selected = false;
		spline.vertices.resize(spline_header.vertex_count);
		for(glm::vec4& vertex : spline.vertices) {
			vertex.x = _world_segment->read<float>();
			vertex.y = _world_segment->read<float>();
			vertex.z = _world_segment->read<float>();
			vertex.w = _world_segment->read<float>();
		}
	}
}

void level::read_moby_models(std::size_t asset_offset, level_asset_header asset_header) {
	uint32_t mdl_base = asset_offset + asset_header.moby_model_offset;
	_file.seek(mdl_base);
	
	for(std::size_t i = 0; i < asset_header.moby_model_count; i++) {
		auto entry = _file.read<level_moby_model_entry>(mdl_base + sizeof(level_moby_model_entry) * i);
		if(entry.offset_in_asset_wad == 0) {
			continue;
		}
		
		auto model_header = _asset_segment->read<moby_model_level_header>(entry.offset_in_asset_wad);
		uint32_t rel_offset = model_header.rel_offset;
		uint32_t abs_offset = entry.offset_in_asset_wad;
		if(rel_offset == 0) {
			continue;
		}
		moby_model& model = moby_models.emplace_back(_asset_segment, abs_offset, 0, moby_model_header_type::LEVEL);
		model.set_name("class " + std::to_string(entry.o_class));
		model.scale = model_header.scale;
		model.read();
		
		for(uint8_t texture : entry.textures) {
			if(texture == 0xff) {
				break;
			}
			
			model.texture_indices.push_back(texture);
		}
		
		uint32_t o_class = entry.o_class;
		moby_class_to_model.emplace(o_class, moby_models.size() - 1);
	}
}

void level::read_textures(std::size_t asset_offset, level_asset_header asset_header) {
	_file.seek(asset_offset + asset_header.mipmap_offset);
	std::size_t little_texture_base =
		_file_header.primary_header_offset + _primary_header.tex_pixel_data_base;
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
		backing.seek(asset_offset + offset);
		for(std::size_t i = 0; i < count; i++) {
			auto entry = backing.read<level_texture_entry>();
			auto ptr = asset_header.tex_data_in_asset_wad + entry.ptr;
			auto palette = little_texture_base + entry.palette * 0x100;
			textures.emplace_back(_asset_segment, ptr, &_file, palette, vec2i { entry.width, entry.height });
		}
		return textures;
	};
	
	terrain_textures = load_texture_table(_file, asset_header.terrain_texture_offset, asset_header.terrain_texture_count);
	moby_textures = load_texture_table(_file, asset_header.moby_texture_offset, asset_header.moby_texture_count);
	tie_textures = load_texture_table(_file, asset_header.tie_texture_offset, asset_header.tie_texture_count);
	shrub_textures = load_texture_table(_file, asset_header.shrub_texture_offset, asset_header.shrub_texture_count);
	sprite_textures = load_texture_table(_file, asset_header.sprite_texture_offset, asset_header.sprite_texture_count);
}

void level::read_tfrags() {
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

void level::read_hud_banks(iso_stream* iso) {
	const auto read_hud_bank = [&](int index, uint32_t relative_offset, uint32_t size) {
		if(size > 0x10) {
			uint32_t absolute_offset = _file_header.primary_header_offset + relative_offset;
			stream* bank = iso->get_decompressed(_file_header.base_offset + absolute_offset);
			bank->name = "HUD Bank " + std::to_string(index);
		}
	};
	
	//auto hud_header = _file.read<level_hud_header>(_file_header.primary_header_offset + _primary_header.hud_header_offset);
	read_hud_bank(0, _primary_header.hud_bank_0_offset, _primary_header.hud_bank_0_size);
	read_hud_bank(1, _primary_header.hud_bank_1_offset, _primary_header.hud_bank_1_size);
	read_hud_bank(2, _primary_header.hud_bank_2_offset, _primary_header.hud_bank_2_size);
	read_hud_bank(3, _primary_header.hud_bank_3_offset, _primary_header.hud_bank_3_size);
	read_hud_bank(4, _primary_header.hud_bank_4_offset, _primary_header.hud_bank_4_size);
}

stream* level::moby_stream() {
	return _world_segment;
}

void level::clear_selection() {
	for_each<entity>([&](entity& ent) {
		ent.selected = false;
	});
}

std::vector<entity_id> level::selected_entity_ids() {
	std::vector<entity_id> ids;
	for_each<entity>([&](entity& ent) {
		if(ent.selected) {
			ids.push_back(ent.id);
		}
	});
	return ids;
}
