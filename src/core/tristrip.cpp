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

#include "tristrip.h"

#include <core/mesh_graph.h>

// Code to generate triangle strips and split said strips into packets based on
// a set of size constraints.
//
// Some of the algorithms here were adapted from the NvTriStrip library.

static FaceStrip weave_multiple_strips_and_pick_the_best(FaceStrips& dest, MeshGraph& graph, const EffectiveMaterial& effective);
static FaceIndex find_start_face(const MeshGraph& graph, const EffectiveMaterial& effective, FaceIndex min_face);
static void weave_strip(FaceStrips& dest, FaceIndex start_face, EdgeIndex start_edge, bool to_v1, MeshGraph& graph, const EffectiveMaterial& effective);
static FaceStrip weave_strip_in_one_direction(FaceStrips& dest, FaceIndex start_face, VertexIndex nv0, VertexIndex nv1, std::vector<VertexIndex>& scratch_indices, MeshGraph& graph, const EffectiveMaterial& effective);
static FaceStripPackets generate_packets(const FaceStrips& strips, const std::vector<Material>& materials, const std::vector<EffectiveMaterial>& effectives, const TriStripConfig& config);
static TriStripPackets facestrips_to_tripstrips(const FaceStripPackets& input, const std::vector<EffectiveMaterial>& effectives);
static void facestrip_to_tristrip(TriStripPackets& output, const FaceStrip& face_strip, const std::vector<StripFace>& faces);
static VertexIndex unique_vertex_from_rhs(const StripFace& lhs, const StripFace& rhs);
static std::pair<VertexIndex, VertexIndex> get_shared_vertices(const StripFace& lhs, const StripFace& rhs);
static void verify_face_strips(const std::vector<FaceStrip>& strips, const std::vector<StripFace>& faces, const char* context, const MeshGraph& graph);

TriStripPackets weave_tristrips(const Mesh& mesh, const std::vector<Material>& materials, const TriStripConfig& config) {
	// Firstly we build a graph structure to make finding adjacent faces fast.
	MeshGraph graph(mesh);
	std::vector<EffectiveMaterial> effectives = effective_materials(materials, MATERIAL_ATTRIB_SURFACE | MATERIAL_ATTRIB_WRAP_MODE);
	FaceStrips strips;
	for(s32 i = 0; i < (s32) effectives.size(); i++) {
		const EffectiveMaterial& effective = effectives[i];
		for(;;) {
			bool done = true;
			for(FaceIndex j = {0}; j.index < graph.face_count(); j.index++) {
				if(graph.can_be_added_to_strip(j, effective)) {
					done = false;
				}
			}
			if(done) {
				break;
			}
			FaceStrip strip = weave_multiple_strips_and_pick_the_best(strips, graph, effective);
			strip.effective_material = i;
			for(s32 i = 0; i < strip.face_count; i++) {
				StripFace& face = strips.faces[strip.face_begin + i];
				if(face.index != NULL_FACE_INDEX) {
					graph.put_in_strip(face.index, (s32) strips.strips.size());
				}
			}
			strips.strips.emplace_back(strip);
		}
	}
	FaceStripPackets packets = generate_packets(strips, materials, effectives, config);
	verify_face_strips(packets.strips, packets.faces, "generate_packets", graph);
	// Convert those strips of faces to tristrips.
	return facestrips_to_tripstrips(packets, effectives);
}

