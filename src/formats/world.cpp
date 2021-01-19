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
	world_header_rac23 header = backing->read<world_header_rac23>(0);
	
	read_table(header.unknown_8c, &thing_8cs);
	
	properties = backing->read<world_properties>(header.properties);
	world_property_thing property_thing;
	do {
		property_thing = backing->read<world_property_thing>();
		property_things.push_back(property_thing);
	} while(property_things.size() < property_thing.count);
	
	switch(header.us_english_strings % 0x10) {
		case 0x0:
			game = world_type::RAC2;
			break;
		case 0x4:
			game = world_type::RAC3;
			break;
		case 0x8:
			game = world_type::DL;
			break;
		default:
			throw stream_format_error("Invalid language block alignment.");
	}
	
	languages[0] = read_language(header.us_english_strings, true);
	languages[1] = read_language(header.uk_english_strings, false);
	languages[2] = read_language(header.french_strings, false);
	languages[3] = read_language(header.german_strings, false);
	languages[4] = read_language(header.spanish_strings, false);
	languages[5] = read_language(header.italian_strings, false);
	languages[6] = read_language(header.japanese_strings, false);
	languages[7] = read_language(header.korean_strings, false);
	read_table(header.directional_lights, &directional_lights);
	read_table(header.unknown_84, &thing_84s);
	read_table(header.unknown_8, &thing_8s);
	read_table(header.unknown_c, &thing_cs);
	thing_48s = read_u32_list(header.unknown_48);
	mobies = read_entity_table<moby_entity, world_moby_rac23>(header.mobies, swap_moby_rac23);
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
	splines = read_splines<regular_spline_entity>(
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
	
	world_grindrail_header thing_7c_header = backing->read<world_grindrail_header>(header.grindrails);
	auto grindrails_part_1 = backing->read_multiple<world_grindrail_part_1>(thing_7c_header.count);
	grindrails = read_splines<grindrail_spline_entity>(
		backing->tell(),
		thing_7c_header.count,
		header.grindrails + thing_7c_header.part_2_data_offset);
	for(size_t i = 0; i < thing_7c_header.count; i++) {
		grindrail_spline_entity& dest = grindrails[i];
		auto& src = grindrails_part_1[i];
		dest.special_point = glm::vec4(src.x, src.y, src.z, src.w);
		std::memcpy(dest.unknown_10, src.unknown_10, 0x10);
	}
	
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
	world_header_rac4 header = backing->read<world_header_rac4>(0);
	
	mobies = read_entity_table<moby_entity, world_moby_rac4>(header.mobies, swap_moby_rac4);
	max_moby_count = mobies.size() + backing->read<uint32_t>(header.mobies + sizeof(uint32_t));
	
	auto spline_table = backing->read<world_spline_table>(header.splines);
	splines = read_splines<regular_spline_entity>(
		backing->tell(),
		spline_table.spline_count,
		header.splines + spline_table.data_offset);
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

game_language world_segment::read_language(uint32_t offset, bool is_english) {
	game_language language;
	
	if(game == world_type::RAC3 && is_english) {
		language.unknown = backing->read<uint32_t>(offset - 0x4);
	}
	
	auto table = backing->read<world_string_table_header>(offset);
	std::vector<world_string_table_entry> entries(table.num_strings);
	backing->read_v(entries);
	
	if(game == world_type::RAC3) {
		offset += sizeof(world_string_table_header);
	}
	
	for(world_string_table_entry& entry : entries) {
		backing->seek(offset + entry.string.value);
		language.strings.push_back({
			entry.id,
			entry.secondary_id,
			entry.unknown_c, entry.unknown_e,
			backing->read_string()
		});
		if(entry.string.value == 0xf648) printf("its here %x\n",*(uint32_t*)language.strings.back().str.c_str() & 0xff);
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

template <typename T>
std::vector<T> world_segment::read_splines(
		uint32_t table_offset,
		uint32_t table_count,
		uint32_t data_offset) {
	std::vector<T> splines;
	std::vector<uint32_t> offsets = backing->read_multiple<uint32_t>(table_count);
	for(uint32_t offset : offsets) {
		auto vertex_header = backing->read<world_vertex_header>(data_offset + offset);
		T& spline = splines.emplace_back();
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
	world_header_rac23 header;
	defer([&] {
		backing->write(0, header);
	});
	backing->seek(sizeof(world_header_rac23));
	
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
	
	const auto write_language = [&](game_language& language, bool is_english) {
		backing->pad(0x10, 0);
		if(game == world_type::RAC3 && is_english) {
			assert(language.unknown.has_value());
			backing->write<uint32_t>(*language.unknown);
		}
		size_t base_pos = backing->tell();
		backing->seek(base_pos + sizeof(world_string_table_header));
		if(!is_english)printf("bp=%x tell=%x\n", base_pos, backing->tell());
		
		size_t data_pos = sizeof(world_string_table_header) +
			language.strings.size() * sizeof(world_string_table_entry);
		for(game_string& str : language.strings) {
			world_string_table_entry entry;
			entry.string = data_pos;
			if(game == world_type::RAC3) {
				entry.string.value -= sizeof(world_string_table_header);
			}
			entry.id = str.id;
			entry.secondary_id = str.secondary_id;
			entry.unknown_c = str.unknown_c;
			entry.unknown_e = str.unknown_e;
			backing->write(entry);
			data_pos += str.str.size() + 1;
			if(game == world_type::RAC2) {
				while(data_pos % 4 != 0) data_pos++;
			}
		}
		
		for(game_string& str : language.strings) {
			if(game == world_type::RAC2) {
				backing->pad(0x4, 0);
			}
			if(str.str == "\x81\xBA\x83\x9C\x81\x66\x20\x83\x03\x82\xAF\x80\x67\x20\x56\x34") printf("write here %x %x\n", backing->tell()-base_pos,*(uint32_t*)str.str.c_str() & 0xff);
			backing->write_n(str.str.c_str(), str.str.size() + 1);
		}
		backing->pad(0x10, 0);
		
		world_string_table_header string_table;
		string_table.num_strings = language.strings.size();
		string_table.size = data_pos;
		if(game == world_type::RAC3) {
			string_table.size -= sizeof(world_string_table_header);
		}
		backing->write(base_pos, string_table);
		
		backing->seek(base_pos + data_pos);
		
		return base_pos;
	};
	header.us_english_strings = write_language(languages[0], true);
	header.uk_english_strings = write_language(languages[1], false);
	header.french_strings = write_language(languages[2], false);
	header.german_strings = write_language(languages[3], false);
	header.spanish_strings = write_language(languages[4], false);
	header.italian_strings = write_language(languages[5], false);
	header.japanese_strings = write_language(languages[6], false);
	header.korean_strings = write_language(languages[7], false);
	
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
	header.mobies = write_entity_table.operator()<moby_entity, world_moby_rac23>(mobies, swap_moby_rac23);
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
	header.grindrails = backing->tell();
	
	world_grindrail_header thing_7c_header;
	defer([&]() {
		backing->write(header.grindrails, thing_7c_header);
	});
	backing->seek(backing->tell() + sizeof(world_grindrail_header));
	
	for(grindrail_spline_entity& grindrail : grindrails) {
		world_grindrail_part_1 dest;
		dest.x = grindrail.special_point.x;
		dest.y = grindrail.special_point.y;
		dest.z = grindrail.special_point.z;
		dest.w = grindrail.special_point.w;
		std::memcpy(dest.unknown_10, grindrail.unknown_10, 0x10);
		backing->write(dest);
	}
	
	std::vector<std::vector<glm::vec4>> grind_rail_vertices;
	for(spline_entity& grindrail : grindrails) {
		grind_rail_vertices.push_back(grindrail.vertices);
	}
	thing_7c_header.count = grindrails.size();
	thing_7c_header.part_2_data_offset = write_vertex_list(grind_rail_vertices) - header.grindrails;
	thing_7c_header.part_2_data_size = backing->tell() -
		thing_7c_header.part_2_data_offset - header.grindrails;
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

void swap_moby_rac23(moby_entity& l, world_moby_rac23& r) {
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

void swap_moby_rac4(moby_entity& l, world_moby_rac4& r) {
	// euler_entity
	SWAP_PACKED(l.position, r.position);
	SWAP_PACKED(l.rotation, r.rotation);
	// moby_entity
	SWAP_PACKED(l.size, r.size);
	SWAP_PACKED(l.uid, r.uid);
	SWAP_PACKED(l.class_num, r.class_num);
	SWAP_PACKED(l.scale, r.scale);
	// TODO: Figure out what the rest of the fields are and swap them.
	l.unknown_4 = 0;
	l.unknown_8 = 0;
	l.unknown_c = 0;
	l.unknown_14 = 0;
	l.unknown_18 = 0;
	l.unknown_1c = 0;
	l.unknown_20 = 0;
	l.unknown_24 = 0;
	l.unknown_30 = 0;
	l.unknown_34 = 0;
	l.unknown_38 = 0;
	l.unknown_3c = 0;
	l.unknown_58 = 0;
	l.unknown_5c = 0;
	l.unknown_60 = 0;
	l.unknown_64 = 0;
	l.pvar_index = -1;
	l.unknown_6c = 0;
	l.unknown_70 = 0;
	l.unknown_74 = 0;
	l.unknown_78 = 0;
	l.unknown_7c = 0;
	l.unknown_80 = 0;
	l.unknown_84 = 0;
}

void swap_trigger(trigger_entity& l, world_trigger& r) {
	SWAP_PACKED(l.local_to_world, r.mat1);
	SWAP_PACKED(l.matrix_reloaded, r.mat2);
}
