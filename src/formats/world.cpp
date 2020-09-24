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

#include "world.h"

#include "../util.h"

void world_segment::read_rac23() {
	world_header header = backing->read<world_header>(0);
	
	read_table(header.unknown_8c, &thing_8cs);
	
	properties = backing->read<world_properties>(header.properties);
	world_property_thing property_thing;
	do {
		property_thing = backing->read<world_property_thing>();
		property_things.push_back(property_thing);
	} while(property_things.size() < property_thing.count);
	
	game_strings[0] = read_language(header.english_strings);
	thing_14 = backing->read<world_thing_14>(header.unknown_14);
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
	thing_48s = read_u32_list(header.unknown_48);
	mobies = read_entity_table<moby_entity, world_moby>(header.mobies, swap_moby);
	max_moby_count = mobies.size() + backing->read<uint32_t>(header.mobies + sizeof(uint32_t));
	pvars = read_pvars(header.pvar_table, header.pvar_data);
	read_terminated_array(thing_58s, header.unknown_58);
	read_terminated_array(thing_64s, header.unknown_64);
	read_table(header.unknown_50, &thing_50_1s, &thing_50_2s);
	read_table(header.unknown_54, &thing_54_1s, &thing_54_2s);
	thing_30s = read_u32_list(header.unknown_30);
	ties = read_entity_table<tie_entity, world_tie>(header.ties, swap_tie);
	
	backing->seek(header.unknown_94);
	for(;;) {
		world_thing_94 thing_94 = backing->read<world_thing_94>();
		if(thing_94.index > -1) {
			thing_94s.push_back(backing->read_multiple<uint8_t>(thing_94.size << 1));
		} else {
			break;
		}
	}
	
	read_table(header.unknown_38, &thing_38_1s, &thing_38_2s, (std::vector<char>*) nullptr, true);
	thing_3cs = read_u32_list(header.unknown_3c);
	shrubs = read_entity_table<shrub_entity, world_shrub>(header.shrubs, swap_shrub);
	read_table(header.unknown_44, &thing_44_1s, &thing_44_2s, (std::vector<char>*) nullptr, true);
	
	auto spline_table = backing->read<world_spline_table>(header.splines);
	splines = read_splines(
		backing->tell(),
		spline_table.spline_count,
		header.splines + spline_table.data_offset);
	
	triggers = read_entity_table<trigger_entity, world_trigger>(header.triggers, swap_trigger);
	read_table(header.unknown_6c, &thing_6cs);
	read_table(header.unknown_70, &thing_70s);
	read_table(header.unknown_74, &thing_74s);
	
	uint32_t thing_88_size = backing->read<uint32_t>(header.unknown_88);
	thing_88 = backing->read_multiple<uint8_t>(thing_88_size);
	
	auto thing_80_table = backing->read<world_object_table>(header.unknown_80);
	thing_80_1 = backing->read_multiple<uint8_t>(0x800);
	thing_80_2 = backing->read_multiple<uint8_t>(thing_80_table.count_1 * 0x10);
	
	world_thing_7c_header thing_7c_header = backing->read<world_thing_7c_header>(header.unknown_7c);
	thing_7c_1s = backing->read_multiple<world_thing_7c_1>(thing_7c_header.count);
	thing_7c_2s = read_splines(
		backing->tell(),
		thing_7c_header.count,
		header.unknown_7c + thing_7c_header.part_2_data_offset);
	
	auto thing_98_header = backing->read<world_thing_98_header>(header.unknown_98);
	thing_98_1s = backing->read_multiple<world_thing_98>(thing_98_header.part_1_count);
	thing_98_2s = backing->read_multiple<uint32_t>(
		((thing_98_header.size - thing_98_header.part_1_count * sizeof(world_thing_98) - 0xc) / sizeof(uint32_t)));
	thing_98_header_8 = thing_98_header.unknown_8;
	thing_98_header_c = thing_98_header.unknown_c;
	
	if(header.unknown_90 != 0) {
		auto thing_90_header = backing->read<world_thing_90_header>(header.unknown_90);
		thing_90_1s = backing->read_multiple<world_thing_90>(thing_90_header.count_1);
		thing_90_2s = backing->read_multiple<world_thing_90>(thing_90_header.count_2);
		thing_90_3s = backing->read_multiple<world_thing_90>(thing_90_header.count_3);
	}
}

void world_segment::read_rac4() {
	
}