static FaceStrip weave_multiple_strips_and_pick_the_best(FaceStrips& dest, MeshGraph& graph, const EffectiveMaterial& effective) {
	// Weave multiple candidate strips.
	FaceStrips temp;
	FaceIndex min_face = {0};
	for(s32 i = 0; i < 10; i++) {
		FaceIndex start_face = find_start_face(graph, effective, min_face);
		min_face = (start_face.index + 1) % graph.face_count();
		weave_strip(temp, start_face, graph.edge_of_face(start_face, 0), false, graph, effective);
		weave_strip(temp, start_face, graph.edge_of_face(start_face, 0), true, graph, effective);
		weave_strip(temp, start_face, graph.edge_of_face(start_face, 1), false, graph, effective);
		weave_strip(temp, start_face, graph.edge_of_face(start_face, 1), true, graph, effective);
		weave_strip(temp, start_face, graph.edge_of_face(start_face, 2), false, graph, effective);
		weave_strip(temp, start_face, graph.edge_of_face(start_face, 2), true, graph, effective);
	}
	
	// Determine which is the best.
	s32 best_strip = -1;
	s32 best_utility = -1;
	for(size_t i = 0; i < temp.strips.size(); i++) {
		FaceStrip& candidate = temp.strips[i];
		s32 utility = candidate.face_count - candidate.zero_area_tri_count;
		if(utility > best_utility) {
			best_strip = i;
			best_utility = utility;
		}
	}
	
	assert(best_strip != -1);
	
	// Copy the best strip from the temp array to the main array.
	FaceStrip strip;
	strip.face_begin = (s32) dest.faces.size();
	strip.face_count = temp.strips[best_strip].face_count;
	strip.zero_area_tri_count = temp.strips[best_strip].zero_area_tri_count;
	for(s32 i = 0; i < temp.strips[best_strip].face_count; i++) {
		dest.faces.emplace_back(temp.faces[temp.strips[best_strip].face_begin + i]);
	}
	return strip;
}

static FaceIndex find_start_face(const MeshGraph& graph, const EffectiveMaterial& effective, FaceIndex min_face) {
	for(s32 neighbour_count = 0; neighbour_count <= 3; neighbour_count++) {
		for(FaceIndex face = min_face; face.index < graph.face_count(); face.index++) {
			s32 neighbours = 0;
			for(s32 i = 0; i < 3; i++) {
				FaceIndex other_face = graph.other_face(graph.edge_of_face(face, i), face);
				if(other_face != NULL_FACE_INDEX && graph.can_be_added_to_strip(other_face, effective)) {
					neighbours++;
				}
			}
			if(neighbours == neighbour_count && graph.can_be_added_to_strip(face, effective)) {
				return face;
			}
		}
		for(FaceIndex face = 0; face < min_face; face.index++) {
			s32 neighbours = 0;
			for(s32 i = 0; i < 3; i++) {
				FaceIndex other_face = graph.other_face(graph.edge_of_face(face, i), face);
				if(other_face != NULL_FACE_INDEX && graph.can_be_added_to_strip(other_face, effective)) {
					neighbours++;
				}
			}
			if(neighbours == neighbour_count && graph.can_be_added_to_strip(face, effective)) {
				return face;
			}
		}
	}
	return NULL_FACE_INDEX;
}

static void weave_strip(FaceStrips& dest, FaceIndex start_face, EdgeIndex start_edge, bool to_v1, MeshGraph& graph, const EffectiveMaterial& effective) {
	FaceStrip& strip = dest.strips.emplace_back();
	strip.face_count = 0;
	strip.face_begin = (s32) dest.faces.size();
	
	std::vector<VertexIndex> scratch_indices;
	
	VertexIndex v0 = to_v1 ? graph.edge_vertex(start_edge, 0) : graph.edge_vertex(start_edge, 1);
	VertexIndex v1 = to_v1 ? graph.edge_vertex(start_edge, 1) : graph.edge_vertex(start_edge, 0);
	
	scratch_indices.emplace_back(v0);
	scratch_indices.emplace_back(v1);
	
	VertexIndex v2 = graph.next_index(scratch_indices, start_face);
	scratch_indices.emplace_back(v2);
	if(v2 == NULL_VERTEX_INDEX) {
		printf("warning: Tristrip weaving failed. Failed to find v2.\n");
		return;
	}
	
	// Do this up here so we don't accidentally add the start face twice.
	graph.put_in_temp_strip(start_face);
	
	// Stores the separate forward and backward strips.
	FaceStrips temp;
	
	// Weave the strip forwards.
	FaceStrip forward = weave_strip_in_one_direction(temp, start_face, v1, v2, scratch_indices, graph, effective);
	
	// Weave the strip backwards.
	scratch_indices.resize(0);
	scratch_indices.push_back(v2);
	scratch_indices.push_back(v1);
	scratch_indices.push_back(v0);
	FaceStrip backward = weave_strip_in_one_direction(temp, start_face, v1, v0, scratch_indices, graph, effective);
	
	// Merge the strips.
	strip.face_count = backward.face_count + 1 + forward.face_count;
	strip.zero_area_tri_count = backward.zero_area_tri_count + forward.zero_area_tri_count;
	for(s32 i = 0; i < backward.face_count; i++) {
		dest.faces.emplace_back(temp.faces[backward.face_begin + (backward.face_count - i - 1)]);
	}
	dest.faces.emplace_back(v0, v1, v2, start_face);
	for(s32 i = 0; i < forward.face_count; i++) {
		dest.faces.emplace_back(temp.faces[forward.face_begin + i]);
	}
	
	// Make it so this strip no longer registers as already part of a strip
	// (since it might be discarded in favour of a better strip).
	graph.discard_temp_strip();
}

