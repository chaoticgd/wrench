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

#include "gltf.h"

#include <nlohmann/json.hpp>
using Json = nlohmann::ordered_json;

namespace GLTF {

packed_struct(GLBHeader,
	u32 magic;
	u32 version;
	u32 length;
)

packed_struct(GLBChunk,
	u32 length;
	u32 type;
)

#define FOURCC(string) ((string)[0] | (string)[1] << 8 | (string)[2] << 16 | (string)[3] << 24)

static Result<s32> read_gltf(ModelFile& dest, const Json& src);
static void write_gltf(Json& dest, const ModelFile& src);

Result<ModelFile> read_glb(::Buffer src) {
	const GLBHeader& header = src.read<GLBHeader>(0);
	
	// The format is made up of a stream of chunks. Find them.
	s64 json_offset = -1;
	s64 json_size = -1;
	s64 bin_offset = -1;
	s64 bin_size = -1;
	for(s64 offset = sizeof(GLBHeader); offset < header.length;) {
		const GLBChunk& chunk = src.read<GLBChunk>(offset);
		if(chunk.type == FOURCC("JSON")) {
			json_offset = offset + sizeof(GLBChunk);
			json_size = chunk.length;
		} else if(chunk.type == FOURCC("BIN\x00")) {
			bin_offset = offset + sizeof(GLBChunk);
			bin_size = chunk.length;
		}
		offset += sizeof(GLBChunk) + chunk.length;
	}
	
	// Safety standards are written in blood!
	if(json_offset < 0 || json_size < 0 || src.lo + json_offset + json_size > src.hi) {
		return RESULT_FAILURE("no valid JSON chunk present");
	}
	
	if(bin_offset < 0 || bin_size < 0 || src.lo + bin_offset + bin_size > src.hi) {
		return RESULT_FAILURE("no valid BIN chunk present");
	}
	
	ModelFile gltf;
	
	try {
		auto json = Json::parse(src.lo + json_offset, src.lo + json_offset + json_size);
		
		// Interpret the parsed JSON.
		Result<s32> result = read_gltf(gltf, json);
		if(result.error) {
			return RESULT_PROPAGATE(result);
		}
	} catch(Json::exception& e) {
		return RESULT_FAILURE("%s", e.what());
	}
	
	// Copy the data in the BIN chunk.
	gltf.bin_data = src.read_bytes(bin_offset, bin_size, "bin chunk");
	
	return RESULT_SUCCESS(gltf);
}

void write_glb(OutBuffer dest, const ModelFile& gltf) {
	Json root = Json::object();
	write_gltf(root, gltf);
	
	std::string json = root.dump();
	
	json.resize(align64(json.size(), 4), ' ');
	size_t padded_binary_size = align64(gltf.bin_data.size(), 4);
	
	GLBHeader header;
	header.magic = FOURCC("glTF");
	header.version = 2;
	header.length = sizeof(GLBHeader) + sizeof(GLBChunk) + json.size() + sizeof(GLBChunk) + padded_binary_size;
	dest.write(header);
	
	GLBChunk json_header;
	json_header.length = json.size();
	json_header.type = FOURCC("JSON");
	dest.write(json_header);
	dest.write_multiple(json);
	
	GLBChunk binary_header;
	binary_header.length = padded_binary_size;
	binary_header.type = FOURCC("BIN\00");
	dest.write(binary_header);
	dest.write_multiple(gltf.bin_data);
	for(size_t i = gltf.bin_data.size(); (i % 4) != 0; i++) {
		dest.write<u8>(0);
	}
}

template <typename T>
static Opt<T> get_opt(const Json& json, const char* property) {
	if(json.contains(property)) {
		return json[property].get<T>();
	} else {
		return std::nullopt;
	}
}

static Result<s32> read_gltf(ModelFile& dest, const Json& src) {
	dest.extensions_used = get_opt<std::vector<std::string>>(src, "extensionsUsed");
	
	return RESULT_SUCCESS(0);
}

static void write_gltf(Json& dest, const ModelFile& src) {
	
}

}
