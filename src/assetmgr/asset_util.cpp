/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2022 chaoticgd

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

#include "asset_util.h"

#include "asset_types.h"

AssetLink::AssetLink() {}

AssetLink::AssetLink(const char* src)
{
	set(src);
}

AssetLinkPointers AssetLink::get() const
{
	AssetLinkPointers pointers;
	const char* ptr = &m_data[0];
	if (m_prefix) {
		pointers.prefix = ptr;
		ptr += strlen(ptr) + 1;
	}
	pointers.tags.reserve(m_tags);
	for (s16 i = 0; i < m_tags; i++) {
		pointers.tags.push_back(ptr);
		ptr += strlen(ptr) + 1;
	}
	return pointers;
}

void AssetLink::set(const char* src)
{
	m_prefix = false;
	m_tags = 0;
	size_t size = strlen(src);
	m_data.resize(size + 1);
	for (size_t i = 0; i < strlen(src); i++) {
		if (src[i] == '.' || src[i] == ':') {
			if (src[i] == ':') {
				verify(!m_prefix && m_tags == 0, "Syntax error while parsing asset link.");
				m_prefix = true;
			} else {
				m_tags++;
			}
			m_data[i] = '\0';
		} else {
			m_data[i] = src[i];
		}
	}
	m_tags++;
	m_data[size] = '\0';
}

void AssetLink::add_prefix(const char* str)
{
	verify_fatal(!m_prefix && m_tags == 0);
	size_t size = strlen(str);
	m_data.resize(size + 1);
	memcpy(m_data.data(), str, size);
	m_data[size] = '\0';
	m_prefix = true;
}

void AssetLink::add_tag(const char* tag)
{
	size_t old_size = m_data.size();
	size_t tag_size = strlen(tag);
	m_data.resize(old_size + tag_size + 1);
	for (size_t i = 0; i < tag_size; i++) {
		m_data[old_size + i] = tag[i];
	}
	m_data[old_size + tag_size] = '\0';
	m_tags++;
}

std::string AssetLink::to_string() const
{
	std::string str;
	const char* ptr = &m_data[0];
	if (m_prefix) {
		str += ptr;
		str += ':';
		ptr += strlen(ptr) + 1;
	}
	for (s16 i = 0; i < m_tags; i++) {
		str += ptr;
		if (i != m_tags - 1) {
			str += '.';
		}
		ptr += strlen(ptr) + 1;
	}
	return str;
}

std::unique_ptr<InputStream> FileReference::open_binary_file_for_reading(fs::file_time_type* modified_time_dest) const
{
	return owner->open_binary_file_for_reading(*this, modified_time_dest);
}

std::string FileReference::read_text_file() const
{
	return owner->read_text_file(path);
}

std::vector<ColladaScene*> read_collada_files(std::vector<std::unique_ptr<ColladaScene>>& owners, std::vector<FileReference> refs)
{
	std::vector<ColladaScene*> scenes;
	for (size_t i = 0; i < refs.size(); i++) {
		bool unique = true;
		size_t j;
		for (j = 0; j < refs.size(); j++) {
			if (i > j && refs[i].owner == refs[j].owner && refs[i].path == refs[j].path) {
				unique = false;
				break;
			}
		}
		if (unique) {
			std::string xml = refs[i].read_text_file();
			std::unique_ptr<ColladaScene>& owner = owners.emplace_back(std::make_unique<ColladaScene>(read_collada((char*) xml.data())));
			scenes.emplace_back(owner.get());
		} else {
			scenes.emplace_back(scenes[j]);
		}
	}
	return scenes;
}

std::vector<GLTF::ModelFile*> read_glb_files(std::vector<std::unique_ptr<GLTF::ModelFile>>& owners, std::vector<FileReference> refs)
{
	std::vector<GLTF::ModelFile*> model_files;
	for (size_t i = 0; i < refs.size(); i++) {
			bool unique = true;
		size_t j;
		for (j = 0; j < refs.size(); j++) {
			if (i > j && refs[i].owner == refs[j].owner && refs[i].path == refs[j].path) {
				unique = false;
				break;
			}
		}
		if (unique) {
			std::unique_ptr<InputStream> stream = refs[i].open_binary_file_for_reading();
			std::vector<u8> buffer = stream->read_multiple<u8>(stream->size());
			std::unique_ptr<GLTF::ModelFile> model_file = std::make_unique<GLTF::ModelFile>(GLTF::read_glb(buffer));
			std::unique_ptr<GLTF::ModelFile>& owner = owners.emplace_back(std::move(model_file));
			model_files.emplace_back(owner.get());
		} else {
			model_files.emplace_back(model_files[j]);
		}
	}
	return model_files;
}

const char* next_hint(const char** hint)
{
	static char temp[256];
	if (hint) {
		for (s32 i = 0;; i++) {
			if ((*hint)[i] == ',' || (*hint)[i] == '\0' || i >= 255) {
				strncpy(temp, *hint, i);
				temp[i] = '\0';
				if ((*hint)[i] == ',') {
					*hint += i + 1;
				} else {
					*hint += i;
				}
				break;
			}
		}
	} else {
		temp[0] = '\0';
	}
	return temp;
}
