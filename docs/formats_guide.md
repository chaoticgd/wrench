# Wrench Formats Guide

## Table of contents

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
| 0x0    | level_pos    | u32  | LEVEL%d.WAD ToC header position in sectors. |
| 0x4    | level_size   | u32  | LEVEL%d.WAD size in sectors.                |
| 0x0    | scene_pos    | u32  | SCENE%d.WAD ToC header position in sectors. |
| 0x4    | scene_size   | u32  | SCENE%d.WAD size in sectors.                |

Each header pointed to has the following fields:

| Offset | Name        | Type | Description                               |
| ------ | ----        | ---- | -----------                               |
| 0x0    | magic       | u32  | Magic identifier?                         |
| 0x4    | base_sector | u32  | Absolute position of the file in sectors. |
