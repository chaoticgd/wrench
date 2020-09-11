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
	
	read_world_segment();
	
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
	write_world_segment();
}

void level::read_world_segment() {
	world_header header = _world_segment->read<world_header>(0);
	
	const auto read_table = [&]<typename T_1, typename T_2 = char, typename T_3 = char>(
			uint32_t offset,
			std::vector<T_1>* first,
			std::vector<T_2>* second = nullptr,
			std::vector<T_3>* third = nullptr,
			bool align = false) {
		auto table = _world_segment->read<world_object_table>(offset);
		if(first != nullptr) {
			*first = _world_segment->read_multiple<T_1>(table.count_1);
		}
		if(align) _world_segment->align(0x10, 0);
		if(second != nullptr) {
			*second = _world_segment->read_multiple<T_2>(table.count_2);
		}
		if(align) _world_segment->align(0x10, 0);
		if(third != nullptr) {
			*third = _world_segment->read_multiple<T_3>(table.count_3);
		}
	};
	
	read_table(header.unknown_8c, &thing_8cs);
	
	properties = _world_segment->read<world_properties>(header.properties);
	
	// Strings
	const auto read_language = [&](uint32_t offset) {
		std::vector<game_string> language;
	
		auto table = _world_segment->read<world_string_table_header>(offset);
		std::vector<world_string_table_entry> entries(table.num_strings);
		_world_segment->read_v(entries);
		
		for(world_string_table_entry& entry : entries) {
			_world_segment->seek(offset + entry.string.value);
			language.push_back({ entry.id, entry.secondary_id, _world_segment->read_string() });
		}
		
		return language;
	};
	game_strings[0] = read_language(header.english_strings);
	thing_14 = _world_segment->read<world_thing_14>(header.unknown_14);
	game_strings[1] = read_language(header.french_strings);
	game_strings[2] = read_language(header.german_strings);
	game_strings[3] = read_language(header.spanish_strings);
	game_strings[4] = read_language(header.italian_strings);
	game_strings[5] = read_language(header.unused1_strings);
	game_strings[6] = read_language(header.unused2_strings);
	
	read_table(header.directional_lights, &directional_lights);
	read_table(header.unknown_84, &thing_84s);
	read_table(header.unknown_8, &thing_8s);
	read_table(header.unknown_c, &thing_cs);
	
	const auto read_u32_list = [&](uint32_t offset) {
		auto count = _world_segment->read<uint32_t>(offset);
		return _world_segment->read_multiple<uint32_t>(count);
	};
	thing_48s = read_u32_list(header.unknown_48);
	
	auto read_entity_table = [&]<typename T_in_mem, typename T_on_disc>(
			uint32_t offset, std::function<void(T_in_mem&, T_on_disc&)> swap_ent) {
		auto table = _world_segment->read<world_object_table>(offset);
		std::vector<T_on_disc> src = _world_segment->read_multiple<T_on_disc>(table.count_1);
		std::vector<T_in_mem> dest;
		for(T_on_disc& on_disc : src) {
			T_in_mem& ent = dest.emplace_back();
			ent.id = { _next_entity_id++ };
			ent.selected = false;
			swap_ent(ent, on_disc);
		}
		return dest;
	};
	mobies = read_entity_table.operator()<moby_entity, world_moby>(header.mobies, swap_moby);
	max_moby_count = mobies.size() + _world_segment->read<uint32_t>(header.mobies + sizeof(uint32_t));
	
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
	
	const auto read_terminated_array = [&]<typename T>(std::vector<T>& dest, uint32_t offset) {
		_world_segment->seek(offset);
		for(;;) {
			T thing = _world_segment->read<T>();
			if(*(int32_t*) &thing > -1) {
				dest.push_back(thing);
			} else {
				break;
			}
		}
	};
	read_terminated_array(thing_58s, header.unknown_58);
	read_terminated_array(thing_64s, header.unknown_64);
	read_table(header.unknown_50, &thing_50_1s, &thing_50_2s);
	read_table(header.unknown_54, &thing_54_1s, &thing_54_2s);
	thing_30s = read_u32_list(header.unknown_30);
	ties = read_entity_table.operator()<tie_entity, world_tie>(header.ties, swap_tie);
	
	_world_segment->seek(header.unknown_94);
	for(;;) {
		world_thing_94 thing_94 = _world_segment->read<world_thing_94>();
		if(thing_94.index > -1) {
			thing_94s.push_back(_world_segment->read_multiple<uint8_t>(thing_94.size << 1));
		} else {
			break;
		}
	}
	
	read_table(header.unknown_38, &thing_38_1s, &thing_38_2s, (std::vector<char>*) nullptr, true);
	thing_3cs = read_u32_list(header.unknown_3c);
	shrubs = read_entity_table.operator()<shrub_entity, world_shrub>(header.shrubs, swap_shrub);
	read_table(header.unknown_44, &thing_44_1s, &thing_44_2s, (std::vector<char>*) nullptr, true);
	
	const auto read_vertex_list = [&](
			uint32_t table_offset,
			uint32_t table_count,
			uint32_t data_offset) {
		std::vector<std::vector<glm::vec4>> result;
		std::vector<uint32_t> offsets = _world_segment->read_multiple<uint32_t>(table_count);
		for(uint32_t offset : offsets) {
			auto vertex_header = _world_segment->read<world_vertex_header>(data_offset + offset);
			std::vector<glm::vec4>& vertices = result.emplace_back();
			for(size_t i = 0; i < vertex_header.vertex_count; i++) {
				glm::vec4 vertex;
				vertex.x = _world_segment->read<float>();
				vertex.y = _world_segment->read<float>();
				vertex.z = _world_segment->read<float>();
				vertex.w = _world_segment->read<float>();
				vertices.push_back(vertex);
			}
		}
		return result;
	};
	
	auto spline_table = _world_segment->read<world_spline_table>(header.splines);
	auto spline_vertex_list = read_vertex_list(
		_world_segment->tell(),
		spline_table.spline_count,
		header.splines + spline_table.data_offset);
	for(std::vector<glm::vec4>& vertices : spline_vertex_list) {
		spline_entity& spline = splines.emplace_back();
		spline.id = { _next_entity_id++ };
		spline.selected = false;
		spline.vertices = vertices;
	}
	
	for(spline_entity& spline : splines) {
		spline.id = { _next_entity_id++ };
		spline.selected = false;
	}
	
	triggers = read_entity_table.operator()<trigger_entity, world_trigger>(header.triggers, swap_trigger);
	
	read_table(header.unknown_6c, &thing_6cs);
	read_table(header.unknown_70, &thing_70s);
	read_table(header.unknown_74, &thing_74s);
	
	uint32_t thing_88_size = _world_segment->read<uint32_t>(header.unknown_88);
	thing_88 = _world_segment->read_multiple<uint8_t>(thing_88_size);
	
	auto thing_80_table = _world_segment->read<world_object_table>(header.unknown_80);
	thing_80_1 = _world_segment->read_multiple<uint8_t>(0x800);
	thing_80_2 = _world_segment->read_multiple<uint8_t>(thing_80_table.count_1 * 0x10);
	
	world_thing_7c_header thing_7c_header = _world_segment->read<world_thing_7c_header>(header.unknown_7c);
	thing_7c_1s = _world_segment->read_multiple<world_thing_7c_1>(thing_7c_header.count);
	auto thing_7c_vertex_list = read_vertex_list(
		_world_segment->tell(),
		thing_7c_header.count,
		header.unknown_7c + thing_7c_header.part_2_data_offset);
	for(std::vector<glm::vec4>& vertices : thing_7c_vertex_list) {
		thing_7c_2& thing = thing_7c_2s.emplace_back();
		thing.vertices = vertices;
	}
	
	auto thing_98_header = _world_segment->read<world_thing_98_header>(header.unknown_98);
	thing_98_1s = _world_segment->read_multiple<world_thing_98>(thing_98_header.part_1_count);
	_world_segment->seek(header.unknown_98 + thing_98_header.part_2_begin - 0x10);
	thing_98_2s = _world_segment->read_multiple<uint32_t>(
		(thing_98_header.part_2_end - thing_98_header.part_2_begin) / sizeof(uint32_t) + 5);
	thing_98_header_c = thing_98_header.unknown_c;
	
	if(header.unknown_90 != 0) {
		auto thing_90_header = _world_segment->read<world_thing_90_header>(header.unknown_90);
		thing_90_1s = _world_segment->read_multiple<world_thing_90>(thing_90_header.count_1);
		thing_90_2s = _world_segment->read_multiple<world_thing_90>(thing_90_header.count_2);
		thing_90_3s = _world_segment->read_multiple<world_thing_90>(thing_90_header.count_3);
	}
}

