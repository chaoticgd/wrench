/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#include "icons.h"

#include <vector>
#include <glm/glm.hpp>

static void path(
	uint32_t image[START_SCREEN_ICON_SIDE][START_SCREEN_ICON_SIDE], std::vector<glm::vec2> points);
static void line(
	uint32_t image[START_SCREEN_ICON_SIDE][START_SCREEN_ICON_SIDE],
	float x0,
	float y0,
	float x1,
	float y1);
static GlTexture upload_icon(uint32_t* pixels, int side);

GlTexture create_dvd_icon()
{
	const int outer_radius = 45;
	const int inner_radius = 10;
	const int shine_max_radius = 41;
	const int shine_min_radius = 14;
	const float shine_min_angle = M_PI * 0.125;
	const float shine_max_angle = M_PI * 0.5 - M_PI * 0.125;
	const glm::vec2 dvd_centre = { START_SCREEN_ICON_SIDE / 2, START_SCREEN_ICON_SIDE / 2 };
	uint32_t icon[START_SCREEN_ICON_SIDE][START_SCREEN_ICON_SIDE];
	for (int y = 0; y < START_SCREEN_ICON_SIDE; y++) {
		for (int x = 0; x < START_SCREEN_ICON_SIDE; x++) {
			glm::vec2 point = glm::vec2(x, y) - dvd_centre;
			int radius = glm::distance(point, glm::vec2(0, 0));
			float angle = atan(-point.y / point.x);
			bool cond = false;
			cond |= radius == outer_radius;
			cond |= radius == inner_radius;
			cond |=
				radius >= shine_min_radius &&
				radius <= shine_max_radius &&
				angle >= shine_min_angle &&
				angle <= shine_max_angle;
			icon[y][x] = cond ? 0xffffffff : 0;
		}
	}
	return upload_icon((uint32_t*) icon, START_SCREEN_ICON_SIDE);
}

GlTexture create_folder_icon()
{
	int top = 10;
	int uppermid = 20;
	int lowermid = 30;
	int baseline = 85;
	uint32_t icon[START_SCREEN_ICON_SIDE][START_SCREEN_ICON_SIDE] = {0};
	path(icon, {{0, top}, {0, baseline}});
	path(icon, {{0, top}, {35, top}, {45, uppermid}, {80, uppermid}, {80, lowermid}});
	path(icon, {{0, baseline}, {20, lowermid}, {95, lowermid}});
	path(icon, {{0, baseline}, {75, baseline}, {95, lowermid}});
	return upload_icon((uint32_t*) icon, START_SCREEN_ICON_SIDE);
}

GlTexture create_floppy_icon()
{
	int left = 5;
	int right = 90;
	int corner = 15;
	uint32_t icon[START_SCREEN_ICON_SIDE][START_SCREEN_ICON_SIDE] = {0};
	path(icon, {{left, 5}, {left, 90}, {right, 90}});
	path(icon, {{left, 5}, {right - corner, 5}, {right, 5 + corner}, {right, 90}});
	path(icon, {{left + 20, 5}, {left + 20, 30}, {right - 20, 30}});
	path(icon, {{right - 20, 5}, {right - 20, 30}});
	path(icon, {{left + 15, 50}, {left + 15, 90}});
	path(icon, {{left + 15, 90}, {left + 15, 50}, {right - 15, 50}, {right - 15, 90}});
	return upload_icon((uint32_t*) icon, START_SCREEN_ICON_SIDE);
}

static void path(
	uint32_t image[START_SCREEN_ICON_SIDE][START_SCREEN_ICON_SIDE], std::vector<glm::vec2> points)
{
	for (size_t i = 0; i < points.size() - 1; i++) {
		line(image, points[i].x, points[i].y, points[i + 1].x, points[i + 1].y);
	}
}

static void line(
	uint32_t image[START_SCREEN_ICON_SIDE][START_SCREEN_ICON_SIDE],
	float x0,
	float y0,
	float x1,
	float y1) {
	auto inbounds = [&](int coord) {
		int max = START_SCREEN_ICON_SIDE - 1;
		if (coord > max) coord = max;
		if (coord < 0) coord = 0;
		return coord;
	};
	float m;
	if (x1 - x0 == 0) {
		m = 1000000.f;
	} else {
		m = (y1 - y0) / (x1 - x0);
	}
	if (m < -1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	float c = y0 - m * x0;
	if (m > 1 || m < -1) {
		for (int y = y0; y < y1; y++) {
			int x = (y - c) / m;
			image[inbounds(y)][inbounds(x)] = 0xffffffff;
		}
	} else {
		float c = y0 - m * x0;
		for (int x = x0; x < x1; x++) {
			int y = m * x + c;
			image[inbounds(y)][inbounds(x)] = 0xffffffff;
		}
	}
}

static GlTexture upload_icon(uint32_t* pixels, int side)
{
	GlTexture texture;
	glGenTextures(1, &texture.id);
	glBindTexture(GL_TEXTURE_2D, texture.id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, side, side, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	return texture;
}
