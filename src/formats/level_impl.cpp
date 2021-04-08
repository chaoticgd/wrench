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
#include "tfrag.h"

#include "../app.h"

void level::read(stream& src, fs::path path_) {
	path = path_;
	std::optional<simple_wad_stream> _instances_segment;
	
	if(src.size() > 1024 * 1024 * 1024) {
		throw stream_format_error("The file is over 1GB in size.");
	}
	
	read_file_header(src);
	
	// Load the level data into memory. This logic should handle both levels
	// that have been written out by my ISO tool, and levels that have been
	// extracted from the disc with a traditional ISO tool. This doesn't handle
	// the case where the ToC header has been appended to the beginning of the
	// file without patching the base_offset field, and it doesn't handle the
	// case where the ToC header is stored in a seperate file.
	if(file_header.base_offset.sectors != 0 && file_header.base_offset.sectors != info.header_size_sectors) {
		throw stream_format_error("Invalid base offset field in file header!\n");
	}
	_file.emplace();
	_file->buffer.resize(src.size() - file_header.base_offset.sectors * SECTOR_SIZE);
	src.seek(file_header.base_offset.sectors * SECTOR_SIZE);
	src.read_n(_file->buffer.data(), _file->buffer.size());

	switch(file_header.type) {
		case level_type::RAC23:
		case level_type::RAC2_68: {
			auto header = _file->read<level_primary_header_rac23>(file_header.primary_header.offset.bytes());
			swap_primary_header_rac23(_primary_header, header);
			break;
		}
		case level_type::RAC4: {
			auto header = _file->read<level_primary_header_rac4>(file_header.primary_header.offset.bytes());
			swap_primary_header_rac4(_primary_header, header);
			break;
		}
	}

	code_segment.header = _file->read<level_code_segment_header>
		(file_header.primary_header.offset.bytes() + _primary_header.code_segment.offset);
	code_segment.bytes.resize(_primary_header.code_segment.size - sizeof(level_code_segment_header));
	_file->read_v(code_segment.bytes);

	_world_segment.emplace(&(*_file), file_header.world_segment.offset.bytes());
	_world_segment->name = "World Segment";
	
	world.backing = &(*_world_segment);
	switch(file_header.type) {
		case level_type::RAC23:
		case level_type::RAC2_68:
			world.read_rac23();
			break;
		case level_type::RAC4:
			size_t instances_wad_offset =
				file_header.primary_header.offset.bytes() +
				_primary_header.instances_wad.offset;
			_instances_segment.emplace(&(*_file), instances_wad_offset);
			_instances_segment->name = "Instances Segment";

			world.read_rac4(&(*_instances_segment));
			break;
	}
	
	size_t asset_wad_offset =
		file_header.primary_header.offset.bytes() +
		_primary_header.asset_wad.offset;
	_asset_segment.emplace(&(*_file), asset_wad_offset);
	_asset_segment->name = "Asset Segment";
	
	uint32_t asset_offset = file_header.primary_header.offset.bytes() + _primary_header.asset_header.offset;
	auto asset_header = _file->read<level_asset_header>(asset_offset);
	
	read_moby_models(asset_offset, asset_header);
	read_shrub_models(asset_offset, asset_header);
	read_textures(asset_offset, asset_header);
	
	read_tfrags();
	read_tcol(asset_offset, asset_header);
	
	if(file_header.type == level_type::RAC4) {
		return;
	}
	
	read_hud_banks(&src);
	read_loading_screen_textures(&src);
}

