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

import os
import sys
import subprocess
import _common

def extract_all(src_path, dest_dir):
	if not os.path.isdir(dest_dir):
		os.mkdir(dest_dir)
	segments = _common.get_segments(src_path, 16)

	with open(src_path, 'rb') as src:
		for segment in segments:
			if 'error' in segment:
				continue

			inner_data = None

			dest_path = dest_dir + os.sep + hex(segment['offset'])
			if segment['type'] == 'wad':
				dest_path += '.decompressed'
				if segment['compressed_data'] != None:
					dest_path += '.' + segment['compressed_data']['type']
					inner_data = segment['compressed_data']
				extract_wad(src_path, segment['offset'], dest_path)
				print('Written decompressed WAD to ' + dest_path)
			else:
				dest_path += '.' + segment['type']
				src.seek(segment['offset'])
				data = src.read(segment['size'])
				with open(dest_path, 'wb') as dest:
					dest.write(data)
				inner_data = segment
				print('Written uncompressed file to ' + dest_path)
			
			if inner_data != None and inner_data['type'] == 'fip':
				convert_fip(dest_path, dest_path + '.bmp')


def extract_wad(src_path, src_offset, dest_path):
	args = [sys.path[0] + '/../bin/wad', 'decompress', src_path, dest_path, '-o', hex(src_offset)]
	err = subprocess.check_output(args)
	if err != b'':
		print(err)

def convert_fip(src_path, dest_path):
	args = [sys.path[0] + '/../bin/fip', 'export', src_path, dest_path]
	err = subprocess.check_output(args)
	if err != b'':
		print(err)

if __name__ == '__main__':
	if len(sys.argv) != 3:
		print('Incorrect number of arguments.')
	extract_all(sys.argv[1], sys.argv[2])
