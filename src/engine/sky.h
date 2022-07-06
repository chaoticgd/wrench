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

#ifndef ENGINE_SKY_H
#define ENGINE_SKY_H

#include <core/buffer.h>
#include <core/collada.h>
#include <core/texture.h>
#include <core/build_config.h>
#include <engine/basic_types.h>

struct SkyShell {
	Mesh mesh;
	bool textured;
	bool bloom;
	s16 rotx;
	s16 roty;
	s16 rotz;
	s16 rotdeltax;
	s16 rotdeltay;
	s16 rotdeltaz;
};

struct SkySprite {
	
};

struct Sky {
	std::vector<SkyShell> shells;
	std::vector<SkySprite> sprites;
	std::vector<Texture> textures;
};

Sky read_sky(Buffer src, Game game);
void write_sky(OutBuffer dest, const Sky& sky, Game game);

#endif
