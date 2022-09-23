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

// Code to generate triangle strips and split said strips into packets based on
// a set of size constraints.
//
// Some of the algorithms here were adapted from the NvTriStrip library.

struct FaceStrip {
	s32 face_begin;
	s32 face_count;
	s32 texture;
};

struct FaceStrips {
	std::vector<FaceStrip> strips;
	std::vector<s32> faces;
};

struct FaceStripPacket {
	s32 strip_begin;
	s32 strip_count;
};

struct FaceStripPackets {
	std::vector<FaceStripPacket> packets;
	std::vector<FaceStrip> strips;
	std::vector<s32> faces;
};

struct TriStripRunningTotals {
	s32 strip_count = 0;
	s32 vertex_count = 0;
	s32 index_count = 0;
	s32 texture_count = 0;
};

static void compute_connectivity_information(TriStripFace* faces, s32 face_count);
static void weave_tristrip(FaceStrips& dest, TriStripFace* faces, s32 face_count);
static FaceStripPackets generate_packets(FaceStrips& input, const TriStripConstraints& constraints, bool support_instancing);
static TriStripPackets facestrips_to_tripstrips(const FaceStripPackets& input, const TriStripFace* faces, s32 face_count);
static void facestrip_to_tristrip(TriStripPackets& output, const FaceStrip& face_strip, const TriStripFace* faces, s32 face_count) ;
static bool try_add_strip(TriStripRunningTotals& totals, const TriStripConstraints& constraints);
static bool try_add_unique_vertex(TriStripRunningTotals& totals, const TriStripConstraints& constraints);
static bool try_add_duplicate_vertex(TriStripRunningTotals& totals, const TriStripConstraints& constraints);
static bool try_add_texture(TriStripRunningTotals& totals, const TriStripConstraints& constraints);
static s32 unique_vertex_from_rhs(const TriStripFace& lhs, const TriStripFace& rhs);
static std::pair<s32, s32> get_shared_vertices(const TriStripFace& lhs, const TriStripFace& rhs);
static bool is_zero_area(const TriStripFace& face);

TriStripPackets weave_tristrips(TriStripFace* faces, s32 face_count, const TriStripConstraints& constraints, bool support_instancing) {
	// Firstly for each face, we determine which other faces are directly connected to it by an edge.
	compute_connectivity_information(faces, face_count);
	// Secondly we generate the strips of faces that when combined cover the mesh.
	FaceStrips strips;
	for(;;) {
		weave_tristrip(strips, faces, face_count);
		bool done = true;
		for(s32 i = 0; i < face_count; i++) {
			if(!faces[i].included) {
				done = false;
			}
		}
		if(done) {
			break;
		}
	}
	// Thirdly we split those strips up between different packets.
	FaceStripPackets packets = generate_packets(strips, constraints, support_instancing);
	// Fourthly we convert those strips of faces to tristrips.
	return facestrips_to_tripstrips(packets, faces, face_count);
}

struct Edge {
	Edge(s32 v0, s32 v1) : v{std::min(v0, v1), std::max(v0, v1)} {}
	bool operator==(const Edge& rhs) const { return v[0] == rhs.v[0] && v[1] == rhs.v[1]; }
	bool operator<(const Edge& rhs) const { if(v[0] != rhs.v[0]) return v[0] < rhs.v[0]; else return v[1] < rhs.v[1]; }
	s32 v[2];
};

struct AdjacentFaces {
	s32 count = 0;
	s32 faces[2];
};

static void compute_connectivity_information(TriStripFace* faces, s32 face_count) {
	std::map<Edge, AdjacentFaces> edge_to_faces;
	
	// Build a map of all the faces connected to each edge.
	for(s32 i = 0; i < face_count; i++) {
		TriStripFace& face = faces[i];
		for(s32 i = 0; i < 3; i++) {
			Edge edge(face.v[i], face.v[(i + 1) % 3]);
			AdjacentFaces& adjacent = edge_to_faces[edge];
			if(adjacent.count < 2) {
				adjacent.faces[adjacent.count++] = i;
			}
		}
	}
	
	// Determine for each face, which other faces share an edge with it.
	for(auto& [edge, adjacent] : edge_to_faces) {
		if(adjacent.count == 2) {
			for(s32 i = 0; i < 2; i++) {
				TriStripFace& face = faces[adjacent.faces[i]];
				if(face.neighbour_count < 3) {
					face.neighbours[face.neighbour_count] = adjacent.faces[1 - i];
					face.neighbour_count++;
				}
			}
		}
	}
}

