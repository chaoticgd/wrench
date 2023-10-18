/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#include "tristrip_packet.h"

static s32 rounded_index_cost(s32 count, s32 multiple);

GeometryPackets generate_tristrip_packets(const GeometryPrimitives& input, const TriStripConfig& config) {
	TriStripPacketGenerator generator(config);
	for(const GeometryPrimitive& primitive : input.primitives) {
		generator.add_primitive(&input.indices[primitive.index_begin], primitive.index_count, primitive.type, primitive.effective_material);
	}
	return generator.get_output();
}

TriStripPacketGenerator::TriStripPacketGenerator(
	const TriStripConfig& config)
	: _config(config) {}

static bool tri_has_zero_area(s32 v0, s32 v1, s32 v2) {
	return v0 == v1 || v0 == v2 || v1 == v2;
}

void TriStripPacketGenerator::add_primitive(const s32* indices, s32 index_count, GeometryType type, s32 effective_material) {
	if(index_count < 3) {
		return;
	}
	
	if(effective_material != _current_effective_material) {
		if(!try_add_material()) {
			new_packet();
			add_primitive(indices, index_count, type, effective_material);
			return;
		}
		_current_effective_material = effective_material;
	}
	
	if(!try_add_strip()) {
		new_packet();
		add_primitive(indices, index_count, type, effective_material);
		return;
	}
	
	// See if we can add the first face.
	bool valid = true;
	valid &= try_add_vertex(indices[0]);
	valid &= try_add_vertex(indices[1]);
	valid &= try_add_vertex(indices[2]);
	if(!valid) {
		new_packet();
		add_primitive(indices, index_count, type, effective_material);
		return;
	}
	
	// Setup the primitive.
	_packet->primitive_count++;
	GeometryPrimitive& dest_primitive = _output.primitives.emplace_back();
	dest_primitive.type = type;
	dest_primitive.index_begin = (s32) _output.indices.size();
	dest_primitive.index_count = 0;
	dest_primitive.effective_material = _current_effective_material;
	
	// Add the first face.
	dest_primitive.index_count += 3;
	_output.indices.emplace_back(indices[0]);
	_output.indices.emplace_back(indices[1]);
	_output.indices.emplace_back(indices[2]);
	
	// Add as many of the remaining faces as possible.
	if(type == GeometryType::TRIANGLE_LIST) {
		for(s32 j = 3; j < index_count; j += 3) {
			bool valid = true;
			valid &= try_add_vertex(indices[j + 0]);
			valid &= try_add_vertex(indices[j + 1]);
			valid &= try_add_vertex(indices[j + 2]);
			if(!valid) {
				// We need to split up the strip.
				new_packet();
				add_primitive(indices + j, index_count - j, type, effective_material);
				return;
			}
			
			dest_primitive.index_count += 3;
			_output.indices.emplace_back(indices[j + 0]);
			_output.indices.emplace_back(indices[j + 1]);
			_output.indices.emplace_back(indices[j + 2]);
		}
	} else {
		for(s32 j = 3; j < index_count; j++) {
			if(!try_add_vertex(indices[j])) {
				// We need to split up the strip.
				new_packet();
				for(; j < index_count && tri_has_zero_area(indices[j - 2], indices[j - 1], indices[j]); j++);
				add_primitive(indices + j - 2, index_count - j + 2, type, effective_material);
				return;
			}
			
			dest_primitive.index_count++;
			_output.indices.emplace_back(indices[j]);
		}
	}
}

GeometryPackets TriStripPacketGenerator::get_output() {
	return std::move(_output);
}

void TriStripPacketGenerator::new_packet() {
	_totals = {}; // Reset the constraint calculations.
	_packet = &_output.packets.emplace_back();
	_packet->primitive_begin = _output.primitives.size();
	_packet->primitive_count = 0;
	if(_config.support_instancing) {
		_current_effective_material = -1;
	}
	_current_packet++;
}

bool TriStripPacketGenerator::try_add_strip() {
	if(_packet == nullptr) {
		return false;
	}
	for(const TriStripConstraint& constraint : _config.constraints) {
		s32 cost = 0;
		cost += constraint.constant_cost;
		cost += constraint.strip_cost * (_totals.strip_count + 1);
		cost += constraint.vertex_cost * _totals.vertex_count;
		cost += rounded_index_cost(constraint.index_cost * _totals.index_count, constraint.round_index_cost_up_to_multiple_of);
		cost += constraint.material_cost * _totals.material_count;
		if(cost > constraint.max_cost) {
			return false;
		}
	}
	_totals.strip_count++;
	return true;
}

bool TriStripPacketGenerator::try_add_vertex(s32 index) {
	bool added;
	if(_config.support_index_buffer) {
		index &= ~(1 << 31); // Clear the sign bit, which could be being used to store the primitive restart bit.
		if(index >= _last_packet_with_vertex.size()) {
			_last_packet_with_vertex.resize(index + 1, -1);
		}
		if(_last_packet_with_vertex[index] != _current_packet) {
			added = try_add_unique_vertex();
			_last_packet_with_vertex[index] = _current_packet;
		} else {
			added = try_add_repeated_vertex();
		}
	} else {
		added = try_add_unique_vertex();
	}
	return added;
}

bool TriStripPacketGenerator::try_add_unique_vertex() {
	if(_packet == nullptr) {
		return false;
	}
	for(const TriStripConstraint& constraint : _config.constraints) {
		s32 cost = 0;
		cost += constraint.constant_cost;
		cost += constraint.strip_cost * _totals.strip_count;
		cost += constraint.vertex_cost * (_totals.vertex_count + 1);
		cost += rounded_index_cost(constraint.index_cost * (_totals.index_count + 1), constraint.round_index_cost_up_to_multiple_of);
		cost += constraint.material_cost * _totals.material_count;
		if(cost > constraint.max_cost) {
			return false;
		}
	}
	_totals.vertex_count++;
	_totals.index_count++;
	return true;
}

bool TriStripPacketGenerator::try_add_repeated_vertex() {
	if(_packet == nullptr) {
		return false;
	}
	for(const TriStripConstraint& constraint : _config.constraints) {
		s32 cost = 0;
		cost += constraint.constant_cost;
		cost += constraint.strip_cost * _totals.strip_count;
		cost += constraint.vertex_cost * _totals.vertex_count;
		cost += rounded_index_cost(constraint.index_cost * (_totals.index_count + 1), constraint.round_index_cost_up_to_multiple_of);
		cost += constraint.material_cost * _totals.material_count;
		if(cost > constraint.max_cost) {
			return false;
		}
	}
	_totals.index_count++;
	return true;
}

bool TriStripPacketGenerator::try_add_material() {
	if(_packet == nullptr) {
		return false;
	}
	for(const TriStripConstraint& constraint : _config.constraints) {
		s32 cost = 0;
		cost += constraint.constant_cost;
		cost += constraint.strip_cost * _totals.strip_count;
		cost += constraint.vertex_cost * _totals.vertex_count;
		cost += rounded_index_cost(constraint.index_cost * _totals.index_count, constraint.round_index_cost_up_to_multiple_of);
		cost += constraint.material_cost * (_totals.material_count + 1);
		if(cost > constraint.max_cost) {
			return false;
		}
	}
	_totals.material_count++;
	return true;
}

static s32 rounded_index_cost(s32 count, s32 multiple) {
	if((count % multiple) != 0) {
		count += multiple - (count % multiple);
	}
	return count;
}
