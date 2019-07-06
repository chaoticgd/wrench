#include "texture.h"

std::vector<const texture*> texture_provider::textures() const {
	auto textures_ = const_cast<texture_provider*>(this)->textures();
	return std::vector<const texture*>(textures_.begin(), textures_.end());
}

texture_browser::texture_browser(texture_provider* provider)
	: _provider(provider) {}

texture_browser::~texture_browser() {
	for(auto& [texture, id] : _gl_textures) {
		glDeleteTextures(1, &id);
	}
}

const char* texture_browser::title_text() const {
	return "Texture Browser";
}

ImVec2 texture_browser::initial_size() const {
	return ImVec2(800, 600);
}

void texture_browser::render(app& a) {
	ImGui::Columns(std::max(1.f, ImGui::GetWindowSize().x / 128));

	int num_loaded_this_frame = 0;

	for(texture* texture : _provider->textures()) {
		auto size = texture->size();
		
		if(_gl_textures.find(texture) == _gl_textures.end()) {

			if(num_loaded_this_frame > 2) {
				//ImGui::NextColumn();
				//continue;
			}

			// Prepare pixel data.
			std::vector<uint8_t> indexed_pixel_data = texture->pixel_data();
			std::vector<uint8_t> colour_data(indexed_pixel_data.size() * 4);
			
			for(std::size_t i = 0; i < indexed_pixel_data.size(); i++) {
				colour c = texture->palette()[indexed_pixel_data[i]];
				colour_data[i * 4] = c.r;
				colour_data[i * 4 + 1] = c.g;
				colour_data[i * 4 + 2] = c.b;
				colour_data[i * 4 + 3] = 255;
			}

			// Send image to OpenGL.
			GLuint texture_id;
			glGenTextures(1, &texture_id);
			glBindTexture(GL_TEXTURE_2D, texture_id);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, colour_data.data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			_gl_textures.emplace(texture, texture_id);

			num_loaded_this_frame++;
		}

		ImGui::Image((void*) (intptr_t) _gl_textures.at(texture), ImVec2(128, 128));
		ImGui::NextColumn();
	}

	ImGui::Columns(1);
}
