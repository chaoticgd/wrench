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

level::level(iso_stream* iso, toc_level index)
	: _index(index),
	  _file_header(read_file_header(iso, index.main_part.bytes())),
	  _file(iso, _file_header.base_offset, index.main_part_size.bytes()) {
	_file.name = "LEVEL" + std::to_string(index.level_table_index) + ".WAD";
	
	_primary_header = _file.read<level_primary_header>(_file_header.primary_header_offset);

	if(_file_header.type != level_type::RAC4) {
		code_segment.header = _file.read<level_code_segment_header>
			(_file_header.primary_header_offset + _primary_header.code_segment_offset);
		code_segment.bytes.resize(_primary_header.code_segment_size - sizeof(level_code_segment_header));
		_file.read_v(code_segment.bytes);
	}

	_world_segment = iso->get_decompressed(_file_header.base_offset + _file_header.world_segment_offset);
	_world_segment->name = "World Segment";
	if(config::get().debug.stream_tracing) {
		// Install a tracepoint for the world segment so we can log reads.
		_world_segment_tracepoint.emplace(_world_segment);
		_world_segment = &(*_world_segment_tracepoint);
	}
	
	world.backing = _world_segment;
	switch(_file_header.type) {
		case level_type::RAC23:
		case level_type::RAC2_68:
			world.read_rac23();
		case level_type::RAC4:
			world.read_rac4();
	}
	
	if(_file_header.type == level_type::RAC4) {
		return;
	}
	
	_asset_segment = iso->get_decompressed
		(_file_header.base_offset + _file_header.primary_header_offset + _primary_header.asset_wad);
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
	read_loading_screen_textures(iso);
}

void level::write() {
	world.write_rac2();
}

level_file_header level::read_file_header(stream* src, std::size_t offset) {
	level_file_header result;
	src->seek(offset);
	uint32_t magic = src->peek<uint32_t>();
	switch(magic) {
		case 0x60: {
			auto file_header = src->read<level_file_header_rac23>();
			result.type = level_type::RAC23;
			result.base_offset = file_header.base_offset.bytes();
			result.level_number = file_header.level_number;
			result.primary_header_offset = file_header.primary_header.bytes();
			result.world_segment_offset = file_header.world_segment.bytes();
			break;
		}
		case 0x68: {
			auto file_header = src->read<level_file_header_rac2_68>();
			result.type = level_type::RAC2_68;
			result.base_offset = file_header.base_offset.bytes();
			result.level_number = file_header.level_number;
			result.primary_header_offset = file_header.primary_header.bytes();
			result.world_segment_offset = file_header.world_segment.bytes();
			break;
		}
		case 0xc68: {
			auto file_header = src->read<level_file_header_rac4>();
			result.type = level_type::RAC4;
			result.base_offset = file_header.base_offset.bytes();
			result.level_number = file_header.level_number;
			result.primary_header_offset = file_header.primary_header.bytes();
			result.world_segment_offset = file_header.world_segment.bytes();
			break;
		}
		default: {
			throw stream_format_error("Invalid level file header!");
		}
	}
	return result;
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

void level::read_loading_screen_textures(iso_stream* iso) {
	size_t primary_header_offset = _file_header.base_offset + _file_header.primary_header_offset;
	size_t load_wad_offset = primary_header_offset + _primary_header.loading_screen_textures_offset;
	if(load_wad_offset > iso->size()) {
		fprintf(stderr, "warning: Failed to read loading screen textures (seek pos > iso size).\n");
		return;
	}
	
	char wad_magic[3];
	iso->seek(load_wad_offset);
	iso->read_n(wad_magic, 3);
	if(std::memcmp(wad_magic, "WAD", 3) == 0) {
		stream* textures = iso->get_decompressed(load_wad_offset);
		loading_screen_textures = read_pif_list(textures, 0);
	} else {
		fprintf(stderr, "warning: Failed to read loading screen textures (missing magic bytes).\n");
	}
}

stream* level::moby_stream() {
	return _world_segment;
}
