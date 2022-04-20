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

void unpack_global_wads(AssetPack& dest_pack, BuildAsset& dest_build, BuildAsset& src_build) {
	ArmorWadAsset& dest_armor = dest_pack.asset_file("armors/armors.asset").root().child<ArmorWadAsset>("armors");
	unpack_armor_wad(dest_armor, src_build.get_armor().as<BinaryAsset>());
	dest_build.armor<ReferenceAsset>().set_asset("/armors");
	
	AudioWadAsset& dest_audio = dest_pack.asset_file("audio/audio.asset").root().child<AudioWadAsset>("audio");
	unpack_audio_wad(dest_audio, src_build.get_audio().as<BinaryAsset>());
	dest_build.audio<ReferenceAsset>().set_asset("/audio");
	
	BonusWadAsset& dest_bonus = dest_pack.asset_file("bonus/bonus.asset").root().child<BonusWadAsset>("bonus");
	unpack_bonus_wad(dest_bonus, src_build.get_bonus().as<BinaryAsset>());
	dest_build.bonus<ReferenceAsset>().set_asset("/bonus");
	
	HudWadAsset& dest_hud = dest_pack.asset_file("hud/hud.asset").root().child<HudWadAsset>("hud");
	unpack_hud_wad(dest_hud, src_build.get_hud().as<BinaryAsset>());
	dest_build.hud<ReferenceAsset>().set_asset("/hud");
	
	MiscWadAsset& dest_misc = dest_pack.asset_file("misc/misc.asset").root().child<MiscWadAsset>("misc");
	unpack_misc_wad(dest_misc, src_build.get_misc().as<BinaryAsset>());
	dest_build.misc<ReferenceAsset>().set_asset("/misc");
	
	MpegWadAsset& dest_mpeg = dest_pack.asset_file("mpegs/mpegs.asset").root().child<MpegWadAsset>("mpegs");
	unpack_mpeg_wad(dest_mpeg, src_build.get_mpeg().as<BinaryAsset>());
	dest_build.mpeg<ReferenceAsset>().set_asset("/mpegs");
	
	OnlineWadAsset& dest_online = dest_pack.asset_file("online/online.asset").root().child<OnlineWadAsset>("online");
	unpack_online_wad(dest_online, src_build.get_online().as<BinaryAsset>());
	dest_build.online<ReferenceAsset>().set_asset("/online");
	
	SpaceWadAsset& dest_space = dest_pack.asset_file("space/space.asset").root().child<SpaceWadAsset>("space");
	unpack_space_wad(dest_space, src_build.get_space().as<BinaryAsset>());
	dest_build.space<ReferenceAsset>().set_asset("/space");
}
