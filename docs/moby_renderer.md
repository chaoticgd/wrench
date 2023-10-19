# Moby Renderer

## File Layout

- Header
- Animation sequence pointers
- Bangles
- Corncob
- Animation sequences
- Submesh table
- Collision
- Skeleton
- Transforms
- Joints
- Sound definitions
- High LOD packets
- Low LOD packets
- Metal packets
- Bangle packets
- Team palettes (UYA and DL only)
- GIF usage table

## Class Header

| Offset | Name                | Type  | Description                                                                                       |
| ------ | ------------------- | ----- | ------------------------------------------------------------------------------------------------- |
| 0x00   | packet_table_offset | s32   | Pointer to a table of packet headers.                                                             |
| 0x04   | high_lod_count      | u8    | The number of high LOD packets.                                                                   |
| 0x5    | low_lod_count       | u8    | The number of low LOD packets, which come right after the high LOD packets.                       |
| 0x6    | metal_count         | u8    | The number of metal packets. These are used to give models a shiny metallic effect.               |
| 0x7    | metal_begin         | u8    | The index of the first metal packet.                                                              |
| 0x8    | sequence_count      | u8    | The number of animations.                                                                         |
| 0xd    | sound_count         | u8    | The number of sound definitions.                                                                  |
| 0xe    | lod_trans           | u8    |                                                                                                   |
| 0xf    | shadow              | u8    | Number of shadow entries.                                                                         |
| 0x10   | collision           | s32   | Pointer to collision information.                                                                 |
| 0x14   | skeleton            | s32   | Pointer to skeleton matrices.                                                                     |
| 0x18   | common_trans        | s32   |                                                                                                   |
| 0x1c   | joints              | s32   |                                                                                                   |
| 0x20   | gif_usage           | s32   | Pointer to table of AD GIF primitives stored in the GIF command lists.                            |
| 0x24   | scale               | f32   | Scale multiplier for the moby class. Used to counteract the quantization of the vertex positions. |
| 0x28   | sound_defs          | s32   | Pointer to sound definitions.                                                                     |
| 0x2c   | bangles             | u8    | Quadword offset of bangles.                                                                       |
| 0x2d   | mip_dist            | u8    | Mipmap distance.                                                                                  |
| 0x2e   | corncob             | s16   | Pointer to data for the corn system.                                                              |
| 0x30   | bounding_sphere     | vec4f | Position (x, y, z) and radius (w) of sphere enclosing the moby.                                   |
| 0x40   | glow_rgba           | s32   |                                                                                                   |
| 0x44   | mode_bits           | s16   | Bit flags.                                                                                        |
| 0x46   | type                | u8    |                                                                                                   |
| 0x47   | mode_bits2          | u8    | More bit flags.                                                                                   |

## Submeshes

### Submesh Header

As VU1 only has 16k of data memory, meshes must be split into packets that can fit in the limited space available. Each packet consists of a VIF command list and a vertex table, and its entry in the packet table is structured like so:

| Offset | Name                           | Type | Description                                   |
| ------ | ------------------------------ | ---- | --------------------------------------------- |
| 0x0    | vif_list_offset                | u32  | Pointer to VIF command list.                  |
| 0x4    | vif_list_size                  | u16  | Size of VIF command list in 16 byte units.    |
| 0x6    | vif_list_texture_unpack_offset | u16  | Pointer to third UNPACK packet if one exists. |
| 0x8    | vertex_offset                  | u32  | Pointer to vertex table header.               |
| 0xc    | vertex_data_size               | u8   | Size of vertex data in 16 byte units.         |
| 0xd    | unknown_d                      | u8   | (0xf + transfer_vertex_count * 6) / 0x10      |
| 0xe    | unknown_e                      | u8   | (3 + transfer_vertex_count) / 4               |
| 0xf    | transfer_vertex_count          | u8   | Number of vertices sent to VU1 memory.        |

### VIF Command List

The VIF command list consists of two to three `UNPACK` commands, sometimes with `NOP` commands between them. The first `UNPACK` command stores ST coordinates. The vertices sent to VU memory and the ST coordinates are indexed into the same way.

