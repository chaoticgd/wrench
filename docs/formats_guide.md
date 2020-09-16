# Wrench Formats Guide

## Table of Contents

- [Table of Contents](##table-of-contents)
- [Disc Table of Contents](##disc-table-of-contents)
- [Model Formats](##mdoel-formats)
	- [Graphics on the PS2](##graphics-on-the-ps2)
	- [Moby Models](##moby-models)

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

The VIF command list consists of two to three `UNPACK` commands, sometimes with `NOP` commands between them. The first `UNPACK` command stores ST coordinates. The vertices to VU memory and the ST coordinates are indexed into the same way.

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
| 0xa    | transfer_vertex_couht | u16  | Usually equal to the other transfer_vertex_count. |
| 0xc    | vertex_table_offset   | u16  | Relative offset of the main vertex table.         |
| 0xe    | unknown_e             | u16  | Padding?                                          |

The main vertex table has a size of `vertex_count_2 + vertex_count_4 + main_vertex_count`.
