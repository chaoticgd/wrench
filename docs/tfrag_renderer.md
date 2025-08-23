# Tfrag Renderer

The tfrag (tesselating fragment) renderer is used to draw most of the large non-instanced geometry in the games. It was created at Naughty Dog for the Jak & Daxter games before being ported for use in Ratchet & Clank. Each level contains a single world space tfrag model in addition to tfrag models for each of the loaded chunks.

Tfrag models are constructed from patches of faces called tfaces (tesselating faces), which can be reduced in quality in a predetermined way to produce lower LOD versions of a model. For example, a 4x4 patch of quads may be reduced to a 2x2 patch of quads and then to just a single quad. The different LOD levels are converted to triangle strips and stored such that data is not unnecessarily duplicated between LOD levels. This system also facilitates smooth interpolation between the different LOD levels and prevents holes in geometry from forming at LOD boundaries.

Each tfrag model is broken up into individual tfrags based on VU1 memory constraints, stripping constraints, and vertex positions. Each tfrag has 3 different LOD levels. LOD 0 is the highest quality, and LOD 2 is the lowest quality.

## Tfrag Header

See `src/engine/tfrag_low.h` for a C++ struct definition.

<table>
	<th>
		<td>0x0</td><td>0x1</td><td>0x2</td><td>0x3</td>
		<td>0x4</td><td>0x5</td><td>0x6</td><td>0x7</td>
		<td>0x8</td><td>0x9</td><td>0xa</td><td>0xb</td>
		<td>0xc</td><td>0xd</td><td>0xe</td><td>0xf</td>
	</th>
	<tr>
		<td>0x00</td>
		<td colspan="16">bsphere</td>
	</tr>
	<tr>
		<td>0x10</td>
		<td colspan="4">data</td>
		<td colspan="2">lod_02_ofs</td>
		<td colspan="2">shared_ofs</td>
		<td colspan="2">lod_1_ofs</td>
		<td colspan="2">lod_0_ofs</td>
		<td colspan="2">tex_ofs</td>
		<td colspan="2">rgba_ofs</td>
	</tr>
	<tr>
		<td>0x20</td>
		<td>common*</td>
		<td>lod_2*</td>
		<td>lod_1*</td>
		<td>lod_0*</td>
		<td>lod_2_rgba**</td>
		<td>lod_1_rgba**</td>
		<td>lod_0_rgba**</td>
		<td>base_only</td>
		<td>texture_count</td>
		<td>rgba_size</td>
		<td>rgba_verts_loc</td>
		<td>occl_index_stash</td>
		<td>msphere_count</td>
		<td>flags</td>
		<td colspan="2">msphere_ofs</td>
	</tr>
	<tr>
		<td>0x30</td>
		<td colspan="2">light_ofs</td>
		<td colspan="2">light_vert_start_ofs</td>
		<td>dir_lights_one</td>
		<td>dir_lights_upd</td>
		<td colspan="2">point_lights</td>
		<td colspan="2">cube_ofs</td>
		<td colspan="2">occl_index</td>
		<td>vert_count</td>
		<td>tri_count</td>
		<td colspan="2">mip_dist</td>
	</tr>
</table>

\* These fields are sizes.

\*\* These fields are counts.

The data field is a pointer to the start of the tfrag, the offset fields are relative to the data field.

## VIF Command List Format

These command lists are stored in the model files and are used for transferring tfrag model data into VU1 memory. Different command lists are run depending on the LOD level.

### LOD 2 Only

| VIF Code             | Always Present | Explanation                                                                           |
| -------------------- | -------------- | ------------------------------------------------------------------------------------- |
| STROW                | Yes            | Converts indices below into addresses by adding the address of the vertex info table. |
| STMOD mode=1         | Yes            | Enable offset mode (see STROW).                                                       |
| UNPACK V4_8 UNSIGNED | Yes            | Unpack indices into VU1 memory.                                                       |
| STMOD mode=0         | Yes            | Disable offset mode (see STROW).                                                      |
| UNPACK V4_8 SIGNED   | Yes            | Unpack strips into VU1 memory.                                                        |

### All LODs