The second `UNPACK` packet is the index buffer. The first four bytes are special (in the compressed data), the second one is the relative quadword offset to the third `UNPACK` in VU memory if it exists and the third one is the first secret index. The rest of the bytes are 1-indexed indices into the vertex buffer in VU memory. Note that more vertex positions are sent to VU memory than are stored directly in the vertex table. The extra vertices are populated using the duplicate vertices array int the vertex table. Each element in this array is a bitshifted index into an intermediate buffer, which will be discussed later.

If an index of zero appears, the VU1 microprogram will copy 0x40 bytes from the third UNPACK to the output GS packet as GIF A+D data. This A+D data can be used for setting arbitrary GS registers. In practice it is used to change the active texture. In addition, a secret index will be read and used as the actual index (although the primitive restart bit will always be set in this case). The first secret index is stored in the before the array of indices, and the rest are stored in the third UNPACK, stuffed into some space that would normally be padding.

Additionally, only the low 7 bits of each byte is actually used as an index. The sign bit is used to determine if there should be a GS drawing kick. This is useful since the vertices are sent to the GS as tristrips, so we can just skip drawing a triangle instead of inserting degenerate triangles. If the sign bit is set, there will be no drawing kick.

The third `UNPACK`, as previously referenced, contains data to change the texture as well as some of the secret indices, and is only included if necessary.

### Vertex Table

The vertex table is processed on the EE core. It starts with a header is structured like so:

| Offset | Name                         | Type | Description                                                                                    |
| ------ | ---------------------------- | ---- | ---------------------------------------------------------------------------------------------- |
| 0x0    | matrix_transfer_count        | u16  | Number of matrices transferred to VU0 memory before the packet is processed.                   |
| 0x2    | two_way_blend_vertex_count   | u16  | Number of vertices that do a two-way blend operation. These come first in the vertex table.    |
| 0x4    | three_way_blend_vertex_count | u16  | Number of vertices that do a three-way blend operation. These come second in the vertex table. |
| 0x6    | main_vertex_count            | u16  | Number of vertices that don't do a blend operation. These come third in the vertex table.      |
| 0x8    | duplicate_vertex_count       | u16  | This is the weird duplicate vertex type.                                                       |
| 0xa    | transfer_vertex_count        | u16  | Usually equal to the other transfer_vertex_count.                                              |
| 0xc    | vertex_table_offset          | u16  | Relative offset of the main vertex table.                                                      |
| 0xe    | glow_rgba                    | u16  |                                                                                                |

Note that for R&C1 each u16 in the above table is instead a u32.

After the header there's an array of joint indices and VU0 destination addresses with `matrix_transfer_count` elements. After that there's an array specifying which vertices should be duplicated so that the final vertex buffer matches up with the ST coordinates stored in the VIF command list. The main vertex table has a size of `two_way_blend_vertex_count + three_way_blend_vertex_count + main_vertex_count`.

Each vertex is structured in one of three ways, which are shown below with **bit offsets** for each of the fields:

<table>
	<th>
		<td colspan="2">112</td><td colspan="2">96</td>
		<td colspan="2">80</td><td>72</td><td>64</td>
		<td>56</td><td>48</td><td>40</td><td>32</td>
		<td>24</td><td>16</td><td><b>9</b</td><td>0</td>
	</th>
	<tr>
		<td>3-way blend</td>
		<td colspan="2">Z</td>
		<td colspan="2">Y</td>
		<td colspan="2">X</td>
		<td colspan="1">NA</td>
		<td colspan="1">NE</td>
		<td colspan="1">SB</td>
		<td colspan="1">ST</td>
		<td colspan="1">W2</td>
		<td colspan="1">W1</td>
		<td colspan="1">L2</td>
		<td colspan="1">L1</td>
		<td colspan="1">JI</td>
		<td colspan="1">ID</td>
	</tr>
	<tr>
		<td>2-way blend</td>
		<td colspan="2">Z</td>
		<td colspan="2">Y</td>
		<td colspan="2">X</td>
		<td colspan="1">NA</td>
		<td colspan="1">NE</td>
		<td colspan="1">SB</td>
		<td colspan="1">W3</td>
		<td colspan="1">W2</td>
		<td colspan="1">W1</td>
		<td colspan="1">L2</td>
		<td colspan="1">L1</td>
		<td colspan="1">L3</td>
		<td colspan="1">ID</td>
	</tr>
	<tr>
		<td>No blend</td>
		<td colspan="2">Z</td>
		<td colspan="2">Y</td>
		<td colspan="2">X</td>
		<td colspan="1">NA</td>
		<td colspan="1">NE</td>
		<td colspan="1">U</td>
		<td colspan="1">U</td>
		<td colspan="1">U</td>
		<td colspan="1">U</td>
		<td colspan="1">ST</td>
		<td colspan="1">L1</td>
		<td colspan="1">JI</td>
		<td colspan="1">ID</td>
	</tr>
</table>

Note that the above table shows the fields in little endian order. In a hex editor, the fields would appear in the opposite order. The legend below explains the meaning of each field:

| Field               | Explanation                                                                                             |
| ------------------- | ------------------------------------------------------------------------------------------------------- |
| `X`, `Y` and `Z`    | Vertex position.                                                                                        |
| `NA` and `NE`       | Vertex normal, stored as spherical coordinates (azimuth and elevation). Doesn't use the ISO convention. |
| `SB`                | Address in VU0 memory to store the newly blended matrix.                                                |
| `ST`                | Address in VU0 memory to store the matrix transferred from the scratchpad.                              |
| `W1`, `W2` and `W3` | Weights for blending.                                                                                   |
| `L1`, `L2` and `L3` | Addresses in VU0 memory to load the matrices to be blended from.                                        |
| `JI`                | Joint index used to load the matrix to be transferred to VU0 memory.                                    |
| `ID`                | ID number (explained below).                                                                            |
| `U`                 | Unused byte.                                                                                            |

The first 9 bits of each vertex v[i] is an ID which determines at which index vertex v[i-7] will be written into an intermediate buffer if at all. There are epilogue vertices off the end of the buffer which are used to store the last few ID values. If these vertices are used up, the remaining IDs will be stored in the second half of the last epilogue vertex.

### Blending

The games support skeletal animation with 3 joints per vertex. Psuedocode for how the games perform blending is shown below:

```
mtx4 VU0mem[64];
mtx4 SPR[256];
...
for(packet in packets) {
	for(transfer in packet.matrix_transfers) {
		VU0mem[transfer.address/4] = SPR[transfer.joint_index];
	}
	for(v in packet.vertices_that_do_two_way_blends) {
		VU0mem[v.ST/4] = SPR[v.JI];
		v.matrix = VU0mem[v.L1/4]*(v.W1/255.f)+VU0mem[v.L2/4]*(v.W2/255.f);
		VU0mem[v.SB/4] = v.matrix;
	}
	for(v in packet.vertices_that_do_three_way_blends) {
		v.matrix = VU0mem[v.L1/4]*(v.W1/255.f)+VU0mem[v.L2/4]*(v.W2/255.f)+VU0mem[v.L3/4]*(v.W3/255.f);
		VU0mem[v.SB/4] = v.matrix;
	}
	for(v in packet.vertices_that_dont_do_blends) {
		VU0mem[v.ST/4] = SPR[v.JI];
		v.matrix = VU0mem[v.L1/4];
	}
}
```

where `VU0mem` refers to Vector Unit 0's data memory, `SPR` refers to the EE core's scratchpad memory and `v.matrix` refers to the blended matrix to be applied to a given vertex. In the actual game the implementation of the above logic is split across a routine on the EE core and a VU0 microprogram. These two parts communicate via shared registers.

Note that the state of VU0 memory is preserved between packets, and Insomniac's moby exporter took advantage of this. Also note that just because a vertex does no blends, that does not mean that it isn't animated using a previously blended matrix.