template <typename T_1, typename T_2 = char, typename T_3 = char>
void world_segment::read_table(
		uint32_t offset,
		std::vector<T_1>* first,
		std::vector<T_2>* second,
		std::vector<T_3>* third,
		bool align) {
	auto table = backing->read<world_object_table>(offset);
	if(first != nullptr) {
		*first = backing->read_multiple<T_1>(table.count_1);
	}
	if(align) backing->align(0x10, 0);
	if(second != nullptr) {
		*second = backing->read_multiple<T_2>(table.count_2);
	}
	if(align) backing->align(0x10, 0);
	if(third != nullptr) {
		*third = backing->read_multiple<T_3>(table.count_3);
	}
}

std::vector<game_string> world_segment::read_language(uint32_t offset) {
	std::vector<game_string> language;
	
	auto table = backing->read<world_string_table_header>(offset);
	std::vector<world_string_table_entry> entries(table.num_strings);
	backing->read_v(entries);
	
	for(world_string_table_entry& entry : entries) {
		backing->seek(offset + entry.string.value);
		language.push_back({ entry.id, entry.secondary_id, backing->read_string() });
	}
	
	return language;
}

std::vector<uint32_t> world_segment::read_u32_list(uint32_t offset) {
		auto count = backing->read<uint32_t>(offset);
		return backing->read_multiple<uint32_t>(count);
}

template <typename T_in_mem, typename T_on_disc>
std::vector<T_in_mem> world_segment::read_entity_table(
		uint32_t offset, std::function<void(T_in_mem&, T_on_disc&)> swap_ent) {
	auto table = backing->read<world_object_table>(offset);
	std::vector<T_on_disc> src = backing->read_multiple<T_on_disc>(table.count_1);
	std::vector<T_in_mem> dest;
	for(T_on_disc& on_disc : src) {
		T_in_mem& ent = dest.emplace_back();
		ent.id = { _next_entity_id++ };
		ent.selected = false;
		swap_ent(ent, on_disc);
	}
	return dest;
}

template <typename T>
void world_segment::read_terminated_array(std::vector<T>& dest, uint32_t offset) {
	backing->seek(offset);
	for(;;) {
		T thing = backing->read<T>();
		if(*(int32_t*) &thing > -1) {
			dest.push_back(thing);
		} else {
			break;
		}
	}
}

std::vector<std::vector<uint8_t>> world_segment::read_pvars(uint32_t table_offset, uint32_t data_offset) {
	int32_t pvar_count = 0;
	for(moby_entity& moby : mobies) {
		pvar_count = std::max(pvar_count, moby.pvar_index + 1);
	}
	
	std::vector<pvar_table_entry> table(pvar_count);
	backing->seek(table_offset);
	backing->read_v(table);
	
	std::vector<std::vector<uint8_t>> pvars;
	for(pvar_table_entry& entry : table) {
		uint32_t size = entry.size;
		std::vector<uint8_t>& pvar = pvars.emplace_back(size);
		backing->seek(data_offset + entry.offset);
		backing->read_v(pvar);
	}
	return pvars;
}

std::vector<spline_entity> world_segment::read_splines(
		uint32_t table_offset,
		uint32_t table_count,
		uint32_t data_offset) {
	std::vector<spline_entity> splines;
	std::vector<uint32_t> offsets = backing->read_multiple<uint32_t>(table_count);
	for(uint32_t offset : offsets) {
		auto vertex_header = backing->read<world_vertex_header>(data_offset + offset);
		spline_entity& spline = splines.emplace_back();
		spline.id = { _next_entity_id++ };
		spline.selected = false;
		for(size_t i = 0; i < vertex_header.vertex_count; i++) {
			glm::vec4 vertex;
			vertex.x = backing->read<float>();
			vertex.y = backing->read<float>();
			vertex.z = backing->read<float>();
			vertex.w = backing->read<float>();
			spline.vertices.push_back(vertex);
		}
	}
	return splines;
}

static const std::vector<char> EMPTY_VECTOR;

