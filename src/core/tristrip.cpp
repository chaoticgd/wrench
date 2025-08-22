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

// Code to generate triangle strips.
//
// Some of the algorithms here were adapted from the NvTriStrip library.

struct FaceStrip
{
	GeometryType type;
	s32 face_begin = 0;
	s32 face_count = 0;
	s32 effective_material = 0;
	s32 zero_area_tri_count = 0;
};

// This is used where a list of faces may contain a zero area triangle i.e. one
// that isn't included in the original mesh but is inserted to construct a
// triangle strip.
struct StripFace
{
	StripFace() {}
	StripFace(VertexIndex v0, VertexIndex v1, VertexIndex v2, FaceIndex i)
		: v{v0, v1, v2}, index(i) {}
	VertexIndex v[3];
	FaceIndex index = NULL_FACE_INDEX;
	bool is_zero_area() const { return v[0] == v[1] || v[0] == v[2] || v[1] == v[2]; }
};

struct FaceStrips
{
	std::vector<FaceStrip> strips;
	std::vector<StripFace> faces;
};

struct FaceStripPacket
{
	s32 strip_begin = 0;
	s32 strip_count = 0;
};

struct FaceStripPackets
{
	std::vector<FaceStripPacket> packets;
	std::vector<FaceStrip> strips;
	std::vector<StripFace> faces;
};

static FaceStrip weave_multiple_strips_and_pick_the_best(
	FaceStrips& dest, MeshGraph& graph, const EffectiveMaterial& effective);
static FaceIndex find_start_face(
	const MeshGraph& graph, const EffectiveMaterial& effective, FaceIndex next_faces[4]);
static void weave_strip(
	FaceStrips& dest,
	FaceIndex start_face,
	EdgeIndex start_edge,
	bool to_v1,
	MeshGraph& graph,
	const EffectiveMaterial& effective);
static FaceStrip weave_strip_in_one_direction(
	FaceStrips& dest,
	FaceIndex start_face,
	VertexIndex v1,
	VertexIndex v2,
	MeshGraph& graph,
	const EffectiveMaterial& effective);
static GeometryPrimitives facestrips_to_tristrips(
	const FaceStrips& input, const std::vector<EffectiveMaterial>& effectives);
static void facestrip_to_tristrip(
	std::vector<s32>& output_indices, const FaceStrip& face_strip, const std::vector<StripFace>& faces);
static void batch_single_triangles_together(GeometryPrimitives& primitives);
static VertexIndex unique_vertex_from_rhs(const StripFace& lhs, const StripFace& rhs);
static std::pair<VertexIndex, VertexIndex> get_shared_vertices(
	const StripFace& lhs, const StripFace& rhs);
static void verify_face_strips(const std::vector<FaceStrip>& strips, const std::vector<StripFace>& faces, const char* context, const MeshGraph& graph);

GeometryPrimitives weave_tristrips(const GLTF::Mesh& mesh, const std::vector<EffectiveMaterial>& effectives)
{
	// Firstly we build a graph structure to make finding adjacent faces fast.
	MeshGraph graph(mesh);
	if (graph.face_count() == 0) {
		return {};
	}
	FaceStrips face_strips;
	for (s32 i = 0; i < (s32) effectives.size(); i++) {
		const EffectiveMaterial& effective = effectives[i];
		for (;;) {
			FaceStrip strip = weave_multiple_strips_and_pick_the_best(face_strips, graph, effective);
			if (strip.face_count == 0) {
				break;
			}
			strip.effective_material = i;
			for (s32 i = 0; i < strip.face_count; i++) {
				StripFace& face = face_strips.faces[strip.face_begin + i];
				if (face.index != NULL_FACE_INDEX) {
					graph.put_in_strip(face.index, (s32) face_strips.strips.size());
				}
			}
			face_strips.strips.emplace_back(strip);
		}
	}
	verify_face_strips(face_strips.strips, face_strips.faces, "weave_multiple_strips_and_pick_the_best", graph);
	GeometryPrimitives tri_strips = facestrips_to_tristrips(face_strips, effectives);
	//batch_single_triangles_together(tri_strips);
	return tri_strips;
}

std::vector<s32> zero_area_tris_to_restart_bit_strip(const std::vector<s32>& indices)
{
	std::vector<s32> output;
	bool set_next_restart_bit = false;
	for (size_t i = 0; i < indices.size(); i++) {
		if (i <= 0 || indices[i - 1] != indices[i]) {
			if (set_next_restart_bit) {
				output.emplace_back(indices[i] | (1 << 31));
				set_next_restart_bit = false;
			} else {
				output.emplace_back(indices[i]);
			}
		} else {
			set_next_restart_bit = true;
		}
	}
	return output;
}

