## File Loading

Each game disc has a standard ISO filesystem which is used to access the `SYSTEM.CNF` file required for booting the game, the main executable, and some other files.

Once this executable is loaded all other assets are accessed by their sector number, via raw disk I/O. In the case of R&C2 retail builds these assets are also included on the filesystem, however for retail builds of R&C1, R&C3 and Deadlocked they are not. The sector numbers of these assets are stored in a file that is unofficially called the table of contents, which is itself located at a hardcoded sector number and is not to be confused with the standard CD table of contents. A single sector is `0x800` bytes in size.

## Code

The game's code is stored in both the main ELF file and overlays in the level files. The ELF file contains sections `.vutext`, `core.text`, `core.data`, `core.rdata`, `core.bss` and `core.lit` which remain in memory at all times, as well as sections `.lit`, `.bss`, `.data`, `lvl.vtbl`, `lvl.camvtbl`, `lvl.sndvtbl` and `.text` which are overwritten when a level is loaded.

The majority of the game's code is stored in the non-core sections. During the level loading process, these sections are loaded from the level files in the form of a custom executable format used by the games. This format consists of a stream of section headers immediately followed by the data. These headers are structured like so:

| Offset | Name         | Type | Description                                       |
| ------ | ------------ | ---- | ------------------------------------------------- |
| 0x0    | dest_address | s32  | The address to copy the section data to.          |
| 0x4    | copy_size    | s32  | The size of the section not including the header. |
| 0x8    | section_type | s32  | The ELF section type. Not used by the game.       |
| 0xc    | entry_point  | s32  | The address of the startlevel function.           |

The `lvl.*vtbl` sections contain tables of function pointers for the various moby, camera and sound classes that are included in a level. For example `lvl.vtbl` contains pointers to all the moby update functions.

Note that in the case of UYA and DL the main ELF file is packed. The actual sections are stored in the same format used for the level overlays, except compressed (see WAD Compression section below). Wrench (versions >= 0.4) will automatically unpack the file while importing the ISO.

## Table of Contents

### R&C1

The table of contents begins at sector number `1500`. It consists of a C struct with sector numbers for all the global assets followed by an array of sector numbers pointing to level headers. In R&C1, there is only a single header for each level, which is stored at the beginning of the level WAD file. These headers contain absolute sector numbers pointing to all the assets needed for a level.

### GC, UYA and DL

The table of contents begins at sector number `1001` (`0x1f4800` in bytes) and in the case of R&C2 is referenced on the filesystem as `RC2.HDR`. The first section of the file consists of the headers for all the global WAD files appended together with no additional padding inserted. These headers all begin like so:

| Offset | Name        | Type | Description                         |
| ------ | ----------- | ---- | ----------------------------------- |
| 0x0    | header_size | s32  | Size of this header.                |
| 0x4    | sector      | s32  | Absolute sector number of the file. |

Following the global headers is the level table. Each level entry contains three position/size pairs (again in sectors). The positions are absolute. For each triple, one points to a duplicate of the header for a `LEVEL%d.WAD` file, one for the `AUDIOn.WAD` file, and another for the `SCENE%d.WAD` file.

For R&C2, the triples are structured like so:

| Offset | Name       | Type | Description                                |
| ------ | ---------- | ---- | ------------------------------------------ |
| 0x0    | level_pos  | s32  | LEVELn.WAD ToC header position in sectors. |
| 0x4    | level_size | s32  | LEVELn.WAD size in sectors.                |
| 0x8    | audio_pos  | s32  | AUDIOn.WAD ToC header position in sectors. |
| 0xc    | audio_size | s32  | AUDIOn.WAD size in sectors.                |
| 0x10   | scene_pos  | s32  | SCENEn.WAD ToC header position in sectors. |
| 0x14   | scene_size | s32  | SCENEn.WAD size in sectors.                |

For UYA/Deadlocked:

| Offset | Name       | Type | Description                                |
| ------ | ---------- | ---- | ------------------------------------------ |
| 0x0    | audio_pos  | s32  | AUDIOn.WAD ToC header position in sectors. |
| 0x4    | audio_size | s32  | AUDIOn.WAD size in sectors.                |
| 0x8    | level_pos  | s32  | LEVELn.WAD ToC header position in sectors. |
| 0xc    | level_size | s32  | LEVELn.WAD size in sectors.                |
| 0x10   | scene_pos  | s32  | SCENEn.WAD ToC header position in sectors. |
| 0x14   | scene_size | s32  | SCENEn.WAD size in sectors.                |

Each header pointed to has the following fields:

| Offset | Name        | Type | Description                               |
| ------ | ----------- | ---- | ----------------------------------------- |
| 0x0    | header_size | s32  | Size of the header in bytes.              |
| 0x4    | sector      | s32  | Absolute position of the file in sectors. |

## Levels

### File Headers

#### R&C1

The header stored on the disc is located at the beginning of the main level file, uses absolute sector numbers, and references sectors before itself on the disc.

Because this header would be useless if directly extracted from the disc, the Wrench Build Tool rewrites the header into 3 seperate headers for the 3 different files that make up a level, using relative sector numbers. These headers were designed to be similar those of R&C2/R&C3/Deadlocked.

