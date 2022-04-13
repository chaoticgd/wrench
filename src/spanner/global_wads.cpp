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
	ArmorWadAsset& dest_armor = dest_build.switch_files("armors/armors.asset").armor<ArmorWadAsset>();
	unpack_armor_wad(dest_armor, src_build.get_armor().as<BinaryAsset>());
	
	AudioWadAsset& dest_audio = dest_build.switch_files("audio/audio.asset").audio<AudioWadAsset>();
	unpack_audio_wad(dest_audio, src_build.get_audio().as<BinaryAsset>());
	
	BonusWadAsset& dest_bonus = dest_build.switch_files("bonus/bonus.asset").bonus<BonusWadAsset>();
	unpack_bonus_wad(dest_bonus, src_build.get_bonus().as<BinaryAsset>());
	
	HudWadAsset& dest_hud = dest_build.switch_files("hud/hud.asset").hud<HudWadAsset>();
	unpack_hud_wad(dest_hud, src_build.get_hud().as<BinaryAsset>());
	
	MiscWadAsset& dest_misc = dest_build.switch_files("misc/misc.asset").misc<MiscWadAsset>();
	unpack_misc_wad(dest_misc, src_build.get_misc().as<BinaryAsset>());
	
	MpegWadAsset& dest_mpeg = dest_build.switch_files("mpegs/mpegs.asset").mpeg<MpegWadAsset>();
	unpack_mpeg_wad(dest_mpeg, src_build.get_mpeg().as<BinaryAsset>());
	
	OnlineWadAsset& dest_online = dest_build.switch_files("online/online.asset").online<OnlineWadAsset>();
	unpack_online_wad(dest_online, src_build.get_online().as<BinaryAsset>());
	
	SpaceWadAsset& dest_space = dest_build.switch_files("space/space.asset").space<SpaceWadAsset>();
	unpack_space_wad(dest_space, src_build.get_space().as<BinaryAsset>());
}

void pack_global_wad(OutputStream& dest, Asset& wad, Game game) {
	switch(wad.type().id) {
		case ArmorWadAsset::ASSET_TYPE.id: pack_armor_wad(dest, static_cast<ArmorWadAsset&>(wad), game); break;
		case AudioWadAsset::ASSET_TYPE.id: pack_audio_wad(dest, static_cast<AudioWadAsset&>(wad), game); break;
		case BonusWadAsset::ASSET_TYPE.id: pack_bonus_wad(dest, static_cast<BonusWadAsset&>(wad), game); break;
		case HudWadAsset::ASSET_TYPE.id: pack_hud_wad(dest, static_cast<HudWadAsset&>(wad), game); break;
		case MiscWadAsset::ASSET_TYPE.id: pack_misc_wad(dest, static_cast<MiscWadAsset&>(wad), game); break;
		case MpegWadAsset::ASSET_TYPE.id: pack_mpeg_wad(dest, static_cast<MpegWadAsset&>(wad), game); break;
		case OnlineWadAsset::ASSET_TYPE.id: pack_online_wad(dest, static_cast<OnlineWadAsset&>(wad), game); break;
		case SpaceWadAsset::ASSET_TYPE.id: pack_space_wad(dest, static_cast<SpaceWadAsset&>(wad), game); break;
		default: verify_not_reached("Failed to identify WAD asset.");
	}
}
