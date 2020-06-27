meta:
  id: armor
  file-extension: WAD
  application: Ratchet & Clank 2
  endian: le
seq:
  - id: header_size
    type: u4
  - id: base_offset
    type: u4
    doc: Only populated in the ToC, zero elsewhere.
  - id: armor_table
    type: set_of_armor
    repeat: expr
    repeat-expr: (header_size - 8) / 16
types:
  set_of_armor:
    seq:
      - id: model_offset
        type: u4
      - id: model_size
        type: u4
      - id: texture_offset
        type: u4
      - id: texture_size
        type: u4
    instances:
      armor_model:
        pos: model_offset * 0x800
        type: model
        size: model_size * 0x800
  model:
    seq:
      - id: submodel_count_1
        type: u1
      - id: submodel_count_2
        type: u1
      - id: submodel_count_3
        type: u1
      - id: thing_1
        type: u1
      - id: submodel_table_offset
        type: u4
      - id: thing_2
        type: u4
    instances:
      submodel_table:
        pos: submodel_table_offset
        type: submodel
        repeat: expr
        repeat-expr: submodel_count_1 + submodel_count_2 + submodel_count_3
  submodel:
    seq:
      - id: vu1_vif_list_offset
        type: u4
      - id: thing2
        type: u4
      - id: vertex_offset
        type: u4
      - id: thing4
        type: u4
    instances:
      vu1_vif_list:
        pos: vu1_vif_list_offset
        type: vif_list
      vertex_list:
        pos: vertex_offset
        type: vertex_header
  vif_list:
    seq:
      - id: use_bin_vif_to_parse_this
        type: u4
        doc: VIF DMA list. Use bin/vif to parse this e.g. ./bin/vif ARMOR.WAD -o 0x0ff537
  vertex_header:
    seq:
      - id: unknown_0
        type: u4
      - id: unknown_4
        type: u2
      - id: vertex_count
        type: u2
      - id: unknown_8
        type: u4
      - id: vertex_table_offset
        type: u4
    instances:
      vertex_table:
        pos: _parent.vertex_offset + vertex_table_offset
        type: vertex
        repeat: expr
        repeat-expr: vertex_count
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