<table><tbody>
	<tr><td></td><td>0x0</td><td>0x04</td><td>0x8</td><td>0xc</td></tr>
	<tr><td>0x00</td><td>0x30</td><td>0</td><td>level_number</td><td>0</td></tr>
	<tr><td>0x10</td><td colspan=2>primary</td><td colspan=2>gameplay_ntsc</td></tr>
	<tr><td>0x20</td><td colspan=2>gameplay_pal</td><td colspan=2>occlusion</td></tr>
</tbody></table>

#### GC and UYA

<table><tbody>
	<tr><td></td><td>0x0</td><td>0x04</td><td>0x8</td><td>0xc</td></tr>
	<tr><td>0x00</td><td>0x60</td><td>sector</td><td>level_number</td><td>reverb</td></tr>
	<tr><td>0x10</td><td colspan=2>primary</td><td colspan=2>core_bank</td></tr>
	<tr><td>0x20</td><td colspan=2>gameplay</td><td colspan=2>occlusion</td></tr>
	<tr><td>0x30</td><td colspan=2>chunk[0]</td><td colspan=2>chunk[1]</td></tr>
	<tr><td>0x40</td><td colspan=2>chunk[2]</td><td colspan=2>chunk_sound_banks[0]</td></tr>
	<tr><td>0x50</td><td colspan=2>chunk_sound_banks[1]</td><td colspan=2>chunk_sound_banks[2]</td></tr>
</tbody></table>

## WAD Compression

The game uses an LZ compression scheme to store assets. The header is structured like so:

| Offset | Name            | Type    | Description                                  |
| ------ | --------------- | ------- | -------------------------------------------- |
| 0x0    | magic           | char[3] | Always equal to "WAD" in ASCII.              |
| 0x3    | compressed_size | u32     | File size when compressed, including header. |
| 0x7    | padding         | u8[9]   | Padding.                                     |

The compressed data follows immediately after the padding field and consists of a stream of packets. The first byte of each packet is the flag byte, which controls how the rest of the packet is decoded.

The packet types are listed below:

| Condition                               | Type             | Bitstream                                               | Decompressed Data                                                       |
| --------------------------------------- | ---------------- | ------------------------------------------------------- | ----------------------------------------------------------------------- |
| flag == 0 && last_flag >= 0x10          | Big Literal      | 00000000 + N[8] + L[(N+18)\*8]                          | L                                                                       |
| 0x1 <= flag <= 0xf && last_flag >= 0x10 | Medium Literal   | 0000 + N[4] + L[(N+3)\*8]                               | L                                                                       |
| 0x11 <= flag <= 0x1f (implies M >= 1)   | Little Literal   | 00010 + M[3] + 000000 + N[2] + 00000000 + L[N\*8]       | L                                                                       |
| flag == 0x10 \|\| flag == 0x18          | Big Far Match    | 0001 + A[1] + 000 + M[8] + B[6] + N[2] + C[8] + L[N\*8] | M+9 bytes from dest_pos-0x4000-A\*0x800-B-C\*0x40 in the output plus L. |
| 0x11 <= flag <= 0x1f (implies M >= 1)   | Medium Far Match | 0001 + A[1] + M[3] + B[6] + N[2] + C[8] + L[N\*8]       | M+2 bytes from dest_pos-0x4000-A\*0x800-B-C\*0x40 in the output plus L. |
| flag == 0x20                            | Big Match        | 00100000 + M[8] + A[6] + N[2] + B[8] + L[N\*8]          | M+33 bytes from dest_pos-A-B\*0x40-1 in the output plus L.              |
| 0x21 <= flag <= 0x3f (implies M >= 1)   | Medium Match     | 001 + M[5] + A[6] + N[2] + B[8] + L[N\*8]               | M+2 bytes from dest_pos-A-B\*0x40-1 in the output plus L.               |
| 0x40 <= flag <= 0xff (implies M >= 2)   | Little Match     | M[3] + A[3] + N[2] + B[8] + L[N\*8]                     | M+1 bytes from dest_pos-B\*8-A-1 in the output plus L.                  |

where `A[B]` means the bitstream contains a field `A` of length `B` bits, `A + B` means concatenate `A` and `B` and `dest_pos` is equal to the position indicator of the output/decompressed stream.

*Important: I'm still not entirely sure about the dummy/far match packets. There might be edge cases not represented here.*

Normal compression packets cannot cross a 0x2000 byte boundary. I think this is because of how the game buffers the compressed data into the EE's scratchpad memory. To get around this, Insomniac's compressor inserts a packet in the form `12 00 00` (in hex) followed by `0xEE`s until `dest_pos % 0x2000 == 0x10` (where `dest_pos` is offset 0x10 bytes by the WAD header). Additionally, two literal packets in a row is not allowed. To get around this, you can insert a dummy `11 00 00` packet between them (you can encode a tiny literal in this packet). The `N[2] + ... + L[N*8]` string at the end of the bitstream for the match packets is a mechanism to squash a small literal (3 bytes or smaller) into the previous packet.
