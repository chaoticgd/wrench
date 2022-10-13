# Shrub Renderer

## Shrub Microprogram

### Operation

The following operations are performed in seperate loops:

1. Copy GIF tags to GS packet.
2. Copy GS AD texture data to GS packet.
3. Convert vertex positions to floats.
4. Transform vertices and write to GS packet.

### Memory Map

| Address (quadwords) | Size (quadwords) | Description           |
| ------------------- | ---------------- | --------------------- |
| 0x0                 | 0x2              | Bookkeeping           |
| 0x2                 | 0x76             | Input Buffer 1        |
| 0x78                | 0x76             | Input Buffer 2        |
| 0xee                | 0x11a            | Instances (x10)       |
| 0x208               | 0x1f8            | Output Buffer         |

### Data Structures

#### Input Buffer

The data written by the UNPACK commands stored in the shrub packets.

| Offset (quadwords)           | Size (quadwords) | Description                  |
| ---------------------------- | ---------------- | ---------------------------- |
| 0                            | 0x1              | Header                       |
| 1                            | gif_tag_count    | GIF tags                     |
| 1 + gif_tag_count            | texture_count    | GS AD data                   |
| vertex_offset                | vertex_count     | Vertex Positions + offsets   |
| vertex_offset + vertex_count | vertex_count     | Vertex STs + colour indices + stopping condition |

#### Instance

| Offset (quadwords) | Size (quadwords) | Description           |
| ------------------ | ---------------- | --------------------- |
| 0x0                | 0x4              | Instance Matrix       |
| 0x4                | 0x18             | Colour Palette        |

### Register Map

#### Integer Registers

| Register | Description           | Lifetime |
| -------- | --------------------- | -------- |
| vi11     | Input buffer pointer. | 1-4      |

#### Floating Point Registers

| Register | Description                    |
| -------- | ------------------------------ |
| vf17     | Addresses to write GS packets. |

## Shrub Near Microprogram

### Memory Map

Same as above.
