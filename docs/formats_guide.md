# Wrench Formats Guide

## Table of Contents

- [Table of Contents](#table-of-contents)
- [Disc Table of Contents](#disc-table-of-contents)
- [Model Formats](#model-formats)
	- [Graphics on the PS2](#graphics-on-the-ps2)
	- [Moby Models](#moby-models)
- [WAD Compression](#wad-compression)

## Disc Table of Contents

### R&C2, R&C3 and Deadlocked

The table of contents begins with a few sections corresponding to non-level files. For example `ARMOR.WAD` and `MISC.WAD`. These start out like so:

| Offset | Name        | Type | Description                     |
| ------ | ----        | ---- | -----------                     |
| 0x0    | size        | u32  | Size of this ToC section.       |
| 0x4    | base_sector | u32  | Base sector number of the file. |

The next section starts `size` bytes after the beginning of the last section. The body of each section usually consists of offset/size pairs, both in sectors, relative to `base_sector`.

Following all the non-level sections is the level table. Each level entry contains three position/size pairs (again in sectors). The positions are absolute. For each triple, one points to a duplicate of the header for a `LEVEL%d.WAD` file, one for the `AUDIO%d.WAD` file, and another for the `SCENE%d.WAD` file.

For R&C2, the triples are structured like so:

| Offset | Name         | Type | Description                                 |
| ------ | ----         | ---- | -----------                                 |
| 0x0    | level_pos    | u32  | LEVEL%d.WAD ToC header position in sectors. |
| 0x4    | level_size   | u32  | LEVEL%d.WAD size in sectors.                |
| 0x8    | audio_pos    | u32  | AUDIO%d.WAD ToC header position in sectors. |
| 0xc    | audio_size   | u32  | AUDIO%d.WAD size in sectors.                |
| 0x10   | scene_pos    | u32  | SCENE%d.WAD ToC header position in sectors. |
| 0x14   | scene_size   | u32  | SCENE%d.WAD size in sectors.                |

For UYA/Deadlocked:

| Offset | Name         | Type | Description                                 |
| ------ | ----         | ---- | -----------                                 |
| 0x0    | audio_pos    | u32  | AUDIO%d.WAD ToC header position in sectors. |
| 0x4    | audio_size   | u32  | AUDIO%d.WAD size in sectors.                |
| 0x8    | level_pos    | u32  | LEVEL%d.WAD ToC header position in sectors. |
| 0xc    | level_size   | u32  | LEVEL%d.WAD size in sectors.                |
| 0x10   | scene_pos    | u32  | SCENE%d.WAD ToC header position in sectors. |
| 0x14   | scene_size   | u32  | SCENE%d.WAD size in sectors.                |

Each header pointed to has the following fields:

| Offset | Name        | Type | Description                               |
| ------ | ----        | ---- | -----------                               |
| 0x0    | magic       | u32  | Magic identifier?                         |
| 0x4    | base_sector | u32  | Absolute position of the file in sectors. |

## Model Formats

### Graphics on the PS2

For most games, the high-level flow of data when the PS2 is drawing graphics looks something like this:

```
+-----+    +------+    +-----+       +-----+    +----+
|     |--->|      |--->|     | PATH1 |     |    |    |
| RAM |    | VIF1 |    | VU1 |------>| GIF |--->| GS |---> Video Out
|     |-+  |      |-+  |     |       |     |    |    |
+-----+ |  +------+ |  +-----+       |     |    +----+
        |           |          PATH2 |     |
        |           +--------------->|     |
        |                            |     |
        +--------------------------->|     |
                               PATH3 +-----+
```

Model data is stored in RAM in the form of a VIF DMA packet. Static data can be left in memory between frames, however dynamic data will usually be written out as a new command list every frame. This command list is then sent through DMA channel 1 to Vector InterFace unit 1 (VIF1). This unit will interpret the command list, decompressing data and code into VU1's data memory and program memory respectively. It will also control microprogram execution the VU.

The actual VU1 microprogram will usually at minimum perform perspective projection on all the vertices and then write them out to a GS packet which is sent to the Graphics InterFace unit (GIF). This unit will read the GS packet and use it to control the GS during rendering. The Graphics Synthesizer is a fixed-function unit that draws triangles. It also has 2MB of on-board memory which is used to store textures and framebuffers.

This is known as PATH1 rendering. PATH2 rendering bypasses VU1 using a `DIRECT` VIF code. PATH3 rendering bypasses VIF1 entirely.

### Moby Models

As VU1 only has 16k of data memory, models must be split into submodels (sometimes called parts) that can fit in the limited space available. The job of the main model header is thus to point the table of submodels and specify the number of entires in said table.

For skins in `ARMOR.WAD`, this header is structured like so:

| Offset | Name                  | Type | Description                                       |
| ------ | ----                  | ---- | -----------                                       |
| 0x0    | main_submodel_count   | u8   | Number of normal submodels.                       |
| 0x1    | submodel_count_2      | u8   | Number of type 2 submodels.                       |
| 0x2    | submodel_count_3      | u8   | Number of type 3 submodels.                       |
| 0x3    | submodel_count_12     | u8   | main_submodel_count + submodel_count_2?           |
| 0x4    | submodel_table_offset | u32  | Offset of submodel table relative to this header. |
| 0x8    | texture_applications  | u32  | Pointer to list of GIF A+D texture applications.  |

For level mobies:

| Offset | Name                  | Type | Description                                       |
| ------ | ----                  | ---- | -----------                                       |
| 0x0    | submodel_table_offset | u32  | Offset of submodel table relative to this header. |
| ...    | ...                   | ...  | ...                                               |
| 0x7    | submodel_count        | u8   | Number of submodels.                              |
| ...    | ...                   | ...  | ...                                               |
| 0x24   | scale                 | f32  | Scale multiplier.                                 |

Fields that are not listed have an unknown purpose. The texture application table points into the submodel data and is used by the game to replace texture indices with their addresses in GS memory.

Each entry in the main submodel table is structured like so:

| Offset | Name                           | Type | Description                                  |
| ------ | ----                           | ---- | -----------                                  |
| 0x0    | vif_list_offset                | u32  | Offset of VIF command list.                  |
| 0x4    | vif_list_quadword_count        | u16  | Size of VIF command list in 16 byte units.   |
| 0x6    | vif_list_texture_unpack_offset | u16  | Offset of third UNPACK packet if one exists. |
| 0x8    | vertex_offset                  | u32  | Offset of vertex table header.               |
| 0xc    | vertex_data_quadword_count     | u8   | Size of vertex data in 16 byte units.        |
| 0xd    | unknown_d                      | u8   | (0xf + transfer_vertex_count * 6) / 0x10     |
| 0xe    | unknown_e                      | u8   | (3 + transfer_vertex_count) / 4              |
| 0xf    | transfer_vertex_count          | u8   | Number of vertices sent to VU1 memory.       |

The VIF command list consists of two to three `UNPACK` commands, sometimes with `NOP` commands between them. The first `UNPACK` command stores ST coordinates. The vertices sent to VU memory and the ST coordinates are indexed into the same way.

The second `UNPACK` packet is the index buffer. The first four bytes are special (in the compressed data), the second one is the relative quadword offset to the third `UNPACK` in VU memory if it exists. The rest of the bytes are 1-indexed indices into the vertex buffer in VU memory. Note that more vertex positions are sent to VU memory than are stored directly in the vertex table. At the time of writing, I'm not entirely sure how this works, although the additional vertex positions seem to be duplicates. This makes some amount of sense I think, since it means they can have different ST coordinates.

If an index of zero appears, the VU1 microprogram will copy 0x40 bytes from the third UNPACK to the output GS packet as GIF A+D data. This A+D data can be used for setting arbitrary GS registers. For example, it is used to change the active texture.

Additionally, only the low 7 bits of each byte is actually used as an index. The sign bit is used to determine if there should be a GS drawing kick. This is useful since the vertices are sent to the GS as tristrips, so we can just skip drawing a triangle instead of inserting degenerate triangles. If the sign bit is set, there will be no drawing kick.

The third `UNPACK`, as previously referenced, contains data to change the texture, and is only included if necessary.

The vertex table is structured like so:

| Offset | Name                  | Type | Description                                       |
| ------ | ----                  | ---- | -----------                                       |
| 0x0    | unknown_count_0       | u16  | Unknown count.                                    |
| 0x2    | vertex_count_2        | u16  | These come first in the vertex table.             |
| 0x4    | vertex_count_4        | u16  | These come second in the vertex table.            |
| 0x6    | main_vertex_count     | u16  | These come third in the vertex table.             |
| 0x8    | vertex_count_8        | u16  | This is the weird duplicate vertex type.          |
| 0xa    | transfer_vertex_count | u16  | Usually equal to the other transfer_vertex_count. |
| 0xc    | vertex_table_offset   | u16  | Relative offset of the main vertex table.         |
| 0xe    | unknown_e             | u16  | Padding?                                          |

After the header there's an array of unknown `u16`s of length `unknown_count_9`. After that there's an array of more unknown `u16`s of length `vertex_count_8`. The main vertex table has a size of `vertex_count_2 + vertex_count_4 + main_vertex_count`.

Each vertex in the main vertex table is structured like so:

| Offset | Name                  | Type | Description                                       |
| ------ | ----                  | ---- | -----------                                       |
| ...    | ...                   | ...  | ...                                               |
| 0xa    | x                     | i16  | X component of the vertex position.               |
| 0xc    | y                     | i16  | Y component of the vertex position.               |
| 0xe    | z                     | i16  | Z component of the vertex position.               |

## WAD Compression

The game uses a LZ compression scheme to store assets and various other pieces of data. The header is structured like so:

| Offset | Name                  | Type    | Description                                  |
| ------ | ----                  | ------- | -----------                                  |
| 0x0    | magic                 | char[3] | Always equal to "WAD" in ASCII.              |
| 0x3    | compressed_size       | u32     | File size when compressed, including header. |
| 0x7    | padding               | u8[9]   | Padding.                                     |

The compressed data follows immediately after the padding field and consists of a stream of packets. The first byte of each packet is the flag byte, which controls how the rest of the packet is decoded.

The packet types are listed below:

| Condition                               | Type          | Bitstream                                               | Decompressed Data                                                       |
| ---------                               | ----          | ---------                                               | -----------------                                                       |
| flag == 0 && last_flag >= 0x10          | Big Literal   | 00000000 + N[8] + L[(N+18)\*8]                          | L                                                                       |
| 0x1 <= flag <= 0xf && last_flag >= 0x10 | Small Literal | 0000 + N[4] + L[(N+3)\*8]                               | L                                                                       |
| 0x11 <= flag <= 0x1f (implies M >= 1)   | Dummy         | 00010 + M[3] + 000000 + N[2] + 00000000 + L[N\*8]       | L                                                                       |
| flag == 0x10 \|\| flag == 0x18          | Far Match     | 0001 + A[1] + 000 + M[8] + B[6] + N[2] + C[8] + L[N\*8] | M+9 bytes from dest_pos-0x4000-A\*0x800-B-C\*0x40 in the output plus L. |
| 0x11 <= flag <= 0x1f (implies M >= 1)   | Far Match     | 0001 + A[1] + M[3] + B[6] + N[2] + C[8] + L[N\*8]       | M+2 bytes from dest_pos-0x4000-A\*0x800-B-C\*0x40 in the output plus L. |
| flag == 0x20                            | Bigger Match  | 00100000 + M[8] + A[6] + N[2] + B[8] + L[N\*8]          | M+33 bytes from dest_pos-A-B\*0x40 in the output plus L.                |
| 0x21 <= flag <= 0x3f (implies M >= 1)   | Big Match     | 001 + M[5] + A[6] + N[2] + B[8] + L[N\*8]               | M+2 bytes from dest_pos-A-B\*0x40 in the output plus L.                 |
| 0x40 <= flag <= 0xff (implies M >= 2)   | Little Match  | M[3] + A[3] + N[2] + B[8] + L[N\*8]                     | M+1 bytes from dest_pos-B\*8-A-1 in the output plus L.                  |

where `A[B]` means the bitstream contains a field `A` of length `B` bits, `A + B` means concatenate `A` and `B` and `dest_pos` is equal to the position indicator of the output/decompressed stream.

*Important: I'm still not entirely sure about the dummy/far match packets. There might be edge cases not represented here.*

Normal compression packets cannot cross a 0x2000 byte boundary. I think this is because of how the game buffers the compressed data into the EE's scratchpad memory. To get around this, Insomniac's compressor inserts a packet in the form `12 00 00` (in hex) followed by `0xEE`s until `dest_pos % 0x2000 == 0x10` (where `dest_pos` is offset 0x10 bytes by the WAD header). Additionally, two literal packets in a row is not allowed. To get around this, you can insert a dummy `11 00 00` packet between them (you can encode a tiny literal in this packet). The `N[2] + ... + L[N*8]` string at the end of the bitstream for the match packets is a mechanism to squash a tiny literal (3 bytes or smaller) into the previous packet.
