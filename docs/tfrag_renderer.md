# Tfrag Renderer

## Tfrag Microprogram

### Memory Layout

|                          |                       |                         |                         |
| ------------------------ | --------------------- | ----------------------- | ----------------------- |
| common_positions_count   |                       | lod_01_positions_count  |                         |
| lod_0_positions_count    |                       | positions_colours_addr  | vertex_info_addr        |
|                          |                       |                         | vertex_info_part_2_addr |
|                          | indices_addr          |                         |                         |
|                          |                       | strips_addr             | texture_ad_gifs_addr    |
|                          |                       |                         |                         |
| matrix[0][0]             | matrix[0][1]          | matrix[0][2]            | matrix[0][3]            |
| matrix[1][0]             | matrix[1][1]          | matrix[1][2]            | matrix[1][3]            |
| matrix[2][0]             | matrix[2][1]          | matrix[2][2]            | matrix[2][3]            |
| matrix[3][0]             | matrix[3][1]          | matrix[3][2]            | matrix[3][3]            |
|                          |                       |                         |                         |
| position[0][x]           | position[0][y]        | position[0][z]          |                         |
| colour[0][r]             | colour[0][g]          | colour[0][b]            |                         |
| ...                      | ...                   | ...                     | ...                     |
| position[n-1][x]         | position[n-1][y]      | position[n-1][z]        |                         |
| colour[n-1][r]           | colour[n-1][g]        | colour[n-1][b]          |                         |
|                          |                       |                         |                         |
| info[0].texcoord[s]      | info[0].texcoord[t]   | info[0].pos_addr_1\*2   | info[0].pos_addr_2\*2   |
| ...                      | ...                   | ...                     | ...                     |
| info[n-1].texcoord[s]    | info[n-1].texcoord[t] | info[n-1].pos_addr_1\*2 | info[n-1].pos_addr_2\*2 |
|                          |                       |                         |                         |
| unk_indices[0]           | unk_indices[1]        | unk_indices[2]          | unk_indices[3]          |
| ...                      | ...                   | ...                     | ...                     |
| ...                      | unk_indices[n-1]      |                         |                         |
|                          |                       |                         |                         |
| indices[0]               | indices[1]            | indices[2]              | indices[3]              |
| ...                      | ...                   | ...                     | ...                     |
| ...                      | indices[n-1]          |                         |                         |
|                          |                       |                         |                         |
| < 0                      | 0 (copy ad gifs)      | ad_gif_addrs[0]         |                         |
| strips[0].vertex_count   |                       |                         |                         |
| strips[1].vertex_count   |                       |                         |                         |
| ...                      | ...                   | ...                     | ...                     |
| < 0                      | -128 (do XGKICK)      |                         |                         |
| ...                      | ...                   | ...                     | ...                     |
| strips[n-2].vertex_count |                       |                         |                         |
| strips[n-1].vertex_count |                       |                         |                         |
| = 0 (end microprogram)   |                       |                         |                         |