void world_segment::write_rac2() {
	world_header header;
	defer([&] {
		backing->write(0, header);
	});
	backing->seek(sizeof(world_header));
	
	const auto write_table = [&]<typename T_1, typename T_2 = char, typename T_3 = char>(
			const std::vector<T_1>& first,
			const std::vector<T_2>& second = EMPTY_VECTOR,
			const std::vector<T_3>& third = EMPTY_VECTOR,
			bool align = false) {
		backing->pad(0x10, 0);
		size_t base_pos = backing->tell();
		world_object_table table;
		table.count_1 = first.size();
		table.count_2 = second.size();
		table.count_3 = third.size();
		table.pad = 0;
		backing->write(table);
		backing->write_v(first);
		if(align) backing->pad(0x10, 0);
		backing->write_v(second);
		if(align) backing->pad(0x10, 0);
		backing->write_v(third);
		return base_pos;
	};
	
	header.unknown_8c = write_table(thing_8cs);
	
	backing->pad(0x10, 0);
	header.properties = backing->tell();
	backing->write(properties);
	backing->write_v(property_things);
	
	const auto write_language = [&](std::vector<game_string>& strings) {
		backing->pad(0x10, 0);
		size_t base_pos = backing->tell();
		backing->seek(base_pos + sizeof(world_string_table_header));
		
		size_t data_pos = sizeof(world_string_table_header) +
			strings.size() * sizeof(world_string_table_entry);
		for(game_string& str : strings) {
			world_string_table_entry entry;
			entry.string = data_pos;
			entry.id = str.id;
			entry.secondary_id = str.secondary_id;
			entry.padding = 0;
			backing->write(entry);
			data_pos += str.str.size() + 1;
			while(data_pos % 4 != 0) data_pos++;
		}
		
		for(game_string& str : strings) {
			backing->pad(0x4, 0);
			backing->write_n(str.str.c_str(), str.str.size() + 1);
		}
		backing->pad(0x10, 0);
		
		world_string_table_header string_table;
		string_table.num_strings = strings.size();
		string_table.size = data_pos;
		backing->write(base_pos, string_table);
		
		backing->seek(base_pos + data_pos);
		
		return base_pos;
	};
	header.english_strings = write_language(game_strings[0]);
	backing->pad(0x10, 0);
	header.unknown_14 = backing->tell();
	backing->write(thing_14);
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
		backing->pad(0x10, 0);
		size_t base_pos = backing->tell();
		backing->write<uint32_t>(list.size());
		backing->write_v(list);
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
	size_t pos_after_mobies = backing->tell();
	backing->write<uint32_t>(header.mobies + sizeof(uint32_t), max_moby_count - mobies.size());
	backing->seek(pos_after_mobies);
	
	backing->pad(0x10, 0);
	header.pvar_table = backing->tell();
	size_t next_pvar_offset = 0;
	header.pvar_data = header.pvar_table + pvars.size() * sizeof(pvar_table_entry);
	while(header.pvar_data % 0x10 != 0) header.pvar_data++;
	for(std::vector<uint8_t>& pvar : pvars) {
		pvar_table_entry entry;
		entry.offset = next_pvar_offset;
		entry.size = pvar.size();
		backing->write(entry);
		size_t next_pos = backing->tell();
		
		backing->seek(header.pvar_data + next_pvar_offset);
		backing->write_v(pvar);
		
		next_pvar_offset += pvar.size();
		backing->seek(next_pos);
	}
	backing->pad(0x10, 0);
	backing->seek(header.pvar_data + next_pvar_offset); // Skip past the pvar data section.
	
	const auto write_terminated_array = [&]<typename T>(const std::vector<T>& things) {
		backing->pad(0x10, 0);
		size_t result = backing->tell();
		backing->write_v(things);
		backing->write<uint64_t>(0xffffffffffffffff); // terminator
		return result;
	};
	header.unknown_58 = write_terminated_array(thing_58s);
	header.unknown_64 = write_terminated_array(thing_64s);
	header.unknown_50 = write_table(thing_50_1s, thing_50_2s);
	header.unknown_54 = write_table(thing_54_1s, thing_54_2s);
	header.unknown_30 = write_u32_list(thing_30s);
	header.ties = write_entity_table.operator()<tie_entity, world_tie>(ties, swap_tie);
	
	backing->pad(0x10, 0);
	header.unknown_94 = backing->tell();
	for(size_t i = 0; i < thing_94s.size(); i++) {
		world_thing_94 thing_94_header;
		thing_94_header.index = i;
		thing_94_header.size = thing_94s[i].size() >> 1;
		backing->write(thing_94_header);
		backing->write_v(thing_94s[i]);
	}
	backing->write<uint16_t>(0xffff);
	
	header.unknown_38 = write_table(thing_38_1s, thing_38_2s, EMPTY_VECTOR, true);
	header.unknown_3c = write_u32_list(thing_3cs);
	header.shrubs = write_entity_table.operator()<shrub_entity, world_shrub>(shrubs, swap_shrub);
	header.unknown_44 = write_table(thing_44_1s, thing_44_2s, EMPTY_VECTOR, true);
	
	const auto write_vertex_list = [&](std::vector<std::vector<glm::vec4>> list) {
		size_t base_pos = backing->tell();
		
		backing->seek(backing->tell() + list.size() * sizeof(uint32_t));
		backing->pad(0x10, 0);
		size_t data_pos = backing->tell();
		
		std::vector<uint32_t> offsets;
		for(std::vector<glm::vec4> vertices : list) {
			backing->pad(0x10, 0);
			offsets.push_back(backing->tell() - data_pos);
			world_vertex_header vertex_header;
			vertex_header.vertex_count = vertices.size();
			vertex_header.pad[0] = 0;
			vertex_header.pad[1] = 0;
			vertex_header.pad[2] = 0;
			backing->write(vertex_header);
			for(glm::vec4& vertex : vertices) {
				backing->write(vertex.x);
				backing->write(vertex.y);
				backing->write(vertex.z);
				backing->write(vertex.w);
			}
		}
		
		size_t end_pos = backing->tell();
		backing->seek(base_pos);
		backing->write_v(offsets);
		backing->seek(end_pos);
		
		return data_pos;
	};
	
	backing->pad(0x10, 0);
	header.splines = backing->tell();
	
	world_spline_table spline_table;
	defer([&]() {
		backing->write(header.splines, spline_table);
	});
	backing->seek(backing->tell() + sizeof(world_spline_table));
	
	std::vector<std::vector<glm::vec4>> spline_vertices;
	for(spline_entity& spline : splines) {
		spline_vertices.push_back(spline.vertices);
	} 
	spline_table.spline_count = splines.size();
	spline_table.data_offset = write_vertex_list(spline_vertices) - header.splines;
	spline_table.data_size = backing->tell() - header.splines - spline_table.data_offset;
	spline_table.pad = 0;
	
	header.triggers = write_entity_table.operator()<trigger_entity, world_trigger>(triggers, swap_trigger);
	
	header.unknown_6c = write_table(thing_6cs);
	header.unknown_70 = write_table(thing_70s);
	header.unknown_74 = write_table(thing_74s); // Not sure if this is right.
	
	backing->pad(0x10, 0);
	header.unknown_88 = backing->tell();
	backing->write<uint32_t>(thing_88.size());
	backing->write_v(thing_88);
	
	backing->pad(0x10, 0);
	header.unknown_80 = backing->tell();
	world_object_table thing_80_table;
	thing_80_table.count_1 = thing_80_2.size() / 0x10;
	thing_80_table.count_2 = 0;
	thing_80_table.count_3 = 0;
	thing_80_table.pad = 0;
	backing->write(thing_80_table);
	backing->write_v(thing_80_1);
	backing->write_v(thing_80_2);
	
	backing->pad(0x10, 0);
	header.unknown_7c = backing->tell();
	
	world_thing_7c_header thing_7c_header;
	defer([&]() {
		backing->write(header.unknown_7c, thing_7c_header);
	});
	backing->seek(backing->tell() + sizeof(world_thing_7c_header));
	
	backing->write_v(thing_7c_1s);
	backing->pad(0x10, 0);
	
	std::vector<std::vector<glm::vec4>> thing_7c_vertices;
	for(spline_entity& thing : thing_7c_2s) {
		thing_7c_vertices.push_back(thing.vertices);
	}
	thing_7c_header.count = thing_7c_1s.size();
	thing_7c_header.part_2_data_offset = write_vertex_list(thing_7c_vertices) - header.unknown_7c;
	thing_7c_header.part_2_data_size = backing->tell() -
		thing_7c_header.part_2_data_offset - header.unknown_7c;
	thing_7c_header.pad = 0;
	
	backing->pad(0x10, 0);
	header.unknown_98 = backing->tell();
	world_thing_98_header thing_98_header;
	thing_98_header.size =
		sizeof(world_thing_98_header) - // header
		sizeof(uint32_t) + // size field (not included in the size)
		thing_98_1s.size() * sizeof(world_thing_98) + // part 1
		thing_98_2s.size() * sizeof(uint32_t); // part 2
	thing_98_header.part_1_count = thing_98_1s.size();
	thing_98_header.unknown_8 = thing_98_header_8;
	thing_98_header.unknown_c = thing_98_header_c;
	backing->write(thing_98_header);
	backing->write_v(thing_98_1s);
	backing->write_v(thing_98_2s);
	
	if(thing_90_1s.size() > 0 || thing_90_2s.size() > 0 || thing_90_3s.size() > 0) {
		backing->pad(0x40, 0);
		header.unknown_90 = write_table(thing_90_1s, thing_90_2s, thing_90_3s);
	} else {
		header.unknown_90 = 0;
	}
}

// We can't pass around references to fields as we're using packed structs so
// instead of std::swap we have to use this macro.
#define SWAP_PACKED(inmem, packed) \
	{ \
		auto p = packed; \
		packed = inmem; \
		inmem = p; \
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

void swap_trigger(trigger_entity& l, world_trigger& r) {
	SWAP_PACKED(l.local_to_world, r.mat1);
	SWAP_PACKED(l.matrix_reloaded, r.mat2);
}
