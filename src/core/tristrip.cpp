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
static FaceIndex find_start_face(const MeshGraph& graph, const EffectiveMaterial& effective, FaceIndex next_faces[3]);
static void weave_strip(FaceStrips& dest, FaceIndex start_face, EdgeIndex start_edge, bool to_v1, MeshGraph& graph, const EffectiveMaterial& effective);
static FaceStrip weave_strip_in_one_direction(FaceStrips& dest, FaceIndex start_face, VertexIndex v1, VertexIndex v2, MeshGraph& graph, const EffectiveMaterial& effective);
static FaceStripPackets generate_packets(const FaceStrips& strips, const std::vector<Material>& materials, const std::vector<EffectiveMaterial>& effectives, const TriStripConfig& config);
static GeometryPackets facestrips_to_tripstrips(const FaceStripPackets& input, const std::vector<EffectiveMaterial>& effectives);
static void facestrip_to_tristrip(GeometryPackets& output, const FaceStrip& face_strip, const std::vector<StripFace>& faces);
static VertexIndex unique_vertex_from_rhs(const StripFace& lhs, const StripFace& rhs);
static std::pair<VertexIndex, VertexIndex> get_shared_vertices(const StripFace& lhs, const StripFace& rhs);
static void verify_face_strips(const std::vector<FaceStrip>& strips, const std::vector<StripFace>& faces, const char* context, const MeshGraph& graph);

