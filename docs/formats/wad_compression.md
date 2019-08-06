# WAD Compression

The compressed data is divided up into packets that each start with a flag byte. This flag byte determines how the rest of the packet is decoded. This may involve reading data from an earlier position in the output buffer, which means the data must be decompressed sequentially. Different packets of the same type may have different lengths as certain fields may go unused.

See [src/formats/wad.cpp](../../src/formats/wad.cpp) for a C++ implementation. It currently fails to decompress some files, so some of this is probably wrong, especially packet type C. Additionally, the WAD files on the game disc may contain compressed WAD segments, but they won't be at 0x0 within the file.

The below psuedocode only gives a high-level overview of the algorithm. You should read the source code referenced above if anything is unclear.

## File

| Offset                                    | Size   | Type    | Name            | Always Present | Comment                                   |
|-------------------------------------------|--------|---------|-----------------|----------------|-------------------------------------------|
| 0x0                                       | 0x10   | header  | header          | Yes            | The main file header.                     |
| &nbsp; 0x0                                | 0x3    | char[3] | magic           | Yes            | Equals "WAD" in ASCII.                    |
| &nbsp; 0x3                                | 0x4    | u32     | compressed_size | Yes            | Total size in bytes including the header. |
| &nbsp; 0x7                                | 0x9    | N/A     | pad             | Yes            | Padding.                                  |
| 0x10                                      | Varies | init    | init            | Yes            | Initialise the output buffer.             |
| &nbsp; 0x10                               | 0x1    | u8      | init_size       | Yes            | Size to read in.                          |
| &nbsp; 0x11                               | 0x1    | u8      | init_big_size   | No             | If init_size == 0, size to read in - 0xf. |
| offsetof(init) + sizeof(init)             | Varies | packet  | packet_1        | No             | The first data packet.                    |
| offsetof(packet_1) + sizeof(packet_1)     | Varies | packet  | packet_2        | No             | The second data packet.                   |
| ...                                       | ...    | ...     | ...             | ...            | ...                                       |
| offsetof(packet_n-1) + sizeof(packet_n-1) | Varies | packet  | packet_n        | No             | The nth data packet.                      |

Decompression:

1. Read header.
2. Read init_size.
3. If init_size == 0,

	3.1. Read init_big_size.

	3.2. starting_size = init_big_size + 0xf.
	
4. If init_size != 0, starting_size = init_size
5. Copy (starting_size + 3) bytes from the source stream to the desination stream.
6. While the source position indicator < compressed_size,

	6.1. Read in flag.

	6.2. If flag >= 0x40, read packet type A.

	6.3. If 0x1f < flag < 0x40, read packet type B.

	6.4. If flag <= 0x1f, read packet type C.

## Packet Type A (flag >= 0x40)

| Offset                 | Size    | Type     | Name          | Always Present | Comment                                                                                        |
|------------------------|---------|----------|---------------|----------------|------------------------------------------------------------------------------------------------|
| 0x0                    | 0x1     | Bitfield | flag          | Yes            | flag == bytes_to_copy (3 bits) + pos_minor (3 bits) + snd_pos (2 bits)                         |
| &nbsp; 0x0:0-2         | 3/8     | u3       | bytes_to_copy | Yes            | The number of bytes to copy from the destination stream minus 1                                |
| &nbsp; 0x0:3-5         | 3/8     | u3       | pos_minor     | Yes            | Used to determine the lookback offset.                                                         |
| &nbsp; 0x0:6-7         | 2/8     | u2       | snd_pos       | Yes            | Used to determine whenther an additional 3 bytes should be copied from the destination stream. |
| 0x1                    | 0x1     | u8       | pos_major     | Yes            | Used to determine the lookback offset.                                                         |
| 0x2                    | snd_pos | u8[]     | immediate     | No             | Data to copy.                                                                                  |
| Varies                 | 0x1     | u8       | decision      | No             | Used when reading from the source buffer.                                                      |
| offsetof(decision) + 1 | 0x1     | u8       | big_decision  | No             | Used if decision needs to be large.                                                            |

Decompression:

1. lookback_offset = {current pos in output file} - pos_major * 8 - pos_minor - 1
2. Go to read_from_dest.

## Packet Type B (0x1f < flag < 0x40)

