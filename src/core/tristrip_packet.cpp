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

GeometryPackets generate_tristrip_packets(const GeometryPrimitives& input, const std::vector<Material>& materials, const std::vector<EffectiveMaterial>& effectives, const TriStripConfig& config) {
	TriStripPacketGenerator generator(materials, effectives, config.constraints, config.support_instancing);
	for(const GeometryPrimitive& primitive : input.primitives) {
		switch(primitive.type) {
			case GeometryType::TRIANGLE_LIST: {
				generator.add_list(&input.indices[primitive.index_begin], primitive.index_count, primitive.effective_material);
				break;
			}
			case GeometryType::TRIANGLE_STRIP: {
				generator.add_strip(&input.indices[primitive.index_begin], primitive.index_count, primitive.effective_material);
				break;
			}
			case GeometryType::TRIANGLE_FAN: {
				verify_not_reached("Tri fan packet generation not yet implemented.");
				break;
			}
		}
	}
	return generator.get_output();
}

TriStripPacketGenerator::TriStripPacketGenerator(
	const std::vector<Material>& materials,
	const std::vector<EffectiveMaterial>& effectives,
	const TriStripConstraints& constraints,
	bool support_instancing)
	: _materials(materials)
	, _effectives(effectives)
	, _constraints(constraints)
	, _support_instancing(support_instancing) {}

void TriStripPacketGenerator::add_list(const s32* indices, s32 index_count, s32 effective_material) {
	if(index_count < 3) {
		return;
	}
	
	if(effective_material != _current_effective_material) {
		if(!try_add_material()) {
			new_packet();
			add_list(indices, index_count, effective_material);
			return;
		}
		_current_effective_material = effective_material;
	}
	
	if(!try_add_strip()) {
		new_packet();
		add_list(indices, index_count, effective_material);
		return;
	}
	
	// See if we can add the first face.
	bool valid = true;
	valid &= try_add_unique_vertex();
	valid &= try_add_unique_vertex();
	valid &= try_add_unique_vertex();
	if(!valid) {
		new_packet();
		add_list(indices, index_count, effective_material);
		return;
	}
	
	// Setup the triangle list.
	_packet->primitive_count++;
	GeometryPrimitive& dest_primitive = _output.primitives.emplace_back();
	dest_primitive.type = GeometryType::TRIANGLE_LIST;
	dest_primitive.index_begin = (s32) _output.indices.size();
	dest_primitive.index_count = 0;
	dest_primitive.effective_material = _current_effective_material;
	
	// Add the first face.
	dest_primitive.index_count += 3;
	_output.indices.emplace_back(indices[0]);
	_output.indices.emplace_back(indices[1]);
	_output.indices.emplace_back(indices[2]);
	
	// Add as many of the remaining faces as possible.
	for(s32 j = 3; j < index_count; j += 3) {
		bool valid = true;
		valid &= try_add_unique_vertex();
		valid &= try_add_unique_vertex();
		valid &= try_add_unique_vertex();
		if(!valid) {
			// We need to split up the strip.
			new_packet();
			add_list(indices + j, index_count - j, effective_material);
			return;
		}
		
		dest_primitive.index_count += 3;
		_output.indices.emplace_back(indices[j + 0]);
		_output.indices.emplace_back(indices[j + 1]);
		_output.indices.emplace_back(indices[j + 2]);
	}
}

static bool tri_has_zero_area(s32 v0, s32 v1, s32 v2) {
	return v0 == v1 || v0 == v2 || v1 == v2;
}