std::vector<s32> restart_bit_strip_to_zero_area_tris(const std::vector<s32>& indices)
{
	std::vector<s32> output;
	for (size_t i = 0; i < indices.size(); i++) {
		if (i > 0 && indices[i] < 0) {
			output.emplace_back(output.back());
		}
		output.emplace_back(indices[i] & ~(1 << 31));
	}
	return output;
}

static FaceStrip weave_multiple_strips_and_pick_the_best(
	FaceStrips& dest, MeshGraph& graph, const EffectiveMaterial& effective)
{
	// Weave multiple candidate strips.
	FaceStrips temp;
	FaceIndex next_faces[4] = {0, 0, 0, 0};
	FaceIndex last_start_face = NULL_FACE_INDEX;
	for (s32 i = 0; i < 10; i++) {
		FaceIndex start_face = find_start_face(graph, effective, next_faces);
		if (start_face == NULL_FACE_INDEX) {
			return {};
		}
		if (start_face == last_start_face) {
			break;
		}
		
		// Edge with more than two faces, this should be included as a single
		// triangle so it doesn't cause problems.
		if (graph.face_is_evil(start_face)) {
			FaceStrip strip;
			strip.face_begin = (s32) dest.faces.size();
			strip.face_count = 1;
			StripFace& face = dest.faces.emplace_back();
			face.v[0] = graph.face_vertex(start_face, 0);
			face.v[1] = graph.face_vertex(start_face, 1);
			face.v[2] = graph.face_vertex(start_face, 2);
			face.index = start_face;
			return strip;
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
	for (size_t i = 0; i < temp.strips.size(); i++) {
		FaceStrip& candidate = temp.strips[i];
		f32 utility = candidate.face_count - candidate.zero_area_tri_count * 2.5f;
		if (utility > best_utility) {
			best_strip = i;
			best_utility = utility;
		}
	}
	
	verify_fatal(best_strip != -1000000000);
	
	// Copy the best strip from the temp array to the main array.
	FaceStrip strip;
	strip.type = GeometryType::TRIANGLE_STRIP;
	strip.face_begin = (s32) dest.faces.size();
	strip.face_count = temp.strips[best_strip].face_count;
	strip.zero_area_tri_count = temp.strips[best_strip].zero_area_tri_count;
	for (s32 i = 0; i < temp.strips[best_strip].face_count; i++) {
		dest.faces.emplace_back(temp.faces[temp.strips[best_strip].face_begin + i]);
	}
	return strip;
}

static FaceIndex find_start_face(
	const MeshGraph& graph, const EffectiveMaterial& effective, FaceIndex next_faces[4])
{
	// First try individual triangles connected to zero other valid triangles,
	// the one, then two, then three other valid triangles.
	for (s32 i = 0; i <= 3; i++) {
		FaceIndex face = next_faces[i];
		do {
			s32 neighbour_count = 0;
			for (s32 j = 0; j < 3; j++) {
				EdgeIndex edge = graph.edge_of_face(face, j);
				verify_fatal(edge != NULL_EDGE_INDEX);
				FaceIndex other_face = graph.other_face(edge, face);
				if (other_face != NULL_FACE_INDEX && graph.can_be_added_to_strip(other_face, effective)) {
					neighbour_count++;
				}
			}
			if (neighbour_count == i && graph.can_be_added_to_strip(face, effective)) {
				next_faces[i] = (face.index + 1) % graph.face_count();
				return face;
			}
			face.index = (face.index + 1) % graph.face_count();
		} while (face != next_faces[i]);
	}
	return NULL_FACE_INDEX;
}

static void weave_strip(
	FaceStrips& dest,
	FaceIndex start_face,
	EdgeIndex start_edge,
	bool to_v1,
	MeshGraph& graph,
	const EffectiveMaterial& effective)
{
	FaceStrip& strip = dest.strips.emplace_back();
	strip.face_count = 0;
	strip.face_begin = (s32) dest.faces.size();
	
	VertexIndex v0 = to_v1 ? graph.edge_vertex(start_edge, 0) : graph.edge_vertex(start_edge, 1);
	VertexIndex v1 = to_v1 ? graph.edge_vertex(start_edge, 1) : graph.edge_vertex(start_edge, 0);
	
	VertexIndex v2 = graph.next_index(v0, v1, start_face);
	if (v2 == NULL_VERTEX_INDEX) {
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
	for (s32 i = 0; i < backward.face_count; i++) {
		dest.faces.emplace_back(temp.faces[backward.face_begin + (backward.face_count - i - 1)]);
	}
	dest.faces.emplace_back(v0, v1, v2, start_face);
	for (s32 i = 0; i < forward.face_count; i++) {
		dest.faces.emplace_back(temp.faces[forward.face_begin + i]);
	}
	
	// Make it so this strip no longer registers as already part of a strip
	// (since it might be discarded in favour of a better strip).
	graph.discard_temp_strip();
}

static FaceStrip weave_strip_in_one_direction(
	FaceStrips& dest,
	FaceIndex start_face,
	VertexIndex v1,
	VertexIndex v2,
	MeshGraph& graph,
	const EffectiveMaterial& effective) {
	FaceStrip strip;
	strip.face_begin = (s32) dest.faces.size();
	strip.face_count = 0;
	
	VertexIndex v0 = NULL_VERTEX_INDEX;
	FaceIndex f0 = start_face;
	
	for (;;) {
		FaceIndex f1 = graph.other_face(graph.edge(v1, v2), f0);
		
		if (f1 == NULL_FACE_INDEX || !graph.can_be_added_to_strip(f1, effective)) {
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
			//   |  /    |            |  /    |
			//   +-------+            +--->---+
			if (v0 == NULL_VERTEX_INDEX) {
				break;
			}
			FaceIndex f2 = graph.other_face(graph.edge(v0, v2), f0);
			if (f2 == NULL_FACE_INDEX || !graph.can_be_added_to_strip(f2, effective)) {
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
		if (v3 == NULL_VERTEX_INDEX) {
			printf("warning: Tristrip weaving failed. Failed to find v3.\n");
			return strip;
		}
		
		strip.face_count++;
		verify_fatal(f1 != NULL_FACE_INDEX);
		dest.faces.emplace_back(v1, v2, v3, f1);
		graph.put_in_temp_strip(f1);
		
		v0 = v1;
		v1 = v2;
		v2 = v3;
		f0 = f1;
	}
	
	return strip;
}

static GeometryPrimitives facestrips_to_tristrips(
	const FaceStrips& input, const std::vector<EffectiveMaterial>& effectives)
{
	GeometryPrimitives output;
	
	for (const FaceStrip& src_primitive : input.strips) {
		GeometryPrimitive& dest_primitive = output.primitives.emplace_back();
		dest_primitive.effective_material = src_primitive.effective_material;
		verify_fatal(src_primitive.face_count >= 1);
		switch (src_primitive.type) {
			case GeometryType::TRIANGLE_LIST: {
				dest_primitive.type = GeometryType::TRIANGLE_LIST;
				dest_primitive.index_begin = (s32) output.indices.size();
				dest_primitive.index_count = src_primitive.face_count * 3;
				for (s32 i = 0; i < src_primitive.face_count; i++) {
					output.indices.emplace_back(input.faces[src_primitive.face_begin + i].v[0].index);
					output.indices.emplace_back(input.faces[src_primitive.face_begin + i].v[1].index);
					output.indices.emplace_back(input.faces[src_primitive.face_begin + i].v[2].index);
				}
				break;
			}
			case GeometryType::TRIANGLE_STRIP: {
					dest_primitive.type = GeometryType::TRIANGLE_STRIP;
				dest_primitive.index_begin = (s32) output.indices.size();
				dest_primitive.index_count = src_primitive.face_count + 2;
				facestrip_to_tristrip(output.indices, src_primitive, input.faces);
				break;
			}
			case GeometryType::TRIANGLE_FAN: {
				verify_not_reached_fatal("Conversion of face fans to triangle fans not implemented.");
				break;
			}
		}
	}
	
	return output;
}

static void facestrip_to_tristrip(
	std::vector<s32>& output_indices, const FaceStrip& face_strip, const std::vector<StripFace>& faces)
{
	if (face_strip.face_count == 0) {
		return;
	}
	
	// Process the first face.
	StripFace first_face = faces[face_strip.face_begin];
	if (face_strip.face_count >= 2) {
		// Reorder the vertices of the first face such that a strip
		// can be more easily constructed.
		StripFace second_face = faces[face_strip.face_begin + 1];
		VertexIndex unique = unique_vertex_from_rhs(second_face, first_face);
		if (unique == first_face.v[1]) {
			std::swap(first_face.v[0], first_face.v[1]);
		} else if (unique == first_face.v[2]) {
			std::swap(first_face.v[0], first_face.v[2]);
		}
		if (face_strip.face_count >= 3) {
			// Same thing, but with the third face.
			StripFace third_face = faces[face_strip.face_begin + 2];
			if (second_face.is_zero_area()) {
				VertexIndex pivot = second_face.v[1];
				if (first_face.v[1] == pivot) {
					std::swap(first_face.v[1], first_face.v[2]);
				}
			} else {
				auto [shared_0, shared_1] = get_shared_vertices(third_face, first_face);
				if (shared_0 == first_face.v[1] && shared_1 == -1) {
					std::swap(first_face.v[1], first_face.v[2]);
				}
			}
		}
	}
	
	// Now actually add the first face.
	verify_fatal(first_face.v[0] != NULL_VERTEX_INDEX);
	verify_fatal(first_face.v[1] != NULL_VERTEX_INDEX);
	verify_fatal(first_face.v[2] != NULL_VERTEX_INDEX);
	output_indices.emplace_back(first_face.v[0].index);
	output_indices.emplace_back(first_face.v[1].index);
	output_indices.emplace_back(first_face.v[2].index);
	
	// Process the rest of the faces.
	StripFace last_face = first_face;
	for (s32 j = 1; j < face_strip.face_count; j++) {
		const StripFace& face = faces[face_strip.face_begin + j];
		VertexIndex unique = unique_vertex_from_rhs(last_face, face);
		if (unique != NULL_VERTEX_INDEX) {
			verify_fatal(unique != NULL_VERTEX_INDEX);
			output_indices.emplace_back(unique.index);
			last_face.v[0] = last_face.v[1];
			last_face.v[1] = last_face.v[2];
			last_face.v[2] = unique;
		} else {
			// Insert a zero area triangle.
			verify_fatal(face.v[2] != NULL_VERTEX_INDEX);
			output_indices.emplace_back(face.v[2].index);
			last_face.v[0] = face.v[0];
			last_face.v[1] = face.v[1];
			last_face.v[2] = face.v[2];
		}
	}
}

static void batch_single_triangles_together(GeometryPrimitives& primitives)
{
	// For each effective material.
	size_t start_of_group = 0;
	for (size_t i = 0; i < primitives.primitives.size(); i++) {
		s32 effective_material = primitives.primitives[i].effective_material;
		if (i == primitives.primitives.size() - 1 || primitives.primitives[i + 1].effective_material != effective_material) {
			// Find a primitive to stuff the batched triangle list in.
			size_t first_single_tri = SIZE_MAX;
			for (size_t j = start_of_group; j <= i; j++) {
				if (primitives.primitives[j].index_count == 3) {
					first_single_tri = j;
					break;
				}
			}
			// Do the stuffing.
			if (first_single_tri != SIZE_MAX) {
				GeometryPrimitive& first_prim = primitives.primitives[first_single_tri];
				s32 index_begin = (s32) primitives.indices.size();
				s32 index_count = 0;
				for (size_t j = start_of_group; j <= i; j++) {
					GeometryPrimitive& prim = primitives.primitives[j];
					if (prim.index_count == 3) {
						index_count += 3;
						primitives.indices.emplace_back(primitives.indices[prim.index_begin + 0]);
						primitives.indices.emplace_back(primitives.indices[prim.index_begin + 1]);
						primitives.indices.emplace_back(primitives.indices[prim.index_begin + 2]);
						prim.index_count = 0;
					}
				}
				first_prim.type = GeometryType::TRIANGLE_LIST;
				first_prim.index_begin = index_begin;
				first_prim.index_count = index_count;
			}
			start_of_group = i + 1;
			break;
		}
	}
}

static VertexIndex unique_vertex_from_rhs(const StripFace& lhs, const StripFace& rhs)
{
	for (VertexIndex vertex : rhs.v) {
		if (vertex != lhs.v[0] && vertex != lhs.v[1] && vertex != lhs.v[2]) {
			return vertex;
		}
	}
	return NULL_VERTEX_INDEX;
}

static std::pair<VertexIndex, VertexIndex> get_shared_vertices(
	const StripFace& lhs, const StripFace& rhs)
{
	VertexIndex first = NULL_VERTEX_INDEX;
	for (VertexIndex vertex : rhs.v) {
		if (vertex == lhs.v[0] || vertex == lhs.v[1] || vertex == lhs.v[2]) {
			if (first == -1) {
				first = vertex;
			} else {
				return {first, vertex};
			}
		}
	}
	return {first, NULL_VERTEX_INDEX};
}

static void verify_face_strips(
	const std::vector<FaceStrip>& strips,
	const std::vector<StripFace>& faces,
	const char* context,
	const MeshGraph& graph)
{
	std::vector<bool> included(graph.face_count(), false);
	for (const FaceStrip& strip : strips) {
		for (s32 i = 0; i < strip.face_count; i++) {
			const StripFace& face = faces[strip.face_begin + i];
			if (!face.is_zero_area()) {
				std::vector<FaceIndex> face_indices = graph.faces_really_expensive(face.v[0], face.v[1], face.v[2]);
				verify(!face_indices.empty(), "Broken strip generated by %s. Bad face(s).", context);
				for (FaceIndex face_index : face_indices) {
					included[face_index.index] = true;
				}
			}
		}
	}
	for (FaceIndex i = 0; i < graph.face_count(); i.index++) {
		verify(included[i.index], "Broken strip generated by %s. Missing %sface (%d).",
			context, graph.face_is_evil(i) ? "evil " : "", i.index);
	}
}
