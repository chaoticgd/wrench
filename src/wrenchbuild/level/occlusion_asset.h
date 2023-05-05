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

#ifndef WRENCHBUILD_LEVEL_OCCLUSION_ASSET_H
#define WRENCHBUILD_LEVEL_OCCLUSION_ASSET_H

#include <engine/gameplay.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/tests.h>
#include <wrenchbuild/level/level_chunks.h>
#include <wrenchbuild/level/level_classes.h>

ByteRange pack_occlusion(OutputStream& dest, Gameplay& gameplay, const OcclusionAsset& asset, const std::vector<LevelChunk>& chunks, const ClassesHigh& high_classes, BuildConfig config);

#endif
