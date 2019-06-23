#!/usr/bin/env python3

"""
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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
"""

# Shuffles most of the GUI textures in the game. This also demonstrates how to
# use the scan tool included with wrench.

import heapq
import random
import _common

def shuffle(iso_path):
	segments = _common.get_segments(iso_path)

	moved = 0
	errors = 0
	swappables = []

	for segment in segments:

		if 'error' in segment:
			# scan is a little buggy and sometimes fails.
			# Don't let this distract us.
			errors += 1
			continue

		if segment['type'] != 'wad':
			# The segment is not compressed, so ignore it.
			continue
		
		if segment['compressed_data'] == None or segment['compressed_data']['type'] != 'fip':
			# The compressed data segment is not a texture, so ignore it.
			continue
		
		# Hack to prevent keys from being equal as that makes heapq sad.
		key = -segment['compressed_size'] - segment["offset"] / pow(2, 32)
		heapq.heappush(swappables, (key, segment))
	
	with open(iso_path, 'r+b') as iso:
		while len(swappables) > 1:
			dest_segment = heapq.heappop(swappables)[1]
			src_segment = random.choice(swappables)[1]
			iso.seek(src_segment['offset'])
			data = iso.read(src_segment['compressed_size'])
			iso.seek(dest_segment['offset'])
			iso.write(data)
			moved += 1
		

	print('Moved: ' + str(moved))
	print('Errors: ' + str(errors))


if __name__ == '__main__':
	if len(sys.argv) != 2:
		print('Incorrect number of arguments.')
	shuffle(sys.argv[1])
