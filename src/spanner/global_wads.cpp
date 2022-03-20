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

#include "global_wads.h"

#include <spanner/globals/armor_wad.h>
#include <spanner/globals/audio_wad.h>
#include <spanner/globals/bonus_wad.h>
#include <spanner/globals/hud_wad.h>
#include <spanner/globals/misc_wad.h>
#include <spanner/globals/mpeg_wad.h>
#include <spanner/globals/online_wad.h>
#include <spanner/globals/space_wad.h>

void unpack_global_wads(AssetPack& dest_pack, BuildAsset& build) {
	BinaryAsset* armor_wad = dynamic_cast<BinaryAsset*>(build.armor());
	verify(armor_wad, "Invalid armor.wad asset.");
	unpack_armor_wad(dest_pack, *armor_wad);
	
	BinaryAsset* audio_wad = dynamic_cast<BinaryAsset*>(build.audio());
	verify(audio_wad, "Invalid audio.wad asset.");
	unpack_audio_wad(dest_pack, *audio_wad);
	
	BinaryAsset* bonus_wad = dynamic_cast<BinaryAsset*>(build.bonus());
	verify(bonus_wad, "Invalid bonus.wad asset.");
	unpack_bonus_wad(dest_pack, *bonus_wad);
	
	BinaryAsset* hud_wad = dynamic_cast<BinaryAsset*>(build.hud());
	verify(hud_wad, "Invalid hud.wad asset.");
	unpack_hud_wad(dest_pack, *hud_wad);
	
	BinaryAsset* misc_wad = dynamic_cast<BinaryAsset*>(build.misc());
	verify(misc_wad, "Invalid misc.wad asset.");
	unpack_misc_wad(dest_pack, *misc_wad);
	
	BinaryAsset* mpeg_wad = dynamic_cast<BinaryAsset*>(build.mpeg());
	verify(mpeg_wad, "Invalid mpeg.wad asset.");
	unpack_mpeg_wad(dest_pack, *mpeg_wad);
	
	BinaryAsset* online_wad = dynamic_cast<BinaryAsset*>(build.online());
	verify(online_wad, "Invalid online.wad asset.");
	unpack_online_wad(dest_pack, *online_wad);
	
	BinaryAsset* space_wad = dynamic_cast<BinaryAsset*>(build.space());
	verify(space_wad, "Invalid space.wad asset.");
	unpack_space_wad(dest_pack, *space_wad);
}