| VIF Code                 | Always Present | Explanation                                                                                 |
| ------------------------ | -------------- | ------------------------------------------------------------------------------------------- |
| UNPACK V4_16 UNSIGNED    | Yes            | Unpack header containing addresses into VU1 memory.                                         |
| UNPACK V4_32 SIGNED      | Yes            | Unpack AD GIF texture data into VU1 memory.                                                 |
| STROW                    | Yes            | Convert position indices below into addresses by adding the address of the positions table. |
| STMOD mode=1             | Yes            | Enable offset mode (see STROW).                                                             |
| UNPACK V4_16 SIGNED      | Yes            | Unpack vertex info into VU1 memory (part 1).                                                |
| STROW                    | Yes            | Add tfrag origin position to positions below.                                               |
| STCYCL num=100 wl=1 cl=2 | Yes            | Scatter the vertex positions below so that there is space for the colours.                  |
| UNPACK V3_16 SIGNED      | Yes            | Unpack positions into VU1 memory (part 1).                                                  |
| STCYCL num=100 wl=4 cl=4 | Yes            | Turn off scattering.                                                                        |
| STMOD mode=0             | Yes            | Disable offset mode (see STROW)                                                             |

### LOD 1 Only


| VIF Code             | Always Present | Explanation                                                                           |
| -------------------- | -------------- | ------------------------------------------------------------------------------------- |
| UNPACK V4_8 SIGNED   | Yes            | Unpack strips into VU1 memory.                                                        |
| STROW                | Yes            | Converts indices below into addresses by adding the address of the vertex info table. |
| STMOD mode=1         | Yes            | Enable offset mode (see STROW).                                                       |
| UNPACK V4_8 UNSIGNED | Yes            | Unpack indices into VU1 memory.                                                       |

### LOD 0 and LOD 1

| VIF Code                 | Always Present | Explanation                                                                                 |
| ------------------------ | -------------- | ------------------------------------------------------------------------------------------- |
| STROW                    | No             | Converts indices below into addresses by adding the address of the vertex info table.       |
| STMOD mode=1             | No             | Enable offset mode (see STROW).                                                             |
| UNPACK V4_8 UNSIGNED     | No             | Unpack parent indices into VU1 memory.                                                      |
| UNPACK V4_8 UNSIGNED     | No             | Unpack unknown indices 2 into VU1 memory.                                                   |
| STROW                    | No             | Convert position indices below into addresses by adding the address of the positions table. |
| UNPACK V4_16 SIGNED      | No             | Unpack vertex info into VU1 memory (part 2).                                                |
| STROW                    | Yes            | Add tfrag origin position to positions below.                                               |
| STCYCL num=100 wl=1 cl=2 | Yes            | Scatter the vertex positions below so that there is space for the colours.                  |
| UNPACK V3_16 SIGNED      | No             | Unpack positions into VU1 memory (part 2).                                                  |

### LOD 0 Only

Note that the VIF is still in the state setup by the STCYCL and STROW commands from the end of the last command list at the beginning of this command list.

| VIF Code                 | Always Present | Explanation                                                                                 |
| ------------------------ | -------------- | ------------------------------------------------------------------------------------------- |
| UNPACK V3_16 SIGNED      | No             | Unpack positions into VU1 memory (part 3).                                                  |
| STMOD mode=0             | Yes            | Disable offset mode (see STROW)                                                             |
| STCYCL num=100 wl=4 cl=4 | Yes            | Turn off scattering.                                                                        |
| UNPACK V4_8 SIGNED       | Yes            | Unpack strips into VU1 memory.                                                              |
| STROW                    | Yes            | Converts indices below into addresses by adding the address of the vertex info table.       |
| STMOD mode=1             | Yes            | Enable offset mode (see STROW).                                                             |
| UNPACK V4_8 UNSIGNED     | No             | Unpack indices into VU1 memory.                                                             |
| UNPACK V4_8 UNSIGNED     | No             | Unpack parent indices into VU1 memory.                                                      |
| UNPACK V4_8 UNSIGNED     | No             | Unpack unknown indices 2 into VU1 memory.                                                   |
| STROW                    | No             | Convert position indices below into addresses by adding the address of the positions table. |
| UNPACK V4_16 SIGNED      | No             | Unpack vertex info into VU1 memory (part 3).                                                |
| STMOD mode=0             | Yes            | Disable offset mode (see STROW)                                                             |

