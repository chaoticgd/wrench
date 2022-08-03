# Shrub Renderer

## Shrub Microprogram

### Operation

The following operations are performed in seperate loops:

1. Copy GIF tags to GS packet.
2. Copy GS AD texture data to GS packet.
3. Convert vertex positions to floats.
4. Transform vertices and write to GS packet.

### Memory Map

| Address (quadwords)                 | Size (quadwords)  | Description                   |
| ----------------------------------- | ----------------- | ----------------------------- |
| base                                | 1                 | Header.                       |
| base + 1                            | gif_tag_count     | GIF tags.                     |
| base + 1 + gif_tag_count            | texture_count     | GS AD data.                   |
| base + vertex_offset                | vertex_count      | Vertex positions + addresses. |
| base + vertex_offset + vertex_count | vertex_count      | Vertex STs + ???              |

### Register Map

#### Integer Registers

| Register | Description   | Lifetime |
| -------- | ------------- | -------- |
| vi11     | Base address. | 1-4      |