static FaceStrip weave_strip_in_one_direction(FaceStrips& dest, FaceIndex start_face, VertexIndex nv0, VertexIndex nv1, std::vector<VertexIndex>& scratch_indices, MeshGraph& graph, const EffectiveMaterial& effective) {
	FaceStrip strip;
	strip.face_begin = (s32) dest.faces.size();
	strip.face_count = 0;
	
	FaceIndex next_face = graph.other_face(graph.edge(nv0, nv1), start_face);
	
	while(next_face != NULL_FACE_INDEX && graph.can_be_added_to_strip(next_face, effective)) {
		VertexIndex testnv0 = nv1;
		VertexIndex testnv1 = graph.next_index(scratch_indices, next_face);
		if(testnv1 == NULL_VERTEX_INDEX) {
			printf("warning: Tristrip weaving failed. Failed to find testnv1.\n");
			return strip;
		}
		
		FaceIndex next_next_face = graph.other_face(graph.edge(testnv0, testnv1), next_face);
		if(next_next_face == NULL_FACE_INDEX || !graph.can_be_added_to_strip(next_next_face, effective)) {
			FaceIndex test_next_face = graph.other_face(graph.edge(nv0, testnv1), next_face);
			if(test_next_face != NULL_FACE_INDEX && graph.can_be_added_to_strip(test_next_face, effective)) {
				strip.face_count++;
				dest.faces.emplace_back(nv0, nv1, nv0, NULL_FACE_INDEX);
				graph.put_in_temp_strip(test_next_face);
				strip.zero_area_tri_count++;
				
				scratch_indices.emplace_back(nv0);
				testnv0 = nv0;
			}
			break;
		}
		
		VertexIndex ev0 = graph.face_vertex(next_face, 0);
		VertexIndex ev1 = graph.face_vertex(next_face, 1);
		VertexIndex ev2 = graph.face_vertex(next_face, 2);
		
		strip.face_count++;
		assert(next_face != NULL_FACE_INDEX);
		dest.faces.emplace_back(ev0, ev1, ev2, next_face);
		graph.put_in_temp_strip(next_face);
		
		scratch_indices.emplace_back(testnv1);
		
		nv0 = testnv0;
		nv1 = testnv1;
		
		next_face = graph.other_face(graph.edge(nv0, nv1), next_face);
	}
	
	return strip;
}

static FaceStripPackets generate_packets(const FaceStrips& strips, const std::vector<Material>& materials, const std::vector<EffectiveMaterial>& effectives, const TriStripConfig& config) {
	TriStripPacketGenerator generator(materials, effectives, config.constraints, config.support_instancing);
	for(const FaceStrip& strip : strips.strips) {
		generator.add_strip(&strips.faces[strip.face_begin], strip.face_count, strip.effective_material);
	}
	return generator.get_output();
}

static TriStripPackets facestrips_to_tripstrips(const FaceStripPackets& input, const std::vector<EffectiveMaterial>& effectives) {
	TriStripPackets output;
	
	for(const FaceStripPacket& face_packet : input.packets) {
		TriStripPacket& tri_packet = output.packets.emplace_back();
		tri_packet.strip_begin = face_packet.strip_begin;
		tri_packet.strip_count = face_packet.strip_count;
		for(s32 i = 0; i < face_packet.strip_count; i++) {
			TriStrip& tri_strip = output.strips.emplace_back();
			const FaceStrip& face_strip = input.strips[face_packet.strip_begin + i];
			tri_strip.index_begin = output.indices.size();
			tri_strip.index_count = 2 + face_strip.face_count;
			if(face_strip.effective_material >= 0) {
				tri_strip.material = effectives[face_strip.effective_material].materials.at(0);
			} else {
				tri_strip.material = -1;
			}
			assert(face_strip.face_count >= 1);
			
			facestrip_to_tristrip(output, face_strip, input.faces);
		}
	}
	
	return output;
}