void TriStripPacketGenerator::add_strip(const s32* indices, s32 index_count, s32 effective_material) {
	if(index_count < 3) {
		return;
	}
	
	if(effective_material != _current_effective_material) {
		if(!try_add_material()) {
			new_packet();
			add_strip(indices, index_count, effective_material);
			return;
		}
		_current_effective_material = effective_material;
	}
	
	if(!try_add_strip()) {
		new_packet();
		add_strip(indices, index_count, effective_material);
		return;
	}
	
	// See if we can add the first face.
	bool valid = true;
	valid &= try_add_unique_vertex();
	valid &= try_add_unique_vertex();
	valid &= try_add_unique_vertex();
	if(!valid) {
		new_packet();
		add_strip(indices, index_count, effective_material);
		return;
	}
	
	// Setup the triangle strip.
	_packet->primitive_count++;
	GeometryPrimitive& dest_primitive = _output.primitives.emplace_back();
	dest_primitive.type = GeometryType::TRIANGLE_STRIP;
	dest_primitive.index_begin = (s32) _output.indices.size();
	dest_primitive.index_count = 0;
	dest_primitive.effective_material = _current_effective_material;
	
	// Add the first face.
	dest_primitive.index_count += 3;
	_output.indices.emplace_back(indices[0]);
	_output.indices.emplace_back(indices[1]);
	_output.indices.emplace_back(indices[2]);
	
	// Add as many of the remaining faces as possible.
	for(s32 j = 3; j < index_count; j++) {
		if(!try_add_unique_vertex()) {
			// We need to split up the strip.
			new_packet();
			for(; j < index_count && tri_has_zero_area(indices[j - 2], indices[j - 1], indices[j]); j++);
			add_strip(indices + j - 2, index_count - j + 2, effective_material);
			return;
		}
		
		dest_primitive.index_count++;
		_output.indices.emplace_back(indices[j]);
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
	if(_support_instancing) {
		_current_effective_material = -1;
	}
}

bool TriStripPacketGenerator::try_add_strip() {
	if(_packet == nullptr) {
		return false;
	}
	for(s32 i = 0; i < _constraints.num_constraints; i++) {
		s32 cost = 0;
		cost += _constraints.constant_cost[i];
		cost += _constraints.strip_cost[i] * (_totals.strip_count + 1);
		cost += _constraints.vertex_cost[i] * _totals.vertex_count;
		cost += _constraints.index_cost[i] * _totals.index_count;
		cost += _constraints.material_cost[i] * _totals.material_count;
		if(cost > _constraints.max_cost[i]) {
			return false;
		}
	}
	_totals.strip_count++;
	return true;
}

bool TriStripPacketGenerator::try_add_unique_vertex() {
	if(_packet == nullptr) {
		return false;
	}
	for(s32 i = 0; i < _constraints.num_constraints; i++) {
		s32 cost = 0;
		cost += _constraints.constant_cost[i];
		cost += _constraints.strip_cost[i] * _totals.strip_count;
		cost += _constraints.vertex_cost[i] * (_totals.vertex_count + 1);
		cost += _constraints.index_cost[i] * (_totals.index_count + 1);
		cost += _constraints.material_cost[i] * _totals.material_count;
		if(cost > _constraints.max_cost[i]) {
			return false;
		}
	}
	_totals.vertex_count++;
	_totals.index_count++;
	return true;
}

bool TriStripPacketGenerator::try_add_duplicate_vertex() {
	if(_packet == nullptr) {
		return false;
	}
	for(s32 i = 0; i < _constraints.num_constraints; i++) {
		s32 cost = 0;
		cost += _constraints.constant_cost[i];
		cost += _constraints.strip_cost[i] * _totals.strip_count;
		cost += _constraints.vertex_cost[i] * _totals.vertex_count;
		cost += _constraints.index_cost[i] * (_totals.index_count + 1);
		cost += _constraints.material_cost[i] * _totals.material_count;
		if(cost > _constraints.max_cost[i]) {
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
	for(s32 i = 0; i < _constraints.num_constraints; i++) {
		s32 cost = 0;
		cost += _constraints.constant_cost[i];
		cost += _constraints.strip_cost[i] * _totals.strip_count;
		cost += _constraints.vertex_cost[i] * _totals.vertex_count;
		cost += _constraints.index_cost[i] * _totals.index_count;
		cost += _constraints.material_cost[i] * (_totals.material_count + 1);
		if(cost > _constraints.max_cost[i]) {
			return false;
		}
	}
	_totals.material_count++;
	return true;
}