## Tfrag Microprogram

### Memory Layout

The table below represents the contents of VU1 data memory while the tfrag microprogram is running. The data shown, with the exception of the matrix and colours, would all be unpacked into VU1 memory from the command lists described above. Double buffering would be used, however only one buffer is shown. In addition, the output buffers for the GS packets are not shown.

| X                          | Y                     | Z                         | W                       |
| -------------------------- | --------------------- | ------------------------- | ----------------------- |
| common_positions_count     |                       | lod_01_positions_count    |                         |
| lod_0_positions_count      |                       | positions_colours_addr    | vertex_info_addr        |
|                            |                       |                           | vertex_info_part_2_addr |
|                            | indices_addr          |                           |                         |
|                            |                       | strips_addr               | texture_ad_gifs_addr    |
|                            |                       |                           |                         |
| matrix[0][0]               | matrix[0][1]          | matrix[0][2]              | matrix[0][3]            |
| matrix[1][0]               | matrix[1][1]          | matrix[1][2]              | matrix[1][3]            |
| matrix[2][0]               | matrix[2][1]          | matrix[2][2]              | matrix[2][3]            |
| matrix[3][0]               | matrix[3][1]          | matrix[3][2]              | matrix[3][3]            |
|                            |                       |                           |                         |
| position[0][x]             | position[0][y]        | position[0][z]            |                         |
| colour[0][r]               | colour[0][g]          | colour[0][b]              |                         |
| ...                        | ...                   | ...                       | ...                     |
| position[n-1][x]           | position[n-1][y]      | position[n-1][z]          |                         |
| colour[n-1][r]             | colour[n-1][g]        | colour[n-1][b]            |                         |
|                            |                       |                           |                         |
| info[0].texcoord[s]        | info[0].texcoord[t]   | info[0].parent_pos_addr   | info[0].pos_addr        |
| ...                        | ...                   | ...                       | ...                     |
| info[n-1].texcoord[s]      | info[n-1].texcoord[t] | info[n-1].parent_pos_addr | info[n-1].pos_addr      |
|                            |                       |                           |                         |
| parent_indices[0]          | parent_indices[1]     | parent_indices[2]         | parent_indices[3]       |
| ...                        | ...                   | ...                       | ...                     |
| ...                        | parent_indices[n-1]   |                           |                         |
| unk_indices[0]             | ...                   | unk_indices[n-1]          |                         |
|                            |                       |                           |                         |
| indices[0]                 | indices[1]            | indices[2]                | indices[3]              |
| ...                        | ...                   | ...                       | ...                     |
| ...                        | indices[n-1]          |                           |                         |
|                            |                       |                           |                         |
| strips[0].vertex_count-128 | 0 (copy ad gifs)      | ad_gif_addrs[0]           |                         |
| strips[1].vertex_count     |                       |                           |                         |
| ...                        | ...                   | ...                       | ...                     |
| strips[?].vertex_count-128 | -128 (do XGKICK)      |                           |                         |
| ...                        | ...                   | ...                       | ...                     |
| strips[n-2].vertex_count   |                       |                           |                         |
| strips[n-1].vertex_count   |                       |                           |                         |
| 0 (end microprogram)       |                       |                           |                         |

Note that for the last section, the strips do not have to come in the order shown (which is provided as an example).

### Strips

The strips form a command list that the microprogram uses to build a GS packet. The list is interpreted like so:

```
for(strip in strips) {
	s32 vertex_count;
	if(strip[x] <= 0) {
		if(strip[x] == 0) {
			xgkick();
			break;
		} else if(strip[y] <= 0) {
			xgkick();
		} else {
			ad_gif_source_offset = strip[z];
			process_ad_gif(ad_gif_source_offset);
		}
		vertex_count = strip[x] + 128;
	} else {
		vertex_count = strip[x];
	}
	process_strip(vertex_count);
}
```

The first strip always encodes an AD GIF.