| Offset                  | Size     | Type     | Name          | Always Present | Comment                                                                                        |
|-------------------------|----------|----------|---------------|----------------|------------------------------------------------------------------------------------------------|
| 0x0                     | 0x1      | Bitfield | flag          | Yes            | flag == packet_type (3 bits) + bytes (5 bits)                                                  |
| &nbsp; 0x0:0-2          | 3/8      | u3       | packet_type   | Yes            | Used to make the value of the flag fall within the ranges needed for the desired packet type.  |
| &nbsp; 0x0:3-7          | 5/8      | u5       | bytes         | Yes            | Number of bytes to read from destination stream minus 2.                                       |
| 0x1                     | 0x1      | u8       | big_bytes     | No             | Number of bytes to read from destination stream minus 33 if bytes is 0.                        |
| 0x1 or 0x2              | 0x1      | Bitfield | pos_minor_snd | Yes            | Used to determine the lookback offset and snd_pos.                                             |
| &nbsp; (0x1 or 0x2):0-5 | 6/8      | u6       | pos_minor     | Yes            | Used to determine the lookback offset.                                                         |
| &nbsp; (0x1 or 0x2):6-7 | 2/8      | u2       | snd_pos       | Yes            | Used to determine whenther an additional 3 bytes should be copied from the destination stream. |
| 0x2 or 0x3              | 0x1      | u8       | pos_major     | Yes            | Used to determine the lookback offset.                                                         |
| 0x3 or 0x4              | snd_pos  | u8[]     | immediate     | No             | Data to copy.                                                                                  |
| Varies                  | 0x1      | u8       | decision      | No             | Used when reading from the source buffer.                                                      |
| offsetof(decision) + 1  | 0x1      | u8       | big_decision  | No             | Used if decision needs to be large.                                                            |

Decompression:

1. If bytes == 0,

	1.1. Read big_bytes.

	1.2. bytes_to_copy = big_bytes + 33.

2. If bytes != 0, bytes_to_copy = bytes + 2.
3. lookback_offset = {current pos in output file} - pos_major * 0x40 - pos_minor - 1
4. Go to read_from_dest.

## Packet Type C (flag <= 0x1f)

| Offset                  | Size    | Type     | Name          | Always Present | Comment                                                                                        |
|-------------------------|---------|----------|---------------|----------------|------------------------------------------------------------------------------------------------|
| 0x0                     | 0x1     | Bitfield | flag          | Yes            | flag == packet_type (4 bits) + pos_major (1 bit) + bytes (3 bits)                              |
| &nbsp; 0x0:0-3          | 4/8     | u4       | packet_type   | Yes            | Used to make the value of the flag fall within the ranges needed for the desired packet type.  |
| &nbsp; 0x0:4            | 1/8     | u1       | pos_major     | Yes            | Used to determine lookback offset.                                                             |
| &nbsp; 0x0:5-7          | 3/8     | u3       | bytes         | Yes            | Used to determine number of bytes to copy.                                                     |
| 0x1                     | 0x1     | u8       | big_bytes     | No             | If bytes == 0.                                                                                 |
| 0x1 or 0x2              | 0x1     | Bitfield | pos_mid_snd   | Yes            | Determine number of bytes to copy from source or lookback_offset depending  on value.          |
| &nbsp; (0x1 or 0x2):0-5 | 6/8     | u6       | pos_mid       | Yes            | Used to determine the lookback offset.                                                         |
| &nbsp; (0x1 or 0x2):6-7 | 2/8     | u2       | snd_pos       | Yes            | Used to determine whenther an additional 3 bytes should be copied from the destination stream. |
| 0x2 or 0x3              | 0x1     | u8       | pos_minor     | Yes            | Used to determine lookback_offset.                                                             |
| 0x3 or 0x4              | snd_pos | u8[]     | immediate     | No             | Data to copy.                                                                                  |
| Varies                  | 0x1     | u8       | decision      | No             | Used when reading from the source buffer.                                                      |
| offsetof(decision) + 1  | 0x1     | u8       | big_decision  | No             | Used if decision needs to be large.                                                            |


Decompression

1. If pos_mid_snd > 0 and flag_byte == 0x11,

	1.1. Read big_bytes.

	1.2. bytes_to_copy = big_bytes + 7.

2. Otherwise,

	2.1. bytes_to_copy = bytes
	
	2.2. lookback_offset = {current pos in output file} = -pos_major * 0x800 - pos_mid - pos_minor * 0x40

	2.2. If lookback_offset != {current pos in output file}

		2.2.1. bytes_to_copy += 2
		
		2.2.2. lookback_offset -= 0x4000

		2.2.3. Go to read_from_dest
	
	2.3. Otherwise if bytes_to_copy == 1,

		2.3.1. Go to read_from_src.
	
	2.4. Otherwise,

		2.4.1. While {source position indicator} % 0x1000 != 0x10, advance source position indicator (padding).

		2.4.1. Go to read_from_src.

## read_from_dest (continutation)

1. Copy bytes_to_copy bytes from destination stream at the lookback offset to the end of the destination stream.
2. If snd_pos != 0, copy snd_pos bytes from immediate to the destination.
3. If snd_pos == 0, go to read_from_src.

## read_from_src (continuation)

1. If decision > 0xf, decision is the flag byte of the next packet.
2. If decision <= 0xf,

	2.1. If decision != 0, num_bytes = decision + 3.

	2.2. If decision == 0, num_bytes = big_decision + 18

	2.3. Copy num_bytes bytes from the source stream to the destination stream.