GeometryPackets weave_tristrips(const Mesh& mesh, const std::vector<Material>& materials, const TriStripConfig& config) {
	// Firstly we build a graph structure to make finding adjacent faces fast.
	MeshGraph graph(mesh);
	std::vector<EffectiveMaterial> effectives = effective_materials(materials, MATERIAL_ATTRIB_SURFACE | MATERIAL_ATTRIB_WRAP_MODE);
	FaceStrips strips;
	for(s32 i = 0; i < (s32) effectives.size(); i++) {
		const EffectiveMaterial& effective = effectives[i];
		for(;;) {
			FaceStrip strip = weave_multiple_strips_and_pick_the_best(strips, graph, effective);
			if(strip.face_count == 0) {
				break;
			}
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
	FaceIndex next_faces[4] = {0, 0, 0, 0};
	FaceIndex last_start_face = NULL_FACE_INDEX;
	for(s32 i = 0; i < 20; i++) {
		FaceIndex start_face = find_start_face(graph, effective, next_faces);
		if(start_face == NULL_FACE_INDEX) {
			return {};
		}
		if(start_face == last_start_face) {
			break;
		}
		last_start_face = start_face;
		weave_strip(temp, start_face, graph.edge_of_face(start_face, 0), false, graph, effective);
		weave_strip(temp, start_face, graph.edge_of_face(start_face, 0), true, graph, effective);
		weave_strip(temp, start_face, graph.edge_of_face(start_face, 1), false, graph, effective);
		weave_strip(temp, start_face, graph.edge_of_face(start_face, 1), true, graph, effective);
		weave_strip(temp, start_face, graph.edge_of_face(start_face, 2), false, graph, effective);
		weave_strip(temp, start_face, graph.edge_of_face(start_face, 2), true, graph, effective);
	}
	
	// Determine which is the best.
	s32 best_strip = -1;
	f32 best_utility = -1000000000;
	for(size_t i = 0; i < temp.strips.size(); i++) {
		FaceStrip& candidate = temp.strips[i];
		f32 utility = candidate.face_count - candidate.zero_area_tri_count * 2.5f;
		if(utility > best_utility) {
			best_strip = i;
			best_utility = utility;
		}
	}
	
	assert(best_strip != -1000000000);
	
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

static FaceIndex find_start_face(const MeshGraph& graph, const EffectiveMaterial& effective, FaceIndex next_faces[4]) {
	// First try individual triangles connected to zero other valid triangles,
	// the one, then two, then three other valid triangles.
	for(s32 i = 0; i <= 3; i++) {
		FaceIndex face = next_faces[i];
		do {
			s32 neighbour_count = 0;
			for(s32 j = 0; j < 3; j++) {
				FaceIndex other_face = graph.other_face(graph.edge_of_face(face, j), face);
				if(other_face != NULL_FACE_INDEX && graph.can_be_added_to_strip(other_face, effective)) {
					neighbour_count++;
				}
			}
			if(neighbour_count == i && graph.can_be_added_to_strip(face, effective)) {
				next_faces[i] = (face.index + 1) % graph.face_count();
				return face;
			}
			face.index = (face.index + 1) % graph.face_count();
		} while(face != next_faces[i]);
	}
	return NULL_FACE_INDEX;
}

static void weave_strip(FaceStrips& dest, FaceIndex start_face, EdgeIndex start_edge, bool to_v1, MeshGraph& graph, const EffectiveMaterial& effective) {
	FaceStrip& strip = dest.strips.emplace_back();
	strip.face_count = 0;
	strip.face_begin = (s32) dest.faces.size();
	
	VertexIndex v0 = to_v1 ? graph.edge_vertex(start_edge, 0) : graph.edge_vertex(start_edge, 1);
	VertexIndex v1 = to_v1 ? graph.edge_vertex(start_edge, 1) : graph.edge_vertex(start_edge, 0);
	
	VertexIndex v2 = graph.next_index(v0, v1, start_face);
	if(v2 == NULL_VERTEX_INDEX) {
		printf("warning: Tristrip weaving failed. Failed to find v2.\n");
		return;
	}
	
	// Do this up here so we don't accidentally add the start face twice.
	graph.put_in_temp_strip(start_face);
	
	// Stores the separate forward and backward strips.
	FaceStrips temp;
	
	// Weave the strip forwards.
	FaceStrip forward = weave_strip_in_one_direction(temp, start_face, v1, v2, graph, effective);
	
	// Weave the strip backwards.
	FaceStrip backward = weave_strip_in_one_direction(temp, start_face, v1, v0, graph, effective);
	
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

static FaceStrip weave_strip_in_one_direction(FaceStrips& dest, FaceIndex start_face, VertexIndex v1, VertexIndex v2, MeshGraph& graph, const EffectiveMaterial& effective) {
	FaceStrip strip;
	strip.face_begin = (s32) dest.faces.size();
	strip.face_count = 0;
	
	VertexIndex v0 = NULL_VERTEX_INDEX;
	FaceIndex f0 = start_face;
	
	for(;;) {
		FaceIndex f1 = graph.other_face(graph.edge(v1, v2), f0);
		
		if(f1 == NULL_FACE_INDEX || !graph.can_be_added_to_strip(f1, effective)) {
			// Consider swapping, but only if it helps us.
			//
			// Preconditions: f0 already added to strip, can't find f1.
			// Postconditions: Swap added, variables reassigned as shown below.
			//
			// ->+------v1            +------v1
			//   |    /  |            |    /  |
			//   v   >   v            v   x   |
			//   |  / f0 |    SWAP    |  / f1 |
			//   v0-----v2  ------->  v2-->--v3
			//   | f2 /  |            |    /  |
			//   |   /   |            |   v   |
			//   |  / f3 |            |  /    |
			//   v4------+            +--->---+
			if(v0 == NULL_VERTEX_INDEX) {
				break;
			}
			FaceIndex f2 = graph.other_face(graph.edge(v0, v2), f1);
			if(f2 == NULL_FACE_INDEX || !graph.can_be_added_to_strip(f2, effective)) {
				break;
			} 
			VertexIndex v4 = graph.next_index(v0, v2, f2);
			FaceIndex f3 = graph.other_face(graph.edge(v2, v4), f2);
			if(f3 == NULL_FACE_INDEX || !graph.can_be_added_to_strip(f3, effective)) {
				break;
			}
			
			// Remove v2, add v0.
			dest.faces.back() = StripFace(v0, v1, v0, NULL_FACE_INDEX);
			strip.zero_area_tri_count++;
			
			v2 = v0;
			v0 = NULL_VERTEX_INDEX;
			f1 = f0;
			
			// Fall through so we work out the new v3 and add f0/f1 again.
		}
		
		VertexIndex v3 = graph.next_index(v1, v2, f1);
		if(v3 == NULL_VERTEX_INDEX) {
			printf("warning: Tristrip weaving failed. Failed to find v3.\n");
			return strip;
		}
		
		strip.face_count++;
		assert(f1 != NULL_FACE_INDEX);
		dest.faces.emplace_back(v1, v2, v3, f1);
		graph.put_in_temp_strip(f1);
		
		v0 = v1;
		v1 = v2;
		v2 = v3;
		f0 = f1;
	}
	
	return strip;
}

static FaceStripPackets generate_packets(const FaceStrips& strips, const std::vector<Material>& materials, const std::vector<EffectiveMaterial>& effectives, const TriStripConfig& config) {
	TriStripPacketGenerator generator(materials, effectives, config.constraints, config.support_instancing);
	s32 first_strip_with_material = 0;
	for(size_t i = 0; i < strips.strips.size(); i++) {
		const FaceStrip& strip = strips.strips[i];
		if(strip.face_count > 1) {
			generator.add_strip(&strips.faces[strip.face_begin], strip.face_count, strip.effective_material);
		}
		// Batch single triangles together into lists instead of strips.
		if(i == strips.strips.size() - 1 || strips.strips[i + 1].effective_material != strip.effective_material ) {
			std::vector<VertexIndex> indices;
			for(size_t j = first_strip_with_material; j <= i; j++) {
				const FaceStrip& other_strip = strips.strips[j];
				if(other_strip.face_count == 1) {
					indices.emplace_back(strips.faces[other_strip.face_begin].v[0]);
					indices.emplace_back(strips.faces[other_strip.face_begin].v[1]);
					indices.emplace_back(strips.faces[other_strip.face_begin].v[2]);
				}
			}
			if(indices.size() > 0) {
				generator.add_list(indices.data(), indices.size() / 3, strip.effective_material);
			}
			first_strip_with_material = i + 1;
		}
	}
	return generator.get_output();
}

static GeometryPackets facestrips_to_tripstrips(const FaceStripPackets& input, const std::vector<EffectiveMaterial>& effectives) {
	GeometryPackets output;
	
	for(const FaceStripPacket& face_packet : input.packets) {
		GeometryPacket& tri_packet = output.packets.emplace_back();
		tri_packet.primitive_begin = face_packet.strip_begin;
		tri_packet.primitive_count = face_packet.strip_count;
		for(s32 i = 0; i < face_packet.strip_count; i++) {
			GeometryPrimitive& dest_primitive = output.primitives.emplace_back();
			const FaceStrip& src_primitive = input.strips[face_packet.strip_begin + i];
			dest_primitive.type = src_primitive.type;
			dest_primitive.index_begin = output.indices.size();
			if(src_primitive.effective_material >= 0) {
				dest_primitive.material = effectives[src_primitive.effective_material].materials.at(0);
			} else {
				dest_primitive.material = -1;
			}
			assert(src_primitive.face_count >= 1);
			if(src_primitive.type == GeometryType::TRIANGLE_LIST) {
				dest_primitive.index_count = src_primitive.face_count * 3;
				for(s32 i = 0; i < src_primitive.face_count; i++) {
					output.indices.emplace_back(input.faces[src_primitive.face_begin + i].v[0].index);
					output.indices.emplace_back(input.faces[src_primitive.face_begin + i].v[1].index);
					output.indices.emplace_back(input.faces[src_primitive.face_begin + i].v[2].index);
				}
			} else {
				dest_primitive.index_count = 2 + src_primitive.face_count;
				facestrip_to_tristrip(output, src_primitive, input.faces);
			}
		}
	}
	
	return output;
}

static void facestrip_to_tristrip(GeometryPackets& output, const FaceStrip& face_strip, const std::vector<StripFace>& faces) {
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