static const std::vector<char> EMPTY_VECTOR;

void level::write_world_segment() {
	world_header header;
	defer([&] {
		_world_segment->write(0, header);
	});
	_world_segment->seek(sizeof(world_header));
	
	const auto write_table = [&]<typename T_1, typename T_2 = char, typename T_3 = char>(
			const std::vector<T_1>& first,
			const std::vector<T_2>& second = EMPTY_VECTOR,
			const std::vector<T_3>& third = EMPTY_VECTOR,
			bool align = false) {
		_world_segment->pad(0x10, 0);
		size_t base_pos = _world_segment->tell();
		world_object_table table;
		table.count_1 = first.size();
		table.count_2 = second.size();
		table.count_3 = third.size();
		table.pad = 0;
		_world_segment->write(table);
		_world_segment->write_v(first);
		if(align) _world_segment->pad(0x10, 0);
		_world_segment->write_v(second);
		if(align) _world_segment->pad(0x10, 0);
		_world_segment->write_v(third);
		return base_pos;
	};
	
	header.unknown_8c = write_table(thing_8cs);
	
	_world_segment->pad(0x10, 0);
	header.properties = _world_segment->tell();
	_world_segment->write(properties);
	
	const auto write_language = [&](std::vector<game_string>& strings) {
		_world_segment->pad(0x10, 0);
		size_t base_pos = _world_segment->tell();
		_world_segment->seek(base_pos + sizeof(world_string_table_header));
		
		size_t data_pos = sizeof(world_string_table_header) +
			strings.size() * sizeof(world_string_table_entry);
		for(game_string& str : strings) {
			world_string_table_entry entry;
			entry.string = data_pos;
			entry.id = str.id;
			entry.secondary_id = str.secondary_id;
			entry.padding = 0;
			_world_segment->write(entry);
			data_pos += str.str.size() + 1;
			while(data_pos % 4 != 0) data_pos++;
		}
		
		for(game_string& str : strings) {
			_world_segment->pad(0x4, 0);
			_world_segment->write_n(str.str.c_str(), str.str.size() + 1);
		}
		_world_segment->pad(0x10, 0);
		
		world_string_table_header string_table;
		string_table.num_strings = strings.size();
		string_table.size = data_pos;
		_world_segment->write(base_pos, string_table);
		
		_world_segment->seek(base_pos + data_pos);
		
		return base_pos;
	};
	header.english_strings = write_language(game_strings[0]);
	_world_segment->pad(0x10, 0);
	header.unknown_14 = _world_segment->tell();
	_world_segment->write(thing_14);
	header.french_strings = write_language(game_strings[1]);
	header.german_strings = write_language(game_strings[2]);
	header.spanish_strings = write_language(game_strings[3]);
	header.italian_strings = write_language(game_strings[4]);
	header.unused1_strings = write_language(game_strings[5]);
	header.unused2_strings = write_language(game_strings[6]);
	
	header.directional_lights = write_table(directional_lights);
	header.unknown_84 = write_table(thing_84s);
	header.unknown_8 = write_table(thing_8s);
	header.unknown_c = write_table(thing_cs);
	
	const auto write_u32_list = [&](const std::vector<uint32_t>& list) {
		_world_segment->pad(0x10, 0);
		size_t base_pos = _world_segment->tell();
		_world_segment->write<uint32_t>(list.size());
		_world_segment->write_v(list);
		return base_pos;
	};
	header.unknown_48 = write_u32_list(thing_48s);
	
	const auto write_entity_table = [&]<typename T_in_mem, typename T_on_disc>(
			std::vector<T_in_mem> ents, std::function<void(T_in_mem&, T_on_disc&)> swap_ent) {
		std::vector<T_on_disc> dest;
		for(T_in_mem& ent : ents) {
			T_on_disc& on_disc = dest.emplace_back();
			ent.id = { _next_entity_id++ };
			ent.selected = false;
			swap_ent(ent, on_disc);
		}
		return write_table(dest);
	};
	header.mobies = write_entity_table.operator()<moby_entity, world_moby>(mobies, swap_moby);
	size_t pos_after_mobies = _world_segment->tell();
	_world_segment->write<uint32_t>(header.mobies + sizeof(uint32_t), max_moby_count - mobies.size());
	_world_segment->seek(pos_after_mobies);
	
	_world_segment->pad(0x10, 0);
	header.pvar_table = _world_segment->tell();
	size_t next_pvar_offset = 0;
	header.pvar_data = header.pvar_table + pvars.size() * sizeof(pvar_table_entry);
	while(header.pvar_data % 0x10 != 0) header.pvar_data++;
	for(std::vector<uint8_t>& pvar : pvars) {
		pvar_table_entry entry;
		entry.offset = next_pvar_offset;
		entry.size = pvar.size();
		_world_segment->write(entry);
		size_t next_pos = _world_segment->tell();
		
		_world_segment->seek(header.pvar_data + next_pvar_offset);
		_world_segment->write_v(pvar);
		
		next_pvar_offset += pvar.size();
		_world_segment->seek(next_pos);
	}
	_world_segment->pad(0x10, 0);
	_world_segment->seek(header.pvar_data + next_pvar_offset); // Skip past the pvar data section.
	
	const auto write_terminated_array = [&]<typename T>(const std::vector<T>& things) {
		_world_segment->pad(0x10, 0);
		size_t result = _world_segment->tell();
		_world_segment->write_v(things);
		_world_segment->write<uint64_t>(0xffffffffffffffff); // terminator
		return result;
	};
	header.unknown_58 = write_terminated_array(thing_58s);
	header.unknown_64 = write_terminated_array(thing_64s);
	header.unknown_50 = write_table(thing_50_1s, thing_50_2s);
	header.unknown_54 = write_table(thing_54_1s, thing_54_2s);
	header.unknown_30 = write_u32_list(thing_30s);
	header.ties = write_entity_table.operator()<tie_entity, world_tie>(ties, swap_tie);
	
	_world_segment->pad(0x10, 0);
	header.unknown_94 = _world_segment->tell();
	for(size_t i = 0; i < thing_94s.size(); i++) {
		world_thing_94 thing_94_header;
		thing_94_header.index = i;
		thing_94_header.size = thing_94s[i].size() >> 1;
		_world_segment->write(thing_94_header);
		_world_segment->write_v(thing_94s[i]);
	}
	_world_segment->write<uint16_t>(0xffff);
	
	header.unknown_38 = write_table(thing_38_1s, thing_38_2s, EMPTY_VECTOR, true);
	header.unknown_3c = write_u32_list(thing_3cs);
	header.shrubs = write_entity_table.operator()<shrub_entity, world_shrub>(shrubs, swap_shrub);
	header.unknown_44 = write_table(thing_44_1s, thing_44_2s, EMPTY_VECTOR, true);
	
	const auto write_vertex_list = [&](std::vector<std::vector<glm::vec4>> list) {
		size_t base_pos = _world_segment->tell();
		
		_world_segment->seek(_world_segment->tell() + list.size() * sizeof(uint32_t));
		_world_segment->pad(0x10, 0);
		size_t data_pos = _world_segment->tell();
		
		std::vector<uint32_t> offsets;
		for(std::vector<glm::vec4> vertices : list) {
			_world_segment->pad(0x10, 0);
			offsets.push_back(_world_segment->tell() - data_pos);
			world_vertex_header vertex_header;
			vertex_header.vertex_count = vertices.size();
			vertex_header.pad[0] = 0;
			vertex_header.pad[1] = 0;
			vertex_header.pad[2] = 0;
			_world_segment->write(vertex_header);
			for(glm::vec4& vertex : vertices) {
				_world_segment->write(vertex.x);
				_world_segment->write(vertex.y);
				_world_segment->write(vertex.z);
				_world_segment->write(vertex.w);
			}
		}
		
		size_t end_pos = _world_segment->tell();
		_world_segment->seek(base_pos);
		_world_segment->write_v(offsets);
		_world_segment->seek(end_pos);
		
		return data_pos;
	};
	
	_world_segment->pad(0x10, 0);
	header.splines = _world_segment->tell();
	
	world_spline_table spline_table;
	defer([&]() {
		_world_segment->write(header.splines, spline_table);
	});
	_world_segment->seek(_world_segment->tell() + sizeof(world_spline_table));
	
	std::vector<std::vector<glm::vec4>> spline_vertices;
	for(spline_entity& spline : splines) {
		spline_vertices.push_back(spline.vertices);
	} 
	spline_table.spline_count = splines.size();
	spline_table.data_offset = write_vertex_list(spline_vertices) - header.splines;
	spline_table.data_size = _world_segment->tell() - header.splines - spline_table.data_offset;
	spline_table.pad = 0;
	
	header.triggers = write_entity_table.operator()<trigger_entity, world_trigger>(triggers, swap_trigger);
	
	header.unknown_6c = write_table(thing_6cs);
	header.unknown_70 = write_table(thing_70s);
	header.unknown_74 = write_table(thing_74s); // Not sure if this is right.
	
	_world_segment->pad(0x10, 0);
	header.unknown_88 = _world_segment->tell();
	_world_segment->write<uint32_t>(thing_88.size());
	_world_segment->write_v(thing_88);
	
	_world_segment->pad(0x10, 0);
	header.unknown_80 = _world_segment->tell();
	world_object_table thing_80_table;
	thing_80_table.count_1 = thing_80_2.size() / 0x10;
	thing_80_table.count_2 = 0;
	thing_80_table.count_3 = 0;
	thing_80_table.pad = 0;
	_world_segment->write(thing_80_table);
	_world_segment->write_v(thing_80_1);
	_world_segment->write_v(thing_80_2);
	
	_world_segment->pad(0x10, 0);
	header.unknown_7c = _world_segment->tell();
	
	world_thing_7c_header thing_7c_header;
	defer([&]() {
		_world_segment->write(header.unknown_7c, thing_7c_header);
	});
	_world_segment->seek(_world_segment->tell() + sizeof(world_thing_7c_header));
	
	_world_segment->write_v(thing_7c_1s);
	_world_segment->pad(0x10, 0);
	
	std::vector<std::vector<glm::vec4>> thing_7c_vertices;
	for(thing_7c_2& thing : thing_7c_2s) {
		thing_7c_vertices.push_back(thing.vertices);
	}
	thing_7c_header.count = thing_7c_1s.size();
	thing_7c_header.part_2_data_offset = write_vertex_list(thing_7c_vertices) - header.unknown_7c;
	thing_7c_header.part_2_data_size = _world_segment->tell() -
		thing_7c_header.part_2_data_offset - header.unknown_7c;
	thing_7c_header.pad = 0;
	
	_world_segment->pad(0x10, 0);
	header.unknown_98 = _world_segment->tell();
	world_thing_98_header thing_98_header;
	thing_98_header.part_2_begin = sizeof(world_thing_98_header) +
		thing_98_1s.size() * sizeof(world_thing_98) + 0x10;
	thing_98_header.part_1_count = thing_98_1s.size();
	thing_98_header.part_2_end = thing_98_header.part_2_begin - 0x10 +
		(thing_98_2s.size() - 1) * sizeof(uint32_t);
	thing_98_header.unknown_c = thing_98_header_c;
	_world_segment->write(thing_98_header);
	_world_segment->write_v(thing_98_1s);
	_world_segment->write_v(thing_98_2s);
	
	if(thing_90_1s.size() > 0 || thing_90_2s.size() > 0 || thing_90_3s.size() > 0) {
		_world_segment->pad(0x100, 0);
		header.unknown_90 = write_table(thing_90_1s, thing_90_2s, thing_90_3s);
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

void level::read_loading_screen_textures(iso_stream* iso) {
	std::size_t primary_header_offset = _file_header.base_offset + _file_header.primary_header_offset;
	stream* textures = iso->get_decompressed(primary_header_offset + _primary_header.loading_screen_textures_offset);
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

void swap_tie(tie_entity& l, world_tie& r) {
	// matrix_entity
	SWAP_PACKED(l.local_to_world, r.local_to_world);
	l.local_to_world[3][3] = 1.f;
	r.local_to_world.m44 = 0.01f;
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
	SWAP_PACKED(l.local_to_world, r.local_to_world);
	l.local_to_world[3][3] = 1.f;
	r.local_to_world.m44 = 0.01f;
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
	SWAP_PACKED(l.position, r.position);
	SWAP_PACKED(l.rotation, r.rotation);
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

void swap_trigger(trigger_entity& l, world_trigger& r)
{
	SWAP_PACKED(l.local_to_world, r.mat1);
	SWAP_PACKED(l.matrix_reloaded, r.mat2);
}
