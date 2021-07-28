/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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
#include "tfrag.h"
#include "../../lz/compression.h"

#include "../app.h"

void level::open(fs::path json_path, Json& json) {
	fs::path level_dir = json_path.parent_path();
	path = json_path;
	from_json(_gameplay, Json::parse(read_file(level_dir/std::string(json["gameplay"]))));
	fs::path primary_path = level_dir/std::string(json["primary"]);
	read_primary(primary_path, Game::RAC2);
}

void level::save() {
	Json data_json = to_json(_gameplay);
	
	Json gameplay_json;
	gameplay_json["metadata"] = get_file_metadata("gameplay", "Wrench Level Editor");
	for(auto& item : data_json.items()) {
		gameplay_json[item.key()] = std::move(item.value());
	}
	fs::path dest_dir = path.parent_path();
	Json level_json = Json::parse(read_file(path));
	std::string gameplay_path = level_json["gameplay"];
	write_file(dest_dir, fs::path(gameplay_path), gameplay_json.dump(1, '\t'));
}

void level::read_primary(fs::path bin_path, Game game) {
	_primary.emplace(bin_path.string());
	
	switch(game) {
		case Game::RAC2:
		case Game::RAC3: {
			auto header = _primary->read<level_primary_header_rac23>(0);
			swap_primary_header_rac23(_primary_header, header);
			break;
		}
		case Game::DL: {
			auto header = _primary->read<level_primary_header_rac4>(0);
			swap_primary_header_rac4(_primary_header, header);
			break;
		}
	}

	code_segment.header = _primary->read<level_code_segment_header>
		(_primary_header.code_segment.offset);
	code_segment.bytes.resize(_primary_header.code_segment.size - sizeof(level_code_segment_header));
	_primary->read_v(code_segment.bytes);
	
	uint32_t asset_wad_offset = _primary_header.asset_wad.offset;
	_asset_segment.emplace(&(*_primary), asset_wad_offset);
	_asset_segment->name = "Asset Segment";
	
	uint32_t asset_offset = _primary_header.asset_header.offset;
	auto asset_header = _primary->read<level_asset_header>(_primary_header.asset_header.offset);
	read_moby_models(asset_offset, asset_header);
	read_shrub_models(asset_offset, asset_header);
	read_textures(asset_offset, asset_header, game == Game::DL);
	read_tfrags();
	read_tcol(asset_offset, asset_header);
}

