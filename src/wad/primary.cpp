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

#include "primary.h"

#include <engine/compression.h>
#include <wad/assets.h>

static WadBuffer wad_buffer(Buffer buf) {
	return {buf.lo, buf.hi};
}

void read_primary(LevelWad& wad, Buffer src) {
	PrimaryHeader header = read_primary_header(src, wad.game);
	
	wad.code = src.read_bytes(header.code.offset, header.code.size, "code");
	if(header.core_bank.has_value()) {
		wad.core_bank = src.read_bytes(header.core_bank->offset, header.core_bank->size, "core bank");
	}
	wad.asset_header = src.read_bytes(header.asset_header.offset, header.asset_header.size, "asset header");
	wad.hud_header = src.read_bytes(header.hud_header.offset, header.hud_header.size, "hud header");
	for(s32 i = 0; i < 5; i++) {
		if(header.hud_banks[i].offset > 0) {
			wad.hud_banks[i] = src.read_bytes(header.hud_banks[i].offset, header.hud_banks[i].size, "hud bank");
		}
	}
	std::vector<u8> assets_vec;
	verify(decompress_wad(assets_vec, wad_buffer(src.subbuf(header.assets.offset))), "Failed to decompress assets.");
	Buffer assets(assets_vec);
	read_assets(wad, wad.asset_header, assets, src.subbuf(header.gs_ram.offset));
	if(header.transition_textures.has_value() && header.transition_textures->offset > -1) {
		wad.transition_textures = src.read_bytes(header.transition_textures->offset, header.transition_textures->size, "transition textures");
	}
	if(header.moby8355_pvars.has_value()) {
		wad.moby8355_pvars = src.read_bytes(header.moby8355_pvars->offset, header.moby8355_pvars->size, "moby 8355 pvars");
	}
	if(header.global_nav_data.has_value()) {
		wad.global_nav_data = src.read_bytes(header.global_nav_data->offset, header.global_nav_data->size, "global nav data");
	}
}

static ByteRange write_primary_block(OutBuffer dest, const std::vector<u8>& bytes, s64 primary_ofs) {
	dest.pad(0x40);
	s64 block_ofs = dest.tell();
	dest.write_multiple(bytes);
	return {(s32) (block_ofs - primary_ofs), (s32) (dest.tell() - block_ofs)};
}

SectorRange write_primary(OutBuffer dest, LevelWad& wad) {
	dest.pad(SECTOR_SIZE, 0);
	
	PrimaryHeader header = {0};
	s64 header_ofs = 0;
	switch(wad.game) {
		case Game::RAC1:
			header_ofs = dest.alloc<Rac1PrimaryHeader>();
			break;
		case Game::RAC2:
		case Game::RAC3:
			header_ofs = dest.alloc<Rac23PrimaryHeader>();
			break;
		case Game::DL:
			header_ofs = dest.alloc<DeadlockedPrimaryHeader>();
			break;
	}
	
	if(wad.moby8355_pvars.has_value()) {
		header.moby8355_pvars = write_primary_block(dest, *wad.moby8355_pvars, header_ofs);
	}
	header.code = write_primary_block(dest, wad.code, header_ofs);
	
	std::vector<u8> asset_header;
	std::vector<u8> asset_data;
	std::vector<u8> gs_ram;
	write_assets(OutBuffer(asset_header), OutBuffer(asset_data), gs_ram, wad);
	std::vector<u8> compressed_assets;
	compress_wad(compressed_assets, asset_data, 8);
	*(s32*) &asset_header[0x88] = compressed_assets.size();
	
	header.asset_header = write_primary_block(dest, asset_header, header_ofs);
	header.gs_ram = write_primary_block(dest, gs_ram, header_ofs);
	header.hud_header = write_primary_block(dest, wad.hud_header, header_ofs);
	for(s32 i = 0; i < 5; i++) {
		if(wad.hud_banks[i].size() > 0) {
			header.hud_banks[i] = write_primary_block(dest, wad.hud_banks[i], header_ofs);
		}
	}
	
	header.assets = write_primary_block(dest, compressed_assets, header_ofs);
	if(wad.transition_textures.has_value()) {
		header.transition_textures = write_primary_block(dest, *wad.transition_textures, header_ofs);
	}
	
	write_primary_header(dest, header_ofs, header, wad.game);
	
	return {
		(s32) (header_ofs / SECTOR_SIZE),
		Sector32::size_from_bytes(dest.tell() - header_ofs)
	};
}