static void weave_tristrip(FaceStrips& dest, TriStripFace* faces, s32 face_count) {
	if(face_count == 0) {
		return;
	}
	
	for(s32 i = 0; i < face_count; i++) {
		// Do the very naive thing for now.
		FaceStrip& strip = dest.strips.emplace_back();
		strip.face_begin = (s32) dest.faces.size();
		strip.face_count = 1;
		strip.texture = faces[i].texture;
		dest.faces.emplace_back(i);
		faces[i].included = true;
	}
}

static FaceStripPackets generate_packets(FaceStrips& input, const TriStripConstraints& constraints, bool support_instancing) {
	FaceStripPackets output;
	
	TriStripRunningTotals totals;
	FaceStripPacket* packet;
	s32 texture = -1;
	
	auto new_packet = [&]() {
		totals = {};
		packet = &output.packets.emplace_back();
		packet->strip_begin = output.strips.size();
		packet->strip_count = 0;
		if(support_instancing) {
			texture = -1;
		}
	};
	
	new_packet();
	
	for(size_t i = 0; i < input.strips.size(); i++) {
		FaceStrip& src_strip = input.strips[i];
		
		if(src_strip.face_count == 0) {
			continue;
		}
		
		if(texture != src_strip.texture) {
			if(!try_add_texture(totals, constraints)) {
				new_packet();
				i--;
				continue;
			}
			texture = src_strip.texture;
		}
		
		if(!try_add_strip(totals, constraints)) {
			new_packet();
			i--;
			continue;
		}
		
		// See if we can add the first face.
		bool valid = true;
		valid &= try_add_unique_vertex(totals, constraints);
		valid &= try_add_unique_vertex(totals, constraints);
		valid &= try_add_unique_vertex(totals, constraints);
		if(!valid) {
			new_packet();
			i--;
			continue;
		}
		
		// Setup the strip.
		packet->strip_count++;
		FaceStrip& dest_strip = output.strips.emplace_back();
		dest_strip.face_begin = (s32) output.faces.size();
		dest_strip.face_count = 0;
		dest_strip.texture = texture;
		
		// Add the first face.
		dest_strip.face_count++;
		output.faces.emplace_back(input.faces[src_strip.face_begin]);
		
		// Add as many of the remaining faces as possible.
		for(s32 j = 1; j < src_strip.face_count; j++) {
			if(!try_add_unique_vertex(totals, constraints)) {
				new_packet();
				i--;
				s32 old_face_begin = src_strip.face_begin;
				src_strip.face_begin = src_strip.face_begin + j;
				src_strip.face_count -= src_strip.face_begin - old_face_begin;
				continue;
			}
			
			dest_strip.face_count++;
			output.faces.emplace_back(input.faces[src_strip.face_begin + j]);
		}
	}
	
	return output;
}

static TriStripPackets facestrips_to_tripstrips(const FaceStripPackets& input, const TriStripFace* faces, s32 face_count) {
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
			assert(face_strip.face_count >= 1);
			
			facestrip_to_tristrip(output, face_strip, faces, face_count);
		}
	}
	
	return output;
}

static void facestrip_to_tristrip(TriStripPackets& output, const FaceStrip& face_strip, const TriStripFace* faces, s32 face_count) {
	// Process the first face.
	TriStripFace first_face = faces[face_strip.face_begin];
	if(face_strip.face_count >= 2) {
		// Reorder the vertices of the first face such that a strip
		// can be more easily constructed.
		TriStripFace second_face = faces[face_strip.face_begin + 1];
		s32 unique = unique_vertex_from_rhs(second_face, first_face);
		if(unique == first_face.v[1]) {
			std::swap(first_face.v[0], first_face.v[1]);
		} else if(unique == first_face.v[2]) {
			std::swap(first_face.v[0], first_face.v[2]);
		}
		if(face_strip.face_count >= 3) {
			// Same thing, but with the third face.
			TriStripFace third_face = faces[face_strip.face_begin + 2];
			if(is_zero_area(third_face)) {
				s32 pivot = second_face.v[1];
				if(first_face.v[1] == pivot) {
					std::swap(first_face.v[1], first_face.v[2]);
				}
			} else {
				auto [shared_0, shared_1] = get_shared_vertices(second_face, first_face);
				if(shared_0 == first_face.v[1] && shared_1 == -1) {
					std::swap(first_face.v[1], first_face.v[2]);
				}
			}
		}
	}
	
	// Now actually add the first face.
	output.indices.emplace_back(first_face.v[0]);
	output.indices.emplace_back(first_face.v[1]);
	output.indices.emplace_back(first_face.v[2]);
	
	// Process the rest of the faces.
	TriStripFace last_face = first_face;
	for(s32 j = 1; j < face_strip.face_count; j++) {
		const TriStripFace& face = faces[face_strip.face_begin + j - 1];
		s32 unique = unique_vertex_from_rhs(last_face, face);
		if(unique != -1) {
			output.indices.emplace_back(unique);
			last_face.v[0] = last_face.v[1];
			last_face.v[1] = last_face.v[2];
			last_face.v[2] = unique;
		} else {
			output.indices.emplace_back(face.v[2]);
			last_face.v[0] = face.v[0];
			last_face.v[1] = face.v[1];
			last_face.v[2] = face.v[2];
		}
	}
}

