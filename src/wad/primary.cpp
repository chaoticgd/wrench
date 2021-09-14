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

#include "../lz/compression.h"
#include "assets.h"

static WadBuffer wad_buffer(Buffer buf) {
	return {buf.lo, buf.hi};
}

void read_primary(LevelWad& wad, Buffer src) {
	std::vector<u8> header_bytes = src.read_bytes(0, sizeof(DeadlockedPrimaryHeader), "primary header");
	PrimaryHeader header = {0};
	swap_primary_header(header, header_bytes, wad.game);
	
	wad.code = src.read_bytes(header.code.offset, header.code.size, "code");
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
	if(header.transition_textures.has_value()) {
		wad.transition_textures = src.read_bytes(header.transition_textures->offset, header.transition_textures->size, "transition textures");
	}
	if(header.moby8355_pvars.has_value()) {
		wad.moby8355_pvars = src.read_bytes(header.moby8355_pvars->offset, header.moby8355_pvars->size, "moby 8355 pvars");
	}
	if(header.global_nav_data.has_value()) {
		wad.global_nav_data = src.read_bytes(header.global_nav_data->offset, header.global_nav_data->size, "global nav data");
	}
}

static ByteRange write_primary_block(OutBuffer& dest, const std::vector<u8>& bytes, s64 primary_ofs) {
	dest.pad(0x40);
	s64 block_ofs = dest.tell();
	dest.write_multiple(bytes);
	return {(s32) (block_ofs - primary_ofs), (s32) (dest.tell() - block_ofs)};
}

SectorRange write_primary(OutBuffer& dest, LevelWad& wad) {
	dest.pad(SECTOR_SIZE, 0);
	
	PrimaryHeader header = {0};
	s64 header_ofs;
	if(wad.game == Game::DL) {
		header_ofs = dest.alloc<DeadlockedPrimaryHeader>();
	} else {
		header_ofs = dest.alloc<Rac123PrimaryHeader>();
	}
	
	if(wad.moby8355_pvars.has_value()) {
		header.moby8355_pvars = write_primary_block(dest, *wad.moby8355_pvars, header_ofs);
	}
	header.code = write_primary_block(dest, wad.code, header_ofs);
	std::vector<u8> asset_header;
	std::vector<u8> asset_data;
	std::vector<u8> gs_ram;
	write_assets(OutBuffer(asset_header), asset_data, gs_ram, wad);
	header.asset_header = write_primary_block(dest, asset_header, header_ofs);
	header.gs_ram = write_primary_block(dest, gs_ram, header_ofs);
	header.hud_header = write_primary_block(dest, wad.hud_header, header_ofs);
	for(s32 i = 0; i < 5; i++) {
		if(wad.hud_banks[i].size() > 0) {
			header.hud_banks[i] = write_primary_block(dest, wad.hud_banks[i], header_ofs);
		}
	}
	
	header.assets = write_primary_block(dest, asset_data, header_ofs);
	if(wad.transition_textures.has_value()) {
		header.transition_textures = write_primary_block(dest, *wad.transition_textures, header_ofs);
	}
	
	std::vector<u8> header_bytes;
	swap_primary_header(header, header_bytes, wad.game);
	dest.write_multiple(header_ofs, header_bytes);
	
	return {
		(s32) (header_ofs / SECTOR_SIZE),
		Sector32::size_from_bytes(dest.tell() - header_ofs)
	};
}

void swap_primary_header(PrimaryHeader& l, std::vector<u8>& r, Game game) {
	switch(game) {
		case Game::RAC1:
		case Game::RAC2:
		case Game::RAC3: {
			Rac123PrimaryHeader packed_header = {0};
			if(r.size() >= sizeof(Rac123PrimaryHeader)) {
				packed_header = Buffer(r).read<Rac123PrimaryHeader>(0, "primary header");
				l.transition_textures = ByteRange {0, 0};
			}
			l.moby8355_pvars = {};
			SWAP_PACKED(l.code, packed_header.code);
			SWAP_PACKED(l.asset_header, packed_header.asset_header);
			SWAP_PACKED(l.gs_ram, packed_header.gs_ram);
			SWAP_PACKED(l.hud_header, packed_header.hud_header);
			for(s32 i = 0; i < 5; i++) {
				SWAP_PACKED(l.hud_banks[i], packed_header.hud_banks[i]);
			}
			SWAP_PACKED(l.assets, packed_header.assets);
			SWAP_PACKED(*l.transition_textures, packed_header.transition_textures);
			l.art_instances = {};
			l.gameplay_core = {};
			l.global_nav_data = {};
			if(r.size() == 0) {
				OutBuffer(r).write(packed_header);
			}
			break;
		}
		case Game::DL: {
			DeadlockedPrimaryHeader packed_header = {0};
			if(r.size() >= sizeof(DeadlockedPrimaryHeader)) {
				packed_header = Buffer(r).read<DeadlockedPrimaryHeader>(0, "primary header");
				l.moby8355_pvars = ByteRange {0, 0};
				l.art_instances = ByteRange {0, 0};
				l.gameplay_core = ByteRange {0, 0};
				l.global_nav_data = ByteRange {0, 0};
			}
			SWAP_PACKED(*l.moby8355_pvars, packed_header.moby8355_pvars);
			SWAP_PACKED(l.code, packed_header.code);
			SWAP_PACKED(l.asset_header, packed_header.asset_header);
			SWAP_PACKED(l.gs_ram, packed_header.gs_ram);
			SWAP_PACKED(l.hud_header, packed_header.hud_header);
			for(s32 i = 0; i < 5; i++) {
				SWAP_PACKED(l.hud_banks[i], packed_header.hud_banks[i]);
			}
			SWAP_PACKED(l.assets, packed_header.assets);
			SWAP_PACKED(*l.moby8355_pvars, packed_header.moby8355_pvars);
			SWAP_PACKED(*l.art_instances, packed_header.art_instances);
			SWAP_PACKED(*l.gameplay_core, packed_header.gameplay_core);
			SWAP_PACKED(*l.global_nav_data, packed_header.global_nav_data);
			if(r.size() == 0) {
				OutBuffer(r).write(packed_header);
			}
			break;
		}
	}
}