void level::read_file_header(stream& file) {
	file.seek(0);
	file_header.type = (level_type) file.peek<uint32_t>();
	auto info_iter = LEVEL_FILE_TYPES.find((uint32_t) file_header.type);
	assert(info_iter != LEVEL_FILE_TYPES.end());
	info = info_iter->second;
	switch(file_header.type) {
		case level_type::RAC23: {
			auto header = file.read<level_file_header_rac23>();
			swap_level_file_header_rac23(file_header, header);
			break;
		}
		case level_type::RAC2_68: {
			auto header = file.read<level_file_header_rac2_68>();
			swap_level_file_header_rac2_68(file_header, header);
			break;
		}
		case level_type::RAC4: {
			auto header = file.read<level_file_header_rac4>();
			swap_level_file_header_rac4(file_header, header);
			break;
		}
	}
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
	_file->seek(mdl_base);
	
	for(std::size_t i = 0; i < asset_header.moby_model_count; i++) {
		auto entry = _file->read<level_moby_model_entry>(mdl_base + sizeof(level_moby_model_entry) * i);
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
	_file->seek(mdl_base);

	for (std::size_t i = 0; i < asset_header.shrub_model_count; i++) {
		auto entry = _file->read<level_shrub_model_entry>(mdl_base + sizeof(level_shrub_model_entry) * i);
		if (entry.offset_in_asset_wad == 0) {
			continue;
		}

		shrub_model& model = shrub_models.emplace_back(&(*_asset_segment), entry.offset_in_asset_wad, 0);
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

void level::read_textures(std::size_t asset_offset, level_asset_header asset_header) {
	std::size_t small_texture_base =
		file_header.primary_header.offset.bytes() + _primary_header.small_textures.offset;
	std::size_t last_palette_offset = 0;
	
	std::vector<level_mipmap_descriptor> mipmap_descriptors(asset_header.mipmap_count);
	_file->seek(asset_offset + asset_header.mipmap_offset);
	_file->read_v(mipmap_descriptors);
	for(level_mipmap_descriptor& descriptor : mipmap_descriptors) {
		auto abs_offset = small_texture_base + descriptor.offset_1;
		if(descriptor.width == 0) {
			last_palette_offset = abs_offset;
			continue;
		}
		vec2i size { descriptor.width, descriptor.height };
		texture tex = create_texture_from_streams(size, &(*_file), abs_offset, &(*_file), last_palette_offset);
		mipmap_textures.emplace_back(std::move(tex));
	}
	
	auto load_texture_table = [&](stream& backing, std::size_t offset, std::size_t count) {
		std::vector<texture> textures;
		std::vector<level_texture_descriptor> texture_descriptors(count);
		backing.seek(asset_offset + offset);
		backing.read_v(texture_descriptors);
		for(level_texture_descriptor& descriptor : texture_descriptors) {
			vec2i size { descriptor.width, descriptor.height };
			auto ptr = asset_header.tex_data_in_asset_wad + descriptor.ptr;
			auto palette = small_texture_base + descriptor.palette * 0x100;
			texture tex;
			if (file_header.type == level_type::RAC4) {
				tex = create_texture_from_streams_rac4(size, &(*_asset_segment), ptr, &(*_file), palette);
			}
			else {
				tex = create_texture_from_streams(size, &(*_asset_segment), ptr, &(*_file), palette);
			}
			textures.emplace_back(std::move(tex));
		}
		return textures;
	};

	tfrag_textures = load_texture_table(*_file, asset_header.tfrag_texture_offset, asset_header.tfrag_texture_count);
	moby_textures = load_texture_table(*_file, asset_header.moby_texture_offset, asset_header.moby_texture_count);
	tie_textures = load_texture_table(*_file, asset_header.tie_texture_offset, asset_header.tie_texture_count);
	shrub_textures = load_texture_table(*_file, asset_header.shrub_texture_offset, asset_header.shrub_texture_count);
	sprite_textures = load_texture_table(*_file, asset_header.sprite_texture_offset, asset_header.sprite_texture_count);
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

void level::read_hud_banks(stream* file) {
	//const auto read_hud_bank = [&](int index, uint32_t relative_offset, uint32_t size) {
	//	if(size > 0x10) {
	//		uint32_t absolute_offset = _file_header.primary_header.offset.bytes() + relative_offset;
	//		stream* bank = iso->get_decompressed(_file_header.base_offset.bytes() + absolute_offset);
	//		bank->name = "HUD Bank " + std::to_string(index);
	//	}
	//};
	//
	////auto hud_header = _file->read<level_hud_header>(_file_header.primary_header_offset + _primary_header.hud_header_offset);
	//read_hud_bank(0, _primary_header.hud_bank_0.offset, _primary_header.hud_bank_0.size);
	//read_hud_bank(1, _primary_header.hud_bank_1.offset, _primary_header.hud_bank_1.size);
	//read_hud_bank(2, _primary_header.hud_bank_2.offset, _primary_header.hud_bank_2.size);
	//read_hud_bank(3, _primary_header.hud_bank_3.offset, _primary_header.hud_bank_3.size);
	//read_hud_bank(4, _primary_header.hud_bank_4.offset, _primary_header.hud_bank_4.size);
}

void level::read_loading_screen_textures(stream* file) {
	//size_t primary_header_offset = _file_header.base_offset.bytes() + _file_header.primary_header.offset.bytes();
	//size_t load_wad_offset = primary_header_offset + _primary_header.loading_screen_textures.offset;
	//if(load_wad_offset > iso->size()) {
	//	fprintf(stderr, "warning: Failed to read loading screen textures (seek pos > iso size).\n");
	//	return;
	//}
	//
	//char wad_magic[3];
	//iso->seek(load_wad_offset);
	//iso->read_n(wad_magic, 3);
	//if(std::memcmp(wad_magic, "WAD", 3) == 0) {
	//	stream* textures = iso->get_decompressed(load_wad_offset);
	//	loading_screen_textures = read_pif_list(textures, 0);
	//} else {
	//	fprintf(stderr, "warning: Failed to read loading screen textures (missing magic bytes).\n");
	//}
}

void level::write(array_stream& dest) {
	level_file_header header;
	defer([&]() {
		dest.seek(0);
		switch(file_header.type) {
			case level_type::RAC23: {
				level_file_header_rac23 ondisc {0};
				swap_level_file_header_rac23(header, ondisc);
				dest.write(ondisc);
				break;
			}
			case level_type::RAC2_68: {
				level_file_header_rac2_68 ondisc {0};
				swap_level_file_header_rac2_68(header, ondisc);
				dest.write(ondisc);
				break;
			}
			case level_type::RAC4: {
				level_file_header_rac4 ondisc {0};
				swap_level_file_header_rac4(header, ondisc);
				dest.write(ondisc);
				break;
			}
		}
	});
	dest.seek(SECTOR_SIZE); // Leave some space for the header.

	auto bytes_to_range = [&](size_t begin, size_t end) {
		assert(begin % SECTOR_SIZE == 0);
		if(end % SECTOR_SIZE != 0) {
			end += SECTOR_SIZE - (end % SECTOR_SIZE);
		}
		sector_range result {
			(uint32_t) (begin / SECTOR_SIZE),
			(uint32_t) ((end - begin) / SECTOR_SIZE)
		};
		// If this ever asserts then hello from the distant past.
		assert(result.offset.sectors == begin / SECTOR_SIZE);
		assert(result.size.sectors == (end - begin) / SECTOR_SIZE);
		return result;
	};
	
	auto copy_lump = [&](sector_range range) {
		if(range.size.sectors == 0) {
			return sector_range {{0}, {0}};
		}
		dest.pad(SECTOR_SIZE, 0);
		size_t begin_offset = dest.tell();
		_file->seek(range.offset.bytes());
		stream::copy_n(dest, *_file, range.size.bytes());
		size_t end_offset = dest.tell();
		return bytes_to_range(begin_offset, end_offset);
	};
	
	header.base_offset = sector32{0};
	header.level_number = file_header.level_number;
	header.unknown_c = file_header.unknown_c;
	
	header.sound_bank_1 = copy_lump(file_header.sound_bank_1);
	header.primary_header = copy_lump(file_header.primary_header);
	
	dest.pad(SECTOR_SIZE, 0);
	size_t beginning_of_the_world = dest.tell();
	array_stream world_dest;
	world.write_rac23(world_dest);
	compress_wad(dest, world_dest, config::get().compression_threads);
	size_t end_of_the_world = dest.tell(); // mwuhuhuhu!
	header.world_segment = bytes_to_range(beginning_of_the_world, end_of_the_world);
	
	header.unknown_28 = copy_lump(file_header.unknown_28);
	header.unknown_30 = copy_lump(file_header.unknown_30);
	header.unknown_38 = copy_lump(file_header.unknown_38);
	header.unknown_40 = copy_lump(file_header.unknown_40);
	header.sound_bank_2 = copy_lump(file_header.sound_bank_2);
	header.sound_bank_3 = copy_lump(file_header.sound_bank_3);
	header.sound_bank_4 = copy_lump(file_header.sound_bank_4);
}

stream* level::moby_stream() {
	return &(*_world_segment);
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

void swap_level_file_header_rac23(level_file_header& l, level_file_header_rac23& r) {
	l.type = level_type::RAC23;
	r.magic = (uint32_t) level_type::RAC23;
	SWAP_PACKED(l.base_offset, r.base_offset);
	SWAP_PACKED(l.level_number, r.level_number);
	SWAP_PACKED(l.unknown_c, r.unknown_c);
	SWAP_PACKED(l.primary_header, r.primary_header);
	SWAP_PACKED(l.sound_bank_1, r.sound_bank_1);
	SWAP_PACKED(l.world_segment, r.world_segment);
	SWAP_PACKED(l.unknown_28, r.unknown_28);
	SWAP_PACKED(l.unknown_30, r.unknown_30);
	SWAP_PACKED(l.unknown_38, r.unknown_38);
	SWAP_PACKED(l.unknown_40, r.unknown_40);
	SWAP_PACKED(l.sound_bank_2, r.sound_bank_2);
	SWAP_PACKED(l.sound_bank_3, r.sound_bank_3);
	SWAP_PACKED(l.sound_bank_4, r.sound_bank_4);
}

void swap_level_file_header_rac2_68(level_file_header& l, level_file_header_rac2_68& r) {
	l.type = level_type::RAC2_68;
	r.magic = (uint32_t) level_type::RAC2_68;
	SWAP_PACKED(l.base_offset, r.base_offset);
	SWAP_PACKED(l.level_number, r.level_number);
	SWAP_PACKED(l.primary_header, r.primary_header);
	SWAP_PACKED(l.world_segment, r.world_segment_1);
}

void swap_level_file_header_rac4(level_file_header& l, level_file_header_rac4& r) {
	l.type = level_type::RAC4;
	r.magic = (uint32_t) level_type::RAC4;
	SWAP_PACKED(l.base_offset, r.base_offset);
	SWAP_PACKED(l.level_number, r.level_number);
	SWAP_PACKED(l.primary_header, r.primary_header);
	SWAP_PACKED(l.world_segment, r.world_segment);
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
