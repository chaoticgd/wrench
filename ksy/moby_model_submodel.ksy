meta:
  id: moby_model_submodel
  endian: le
  encoding: ascii
seq:
  - id: vu1_vif_list_offset
    type: u4
  - id: vu1_vif_list_qwc
    type: u2
  - id: vu1_vif_list_offset_from_end_in_quadwords
    type: u2
  - id: vertex_offset
    type: u4
  - id: vertex_table_qwc
    type: u1
  - id: thing4
    type: u1
  - id: thing5
    type: u1
  - id: transfer_vertex_count
    type: u1
instances:
      vu1_vif_list:
        pos: vu1_vif_list_offset
        type: vif_data
        repeat: expr
        repeat-expr: (vu1_vif_list_qwc - vu1_vif_list_offset_from_end_in_quadwords) * 0x10
      vu1_vif_list_texture_data:
        pos: vu1_vif_list_offset + (vu1_vif_list_qwc - vu1_vif_list_offset_from_end_in_quadwords) * 0x10
        type: vif_data
        repeat: expr
        repeat-expr: vu1_vif_list_offset_from_end_in_quadwords * 0x10
      vertex_list:
        pos: vertex_offset
        type: vertex_header
types:
  vif_data:
    seq:
      - id: use_bin_vif_to_parse_this
        type: u1
        doc: VIF DMA list. Use bin/vif to parse this e.g. ./bin/vif ARMOR.WAD -o 0x0ff537
  vertex_header:
    seq:
      - id: unknown_count_0
        type: u2
      - id: vertex_count_2
        type: u2
      - id: vertex_count_4
        type: u2
      - id: main_vertex_count
        type: u2
      - id: vertex_count_8
        type: u2
      - id: transfer_vertex_count
        type: u2
      - id: vertex_table_offset
        type: u2
      - id: unknown_e
        type: u2
      - id: unknown_0_item
        type: u2
        repeat: expr
        repeat-expr: unknown_count_0
      - id: padding
        type: u2
        repeat: expr
        repeat-expr: 4 - ((unknown_count_0 % 4 == 0) ? 4 : (unknown_count_0 % 4))
      - id: vertex_table_8
        type: u2
        repeat: expr
        repeat-expr: vertex_count_8
    instances:
      vertex_table_2:
        pos: _parent.vertex_offset + vertex_table_offset
        type: vertex
        repeat: expr
        repeat-expr: vertex_count_2
      vertex_table_4:
        pos: _parent.vertex_offset + vertex_table_offset + vertex_count_2 * 0x10
        type: vertex
        repeat: expr
        repeat-expr: vertex_count_4
      main_vertex_table:
        pos: _parent.vertex_offset + vertex_table_offset + (vertex_count_2 + vertex_count_4) * 0x10
        type: vertex
        repeat: expr
        repeat-expr: main_vertex_count
  vertex:
    seq:
      - id: unknown_0
        type: u1
      - id: unknown_1
        type: u1
      - id: unknown_2
        type: u1
      - id: unknown_3
        type: u1
      - id: unknown_4
        type: u1
      - id: unknown_5
        type: u1
      - id: unknown_6
        type: u1
      - id: unknown_7
        type: u1
      - id: unknown_8
        type: u1
      - id: unknown_9
        type: u1
      - id: x
        type: s2
      - id: y
        type: s2
      - id: z
        type: s2