void level::read_moby_models(std::size_t asset_offset, level_asset_header asset_header) {
	uint32_t mdl_base = asset_offset + asset_header.moby_model_offset;
	_primary->seek(mdl_base);
	
	for(std::size_t i = 0; i < asset_header.moby_model_count; i++) {
		auto entry = _primary->read<level_moby_model_entry>(mdl_base + sizeof(level_moby_model_entry) * i);
		if(entry.offset_in_asset_wad == 0) {
			continue;
		}
		
		auto model_header = _asset_segment->read<moby_model_level_header>(entry.offset_in_asset_wad);
		uint32_t rel_offset = model_header.rel_offset;
		uint32_t abs_offset = entry.offset_in_asset_wad;
		if(rel_offset == 0) {
			continue;
		}
		moby_model& model = moby_models.emplace_back(&(*_asset_segment), abs_offset, 0, moby_model_header_type::LEVEL);
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

void level::read_shrub_models(std::size_t asset_offset, level_asset_header asset_header) {
	uint32_t mdl_base = asset_offset + asset_header.shrub_model_offset;
	_primary->seek(mdl_base);

	for (std::size_t i = 0; i < asset_header.shrub_model_count; i++) {
		auto entry = _primary->read<level_shrub_model_entry>(mdl_base + sizeof(level_shrub_model_entry) * i);
		if (entry.offset_in_asset_wad == 0) {
			continue;
		}

		auto abs_offset = entry.offset_in_asset_wad;
		shrub_model& model = shrub_models.emplace_back(&(*_asset_segment), abs_offset, 0);
		model.set_name("class " + std::to_string(entry.o_class));
		model.read();

		for (uint8_t texture : entry.textures) {
			if (texture == 0xff) {
				break;
			}

			model.texture_indices.push_back(texture);
		}

		model.update();
		uint32_t o_class = entry.o_class;
		shrub_class_to_model.emplace(o_class, shrub_models.size() - 1);
	}
}

void level::read_textures(std::size_t asset_offset, level_asset_header asset_header, bool is_deadlocked) {
	std::size_t small_texture_base = _primary_header.small_textures.offset;
	std::size_t last_palette_offset = 0;
	
	std::vector<level_mipmap_descriptor> mipmap_descriptors(asset_header.mipmap_count);
	_primary->seek(asset_offset + asset_header.mipmap_offset);
	_primary->read_v(mipmap_descriptors);
	for(level_mipmap_descriptor& descriptor : mipmap_descriptors) {
		auto abs_offset = small_texture_base + descriptor.offset_1;
		if(descriptor.width == 0) {
			last_palette_offset = abs_offset;
			continue;
		}
		vec2i size { descriptor.width, descriptor.height };
		texture tex = create_texture_from_streams(size, &(*_primary), abs_offset, &(*_primary), last_palette_offset);
		mipmap_textures.emplace_back(std::move(tex));
	}
	
	auto load_texture_table = [&](std::size_t offset, std::size_t count) {
		std::vector<texture> textures;
		std::vector<level_texture_descriptor> texture_descriptors(count);
		_primary->seek(asset_offset + offset);
		_primary->read_v(texture_descriptors);
		for(level_texture_descriptor& descriptor : texture_descriptors) {
			vec2i size { descriptor.width, descriptor.height };
			auto ptr = asset_header.tex_data_in_asset_wad + descriptor.ptr;
			auto palette = small_texture_base + descriptor.palette * 0x100;
			texture tex;
			if (is_deadlocked) {
				tex = create_texture_from_streams_rac4(size, &(*_asset_segment), ptr, &(*_primary), palette);
			}
			else {
				tex = create_texture_from_streams(size, &(*_asset_segment), ptr, &(*_primary), palette);
			}
			textures.emplace_back(std::move(tex));
		}
		return textures;
	};

	tfrag_textures = load_texture_table(asset_header.tfrag_texture_offset, asset_header.tfrag_texture_count);
	moby_textures = load_texture_table(asset_header.moby_texture_offset, asset_header.moby_texture_count);
	tie_textures = load_texture_table(asset_header.tie_texture_offset, asset_header.tie_texture_count);
	shrub_textures = load_texture_table(asset_header.shrub_texture_offset, asset_header.shrub_texture_count);
	sprite_textures = load_texture_table(asset_header.sprite_texture_offset, asset_header.sprite_texture_count);
}

void level::read_tfrags() {
	packed_struct(tfrag_header,
		uint32_t entry_list_offset; //0x00
		uint32_t count; //0x04
		uint32_t unknown_8; //0x08
		uint32_t count2; //0x0c
	// 0x30 padding
	);

	auto tfrag_head = _asset_segment->read<tfrag_header>(0);
	_asset_segment->seek(tfrag_head.entry_list_offset);

	for (std::size_t i = 0; i < tfrag_head.count; i++) {
		auto entry = _asset_segment->read<tfrag_entry>();
		tfrag frag = tfrag(&(*_asset_segment), tfrag_head.entry_list_offset + entry.offset, entry);
		frag.update();
		tfrags.emplace_back(std::move(frag));
	}
}

void level::read_tcol(std::size_t asset_offset, level_asset_header asset_header) {
	baked_collisions.push_back(tcol(&(*_asset_segment), asset_header.collision));

	for (auto& col : baked_collisions) {
		col.update();
	}
}

void level::push_command(std::function<void(level&)> apply, std::function<void(level&)> undo) {
	_history_stack.resize(_history_index++);
	_history_stack.emplace_back(undo_redo_command { apply, undo });
	apply(*this);
}

void level::undo() {
	if(_history_index <= 0) {
		throw command_error("Nothing to undo.");
	}
	_history_stack[_history_index - 1].undo(*this);
	_history_index--;
}

void level::redo() {
	if(_history_index >= _history_stack.size()) {
		throw command_error("Nothing to redo.");
	}
	_history_stack[_history_index].apply(*this);
	_history_index++;
}

void swap_primary_header_rac23(level_primary_header& l, level_primary_header_rac23& r) {
	l.unknown_0 = { 0, 0 };
	SWAP_PACKED(l.code_segment, r.code_segment);
	SWAP_PACKED(l.asset_header, r.asset_header);
	SWAP_PACKED(l.small_textures, r.small_textures);
	SWAP_PACKED(l.hud_header, r.hud_header);
	SWAP_PACKED(l.hud_bank_0, r.hud_bank_0);
	SWAP_PACKED(l.hud_bank_1, r.hud_bank_1);
	SWAP_PACKED(l.hud_bank_2, r.hud_bank_2);
	SWAP_PACKED(l.hud_bank_3, r.hud_bank_3);
	SWAP_PACKED(l.hud_bank_4, r.hud_bank_4);
	SWAP_PACKED(l.asset_wad, r.asset_wad);
	SWAP_PACKED(l.loading_screen_textures, r.loading_screen_textures);
}

void swap_primary_header_rac4(level_primary_header& l, level_primary_header_rac4& r) {
	SWAP_PACKED(l.unknown_0, r.unknown_0);
	SWAP_PACKED(l.code_segment, r.code_segment);
	SWAP_PACKED(l.asset_header, r.asset_header);
	SWAP_PACKED(l.small_textures, r.small_textures);
	SWAP_PACKED(l.hud_header, r.hud_header);
	SWAP_PACKED(l.hud_bank_0, r.hud_bank_0);
	SWAP_PACKED(l.hud_bank_1, r.hud_bank_1);
	SWAP_PACKED(l.hud_bank_2, r.hud_bank_2);
	SWAP_PACKED(l.hud_bank_3, r.hud_bank_3);
	SWAP_PACKED(l.hud_bank_4, r.hud_bank_4);
	SWAP_PACKED(l.asset_wad, r.asset_wad);
	SWAP_PACKED(l.instances_wad, r.instances_wad);
}