PrimaryHeader read_primary_header(Buffer src, Game game) {
	PrimaryHeader dest;
	switch(game) {
		case Game::RAC1: {
			auto header = src.read<Rac1PrimaryHeader>(0, "R&C1 primary header");
			dest.code = header.code;
			dest.core_bank = header.core_bank;
			dest.asset_header = header.asset_header;
			dest.gs_ram = header.gs_ram;
			dest.hud_header = header.hud_header;
			dest.hud_banks[0] = header.hud_banks[0];
			dest.hud_banks[1] = header.hud_banks[1];
			dest.hud_banks[2] = header.hud_banks[2];
			dest.hud_banks[3] = header.hud_banks[3];
			dest.hud_banks[4] = header.hud_banks[4];
			dest.assets = header.assets;
			break;
		}
		case Game::RAC2:
		case Game::RAC3: {
			auto header = src.read<Rac23PrimaryHeader>(0, "R&C2/3 primary header");
			dest.code = header.code;
			dest.asset_header = header.asset_header;
			dest.gs_ram = header.gs_ram;
			dest.hud_header = header.hud_header;
			dest.hud_banks[0] = header.hud_banks[0];
			dest.hud_banks[1] = header.hud_banks[1];
			dest.hud_banks[2] = header.hud_banks[2];
			dest.hud_banks[3] = header.hud_banks[3];
			dest.hud_banks[4] = header.hud_banks[4];
			dest.assets = header.assets;
			dest.transition_textures = header.transition_textures;
			break;
		}
		case Game::DL: {
			auto header = src.read<DeadlockedPrimaryHeader>(0, "DL primary header");
			dest.moby8355_pvars = header.moby8355_pvars;
			dest.code = header.code;
			dest.asset_header = header.asset_header;
			dest.gs_ram = header.gs_ram;
			dest.hud_header = header.hud_header;
			dest.hud_banks[0] = header.hud_banks[0];
			dest.hud_banks[1] = header.hud_banks[1];
			dest.hud_banks[2] = header.hud_banks[2];
			dest.hud_banks[3] = header.hud_banks[3];
			dest.hud_banks[4] = header.hud_banks[4];
			dest.assets = header.assets;
			dest.art_instances = header.art_instances;
			dest.gameplay_core = header.gameplay_core;
			dest.global_nav_data = header.global_nav_data;
			break;
		}
	}
	return dest;
}

void write_primary_header(OutBuffer dest, s64 header_ofs, const PrimaryHeader& src, Game game) {
	switch(game) {
		case Game::RAC1: {
			Rac1PrimaryHeader header;
			header.code = src.code;
			assert(src.core_bank.has_value());
			header.core_bank = *src.core_bank;
			header.asset_header = src.asset_header;
			header.gs_ram = src.gs_ram;
			header.hud_header = src.hud_header;
			header.hud_banks[0] = src.hud_banks[0];
			header.hud_banks[1] = src.hud_banks[1];
			header.hud_banks[2] = src.hud_banks[2];
			header.hud_banks[3] = src.hud_banks[3];
			header.hud_banks[4] = src.hud_banks[4];
			header.assets = src.assets;
			dest.write(header_ofs, header);
			break;
		}
		case Game::RAC2:
		case Game::RAC3: {
			Rac23PrimaryHeader header;
			header.code = src.code;
			header.asset_header = src.asset_header;
			header.gs_ram = src.gs_ram;
			header.hud_header = src.hud_header;
			header.hud_banks[0] = src.hud_banks[0];
			header.hud_banks[1] = src.hud_banks[1];
			header.hud_banks[2] = src.hud_banks[2];
			header.hud_banks[3] = src.hud_banks[3];
			header.hud_banks[4] = src.hud_banks[4];
			header.assets = src.assets;
			if(src.transition_textures.has_value()) {
				header.transition_textures = *src.transition_textures;
			} else {
				header.transition_textures = {-1, 0};
			}
			dest.write(header_ofs, header);
			break;
		}
		case Game::DL: {
			DeadlockedPrimaryHeader header;
			assert(src.moby8355_pvars.has_value());
			header.moby8355_pvars = *src.moby8355_pvars;
			header.code = src.code;
			header.asset_header = src.asset_header;
			header.gs_ram = src.gs_ram;
			header.hud_header = src.hud_header;
			header.hud_banks[0] = src.hud_banks[0];
			header.hud_banks[1] = src.hud_banks[1];
			header.hud_banks[2] = src.hud_banks[2];
			header.hud_banks[3] = src.hud_banks[3];
			header.hud_banks[4] = src.hud_banks[4];
			header.assets = src.assets;
			assert(src.art_instances.has_value());
			header.art_instances = *src.art_instances;
			assert(src.gameplay_core.has_value());
			header.gameplay_core = *src.gameplay_core;
			assert(src.global_nav_data.has_value());
			header.global_nav_data = *src.global_nav_data;
			dest.write(header_ofs, header);
			break;
		}
	}
}