static bool try_add_strip(TriStripRunningTotals& totals, const TriStripConstraints& constraints) {
	for(s32 i = 0; i < constraints.num_constraints; i++) {
		s32 cost = 0;
		cost += constraints.constant_cost[i];
		cost += constraints.strip_cost[i] * (totals.strip_count + 1);
		cost += constraints.vertex_cost[i] * totals.vertex_count;
		cost += constraints.index_cost[i] * totals.index_count;
		cost += constraints.texture_cost[i] * totals.texture_count;
		if(cost > constraints.max_cost[i]) {
			return false;
		}
	}
	totals.strip_count++;
	return true;
}

static bool try_add_unique_vertex(TriStripRunningTotals& totals, const TriStripConstraints& constraints) {
	for(s32 i = 0; i < constraints.num_constraints; i++) {
		s32 cost = 0;
		cost += constraints.constant_cost[i];
		cost += constraints.strip_cost[i] * totals.strip_count;
		cost += constraints.vertex_cost[i] * (totals.vertex_count + 1);
		cost += constraints.index_cost[i] * (totals.index_count + 1);
		cost += constraints.texture_cost[i] * totals.texture_count;
		if(cost > constraints.max_cost[i]) {
			return false;
		}
	}
	totals.vertex_count++;
	totals.index_count++;
	return true;
}

static bool try_add_duplicate_vertex(TriStripRunningTotals& totals, const TriStripConstraints& constraints) {
	for(s32 i = 0; i < constraints.num_constraints; i++) {
		s32 cost = 0;
		cost += constraints.constant_cost[i];
		cost += constraints.strip_cost[i] * totals.strip_count;
		cost += constraints.vertex_cost[i] * totals.vertex_count;
		cost += constraints.index_cost[i] * (totals.index_count + 1);
		cost += constraints.texture_cost[i] * totals.texture_count;
		if(cost > constraints.max_cost[i]) {
			return false;
		}
	}
	totals.index_count++;
	return true;
}

static bool try_add_texture(TriStripRunningTotals& totals, const TriStripConstraints& constraints) {
	for(s32 i = 0; i < constraints.num_constraints; i++) {
		s32 cost = 0;
		cost += constraints.constant_cost[i];
		cost += constraints.strip_cost[i] * totals.strip_count;
		cost += constraints.vertex_cost[i] * totals.vertex_count;
		cost += constraints.index_cost[i] * totals.index_count;
		cost += constraints.texture_cost[i] * (totals.texture_count + 1);
		if(cost > constraints.max_cost[i]) {
			return false;
		}
	}
	totals.texture_count++;
	return true;
}

static s32 unique_vertex_from_rhs(const TriStripFace& lhs, const TriStripFace& rhs) {
	for(s32 vertex : rhs.v) {
		if(vertex != lhs.v[0] && vertex != lhs.v[1] && vertex != lhs.v[2]) {
			return vertex;
		}
	}
	return -1;
}

static std::pair<s32, s32> get_shared_vertices(const TriStripFace& lhs, const TriStripFace& rhs) {
	s32 first = -1;
	for(s32 vertex : rhs.v) {
		if(vertex == lhs.v[0] || vertex == lhs.v[1] || vertex == lhs.v[2]) {
			if(first == -1) {
				first = vertex;
			} else {
				return {first, vertex};
			}
		}
	}
	return {first, -1};
}

static bool is_zero_area(const TriStripFace& face) {
	return face.v[0] == face.v[1] || face.v[0] == face.v[2] || face.v[1] == face.v[2];
}
