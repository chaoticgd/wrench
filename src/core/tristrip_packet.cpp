/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

TriStripPacketGenerator::TriStripPacketGenerator(
	const std::vector<EffectiveMaterial>& effectives,
	const std::vector<Material>& materials,
	const TriStripConstraints& constraints,
	bool support_instancing)
	: _effectives(effectives)
	, _materials(materials)
	, _constraints(constraints)
	, _support_instancing(support_instancing) {
	new_packet();
}

void TriStripPacketGenerator::add_strip(const StripFace* faces, s32 face_count, s32 effective_material) {
	if(face_count == 0) {
		return;
	}
	
	if(_last_effective_material != effective_material) {
		if(!try_add_material()) {
			new_packet();
			add_strip(faces, face_count, effective_material);
			return;
		}
		_last_effective_material = effective_material;
		_output_material = effective_material;
	}
	
	if(!try_add_strip()) {
		new_packet();
		add_strip(faces, face_count, effective_material);
		return;
	}
	
	// See if we can add the first face.
	bool valid = true;
	valid &= try_add_unique_vertex();
	valid &= try_add_unique_vertex();
	valid &= try_add_unique_vertex();
	if(!valid) {
		new_packet();
		add_strip(faces, face_count, effective_material);
		return;
	}
	
	// Setup the strip.
	_packet->strip_count++;
	FaceStrip& dest_strip = _output.strips.emplace_back();
	dest_strip.face_begin = (s32) _output.faces.size();
	dest_strip.face_count = 0;
	dest_strip.material = _output_material;
	
	// Add the first face.
	dest_strip.face_count++;
	_output.faces.emplace_back(faces[0]);
	
	// Add as many of the remaining faces as possible.
	for(s32 j = 1; j < face_count; j++) {
		if(!try_add_unique_vertex()) {
			// We need to split up the strip.
			new_packet();
			for(; j < face_count && faces[j].is_zero_area(); j++);
			add_strip(faces + j, face_count - j, effective_material);
			return;
		}
		
		dest_strip.face_count++;
		_output.faces.emplace_back(faces[j]);
	}
	
	_output_material = -1;
}

void TriStripPacketGenerator::add_list(const VertexIndex* indices, s32 face_count, s32 effective_material) {
	
}

FaceStripPackets TriStripPacketGenerator::get_output() {
	return std::move(_output);
}

void TriStripPacketGenerator::new_packet() {
	_totals = {}; // Reset the constraint calculations.
	_packet = &_output.packets.emplace_back();
	_packet->strip_begin = _output.strips.size();
	_packet->strip_count = 0;
	if(_support_instancing) {
		_last_effective_material = -1;
	}
}

bool TriStripPacketGenerator::try_add_strip() {
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
