#!/usr/bin/env python3

# Shuffles most of the GUI textures in the game. This also demonstrates how to
# use the scan tool included with wrench.

import sys
import json
import heapq
import random
import subprocess

def shuffle(iso_path):
	segments_str = subprocess.check_output([sys.path[0] + '/../bin/scan', iso_path])
	segments = [json.loads(line) for line in segments_str.decode().split('\n')[:-1]]

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
	if(len(sys.argv) != 2):
		print('Incorrect number of arguments.')
	shuffle(sys.argv[1])
