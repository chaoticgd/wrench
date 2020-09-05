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
	read_world_segment(world_hdr);
	
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

void level::read_world_segment(world_header header) {
	properties = _world_segment->read<world_properties>(header.properties);
	
	// Strings
	const auto read_language = [&](uint32_t offset) {
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
	
	// Point entities
	auto tie_table = _world_segment->read<world_object_table>(header.ties);
	auto ties_in = _world_segment->read_multiple<world_tie>(tie_table.count);
	for(world_tie& on_disc : ties_in) {
		tie_entity& ent = ties.emplace_back();
		ent.id = { _next_entity_id++ };
		ent.selected = false;
		swap_tie(ent, on_disc);
	}
	
	auto shrub_table = _world_segment->read<world_object_table>(header.shrubs);
	auto shrubs_in = _world_segment->read_multiple<world_shrub>(shrub_table.count);
	for(world_shrub& on_disc : shrubs_in) {
		shrub_entity& ent = shrubs.emplace_back();
		ent.id = { _next_entity_id++ };
		ent.selected = false;
		swap_shrub(ent, on_disc);
	}
	
	auto moby_table = _world_segment->read<world_object_table>(header.mobies);
	auto mobies_in = _world_segment->read_multiple<world_moby>(moby_table.count);
	for(world_moby& on_disc : mobies_in) {
		moby_entity& ent = mobies.emplace_back();
		ent.id = { _next_entity_id++ };
		ent.selected = false;
		swap_moby(ent, on_disc);
	}
	
	// Pvars
	int32_t pvar_count = 0;
	for(moby_entity& moby : mobies) {
		pvar_count = std::max(pvar_count, moby.pvar_index + 1);
	}
	
	std::vector<pvar_table_entry> table(pvar_count);
	_world_segment->seek(header.pvar_table);
	_world_segment->read_v(table);
	
	for(pvar_table_entry& entry : table) {
		uint32_t size = entry.size;
		std::vector<uint8_t>& pvar = pvars.emplace_back(size);
		_world_segment->seek(header.pvar_data + entry.offset);
		_world_segment->read_v(pvar);
	}
	
	// Splines
	auto spline_table = _world_segment->read<world_spline_table>(header.splines);
	
	std::vector<uint32_t> spline_offsets(spline_table.spline_count);
	_world_segment->read_v(spline_offsets);
	
	for(uint32_t spline_offset : spline_offsets) {
		auto spline_header = _world_segment->read<world_spline_header>
			(header.splines + spline_table.data_offset + spline_offset);
		
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
			if(bank == nullptr) {
				fprintf(stderr, "warning: Failed to decompress HUD bank %d.\n", index);
				return;
			}
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
	std::size_t primary_header_offset = _file_header.base_offset + _file_header.primary_header_offset;
	stream* textures = iso->get_decompressed(primary_header_offset + _primary_header.loading_screen_textures_offset);
	if(textures == nullptr) {
		fprintf(stderr, "warning: Failed to decompress loading screen textures.\n");
		return;
	}
	loading_screen_textures = read_pif_list(textures, 0);
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

// We're using packed structs so we can't pass around references to fields so
// instead of std::swap we have to use this macro.
#define SWAP_PACKED(inmem, packed) \
	{ \
		auto p = packed; \
		packed = inmem; \
		inmem = p; \
	}

void swap_tie(tie_entity& l, world_tie& r) {
	// matrix_entity
	swap_mat4(l.local_to_world, r.local_to_world);
	// tie_entity
	SWAP_PACKED(l.unknown_0, r.unknown_0);
	SWAP_PACKED(l.unknown_4, r.unknown_4);
	SWAP_PACKED(l.unknown_8, r.unknown_8);
	SWAP_PACKED(l.unknown_c, r.unknown_c);
	SWAP_PACKED(l.unknown_50, r.unknown_50);
	SWAP_PACKED(l.uid, r.uid);
	SWAP_PACKED(l.unknown_58, r.unknown_58);
	SWAP_PACKED(l.unknown_5c, r.unknown_5c);
}

void swap_shrub(shrub_entity& l, world_shrub& r) {
	// matrix_entity
	swap_mat4(l.local_to_world, r.local_to_world);
	// shrub_entity
	SWAP_PACKED(l.unknown_0, r.unknown_0);
	SWAP_PACKED(l.unknown_4, r.unknown_4);
	SWAP_PACKED(l.unknown_8, r.unknown_8);
	SWAP_PACKED(l.unknown_c, r.unknown_c);
	SWAP_PACKED(l.unknown_50, r.unknown_50);
	SWAP_PACKED(l.unknown_54, r.unknown_54);
	SWAP_PACKED(l.unknown_58, r.unknown_58);
	SWAP_PACKED(l.unknown_5c, r.unknown_5c);
	SWAP_PACKED(l.unknown_60, r.unknown_60);
	SWAP_PACKED(l.unknown_64, r.unknown_64);
	SWAP_PACKED(l.unknown_68, r.unknown_68);
	SWAP_PACKED(l.unknown_6c, r.unknown_6c);
}

void swap_moby(moby_entity& l, world_moby& r) {
	// euler_entity
	swap_vec3(l.position, r.position);
	swap_vec3(l.rotation, r.rotation);
	// moby_entity
	SWAP_PACKED(l.size, r.size);
	SWAP_PACKED(l.unknown_4, r.unknown_4);
	SWAP_PACKED(l.unknown_8, r.unknown_8);
	SWAP_PACKED(l.unknown_c, r.unknown_c);
	SWAP_PACKED(l.uid, r.uid);
	SWAP_PACKED(l.unknown_14, r.unknown_14);
	SWAP_PACKED(l.unknown_18, r.unknown_18);
	SWAP_PACKED(l.unknown_1c, r.unknown_1c);
	SWAP_PACKED(l.unknown_20, r.unknown_20);
	SWAP_PACKED(l.unknown_24, r.unknown_24);
	SWAP_PACKED(l.class_num, r.class_num);
	SWAP_PACKED(l.scale, r.scale);
	SWAP_PACKED(l.unknown_30, r.unknown_30);
	SWAP_PACKED(l.unknown_34, r.unknown_34);
	SWAP_PACKED(l.unknown_38, r.unknown_38);
	SWAP_PACKED(l.unknown_3c, r.unknown_3c);
	SWAP_PACKED(l.unknown_58, r.unknown_58);
	SWAP_PACKED(l.unknown_5c, r.unknown_5c);
	SWAP_PACKED(l.unknown_60, r.unknown_60);
	SWAP_PACKED(l.unknown_64, r.unknown_64);
	SWAP_PACKED(l.pvar_index, r.pvar_index);
	SWAP_PACKED(l.unknown_6c, r.unknown_6c);
	SWAP_PACKED(l.unknown_70, r.unknown_70);
	SWAP_PACKED(l.unknown_74, r.unknown_74);
	SWAP_PACKED(l.unknown_78, r.unknown_78);
	SWAP_PACKED(l.unknown_7c, r.unknown_7c);
	SWAP_PACKED(l.unknown_80, r.unknown_80);
	SWAP_PACKED(l.unknown_84, r.unknown_84);
}

void swap_vec3(glm::vec3& l, vec3f& r) {
	SWAP_PACKED(l, r);
}

void swap_mat4(glm::mat4& l, mat4f& r) {
	SWAP_PACKED(l, r);
}
