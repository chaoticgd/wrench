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

#include "texture.h"

#include <md5.h>

std::string hash_texture(const Texture& texture) {
	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, (u8*) &texture.format, 1);
	if(texture.format == PixelFormat::IDTEX8) {
		MD5Update(&ctx, (u8*) &texture.palette.top, 4);
		MD5Update(&ctx, (u8*) texture.palette.colours.data(), texture.palette.top * 4);
	}
	MD5Update(&ctx, texture.pixels.data(), texture.pixels.size());
	uint8_t bytes[MD5_DIGEST_LENGTH];
	MD5Final(bytes, &ctx);
	return md5_to_printable_string(bytes);
}
