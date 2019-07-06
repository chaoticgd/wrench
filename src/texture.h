#ifndef TEXTURE_H
#define TEXTURE_H

#include <array>
#include <vector>
#include <stdint.h>
#include <glm/glm.hpp>
#include <GL/glew.h>

#include "window.h"
#include "gui.h"

struct colour {
	uint8_t r, g, b;
};

class texture {
public:
	virtual ~texture() = default;

	virtual glm::vec2 size() const = 0;
	virtual void set_size(glm::vec2 size_) = 0;

	virtual std::array<colour, 256> palette() const = 0;
	virtual void set_palette(std::array<colour, 256> palette_) = 0;

	virtual std::vector<uint8_t> pixel_data() const = 0;
	virtual void set_pixel_data(std::vector<uint8_t> pixel_data_) = 0;
};

class texture_provider {
public:
	virtual ~texture_provider() = default;

	virtual std::vector<texture*> textures() = 0;
	
	std::vector<const texture*> textures() const;

	template <typename... T>
	void reflect(T... callbacks);
};

class texture_browser : public window {
public:
	texture_browser(texture_provider* provider);
	~texture_browser();

	const char* title_text() const override;
	ImVec2 initial_size() const override;
	void render(app& a) override;
	
private:
	texture_provider* _provider;
	std::map<texture*, GLuint> _gl_textures;
};

template <typename... T>
void texture_provider::reflect(T... callbacks) {
	rf::reflector r(this, callbacks...);
	r.visit_f("Open in Texture Browser",
		[]() { return static_cast<app*>(nullptr); },
		[this](app* a) {
			a->windows.emplace_back(std::make_unique<texture_browser>(this));
		});
}

#endif