static void facestrip_to_tristrip(TriStripPackets& output, const FaceStrip& face_strip, const std::vector<StripFace>& faces) {
	if(face_strip.face_count == 0) {
		return;
	}
	
	// Process the first face.
	StripFace first_face = faces[face_strip.face_begin];
	if(face_strip.face_count >= 2) {
		// Reorder the vertices of the first face such that a strip
		// can be more easily constructed.
		StripFace second_face = faces[face_strip.face_begin + 1];
		VertexIndex unique = unique_vertex_from_rhs(second_face, first_face);
		if(unique == first_face.v[1]) {
			std::swap(first_face.v[0], first_face.v[1]);
		} else if(unique == first_face.v[2]) {
			std::swap(first_face.v[0], first_face.v[2]);
		}
		if(face_strip.face_count >= 3) {
			// Same thing, but with the third face.
			StripFace third_face = faces[face_strip.face_begin + 2];
			if(second_face.is_zero_area()) {
				VertexIndex pivot = second_face.v[1];
				if(first_face.v[1] == pivot) {
					std::swap(first_face.v[1], first_face.v[2]);
				}
			} else {
				auto [shared_0, shared_1] = get_shared_vertices(third_face, first_face);
				if(shared_0 == first_face.v[1] && shared_1 == -1) {
					std::swap(first_face.v[1], first_face.v[2]);
				}
			}
		}
	}
	
	// Now actually add the first face.
	assert(first_face.v[0] != NULL_VERTEX_INDEX);
	assert(first_face.v[1] != NULL_VERTEX_INDEX);
	assert(first_face.v[2] != NULL_VERTEX_INDEX);
	output.indices.emplace_back(first_face.v[0].index);
	output.indices.emplace_back(first_face.v[1].index);
	output.indices.emplace_back(first_face.v[2].index);
	
	// Process the rest of the faces.
	StripFace last_face = first_face;
	for(s32 j = 1; j < face_strip.face_count; j++) {
		const StripFace& face = faces[face_strip.face_begin + j];
		VertexIndex unique = unique_vertex_from_rhs(last_face, face);
		if(unique != NULL_VERTEX_INDEX) {
			assert(unique != NULL_VERTEX_INDEX);
			output.indices.emplace_back(unique.index);
			last_face.v[0] = last_face.v[1];
			last_face.v[1] = last_face.v[2];
			last_face.v[2] = unique;
		} else {
			// Insert a zero area triangle.
			assert(face.v[2] != NULL_VERTEX_INDEX);
			output.indices.emplace_back(face.v[2].index);
			last_face.v[0] = face.v[0];
			last_face.v[1] = face.v[1];
			last_face.v[2] = face.v[2];
		}
	}
}

static VertexIndex unique_vertex_from_rhs(const StripFace& lhs, const StripFace& rhs) {
	for(VertexIndex vertex : rhs.v) {
		if(vertex != lhs.v[0] && vertex != lhs.v[1] && vertex != lhs.v[2]) {
			return vertex;
		}
	}
	return NULL_VERTEX_INDEX;
}

static std::pair<VertexIndex, VertexIndex> get_shared_vertices(const StripFace& lhs, const StripFace& rhs) {
	VertexIndex first = NULL_VERTEX_INDEX;
	for(VertexIndex vertex : rhs.v) {
		if(vertex == lhs.v[0] || vertex == lhs.v[1] || vertex == lhs.v[2]) {
			if(first == -1) {
				first = vertex;
			} else {
				return {first, vertex};
			}
		}
	}
	return {first, NULL_VERTEX_INDEX};
}

static void verify_face_strips(const std::vector<FaceStrip>& strips, const std::vector<StripFace>& faces, const char* context, const MeshGraph& graph) {
	std::vector<bool> included(graph.face_count(), false);
	for(const FaceStrip& strip : strips) {
		for(s32 i = 0; i < strip.face_count; i++) {
			const StripFace& face = faces[strip.face_begin + i];
			if(!face.is_zero_area()) {
				FaceIndex face_index = graph.face_really_expensive(face.v[0], face.v[1], face.v[2]);
				verify(face_index != NULL_FACE_INDEX, "Broken face strip generated by %s. Bad face(s).", context);
				included[face_index.index] = true;
			}
		}
	}
	for(bool check : included) {
		verify(check, "Broken face strip generated by %s. Missing face(s).", context);
	}
}
