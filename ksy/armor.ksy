meta:
  id: armor
  file-extension: WAD
  application: Ratchet & Clank 2
  endian: le
  encoding: ascii
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
      texture_list:
        pos: texture_offset * 0x800
        type: texture_list
        size: texture_size * 0x800
  model:
    seq:
      - id: main_submodel_count
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
      main_submodel_table:
        pos: submodel_table_offset
        type: submodel
        repeat: expr
        repeat-expr: main_submodel_count
      submodel_table_2:
        pos: submodel_table_offset + main_submodel_count * 0x10
        type: submodel
        repeat: expr
        repeat-expr: submodel_count_2
      submodel_table_3:
        pos: submodel_table_offset + (main_submodel_count + submodel_count_2) * 0x10
        type: submodel
        repeat: expr
        repeat-expr: submodel_count_3
      thing_2_inst:
        pos: thing_2
        type: thing_2_obj
        repeat: expr
        repeat-expr: 4
  submodel:
    seq:
      - id: vu1_vif_list_offset
        type: u4
      - id: vu1_vif_list_qwc
        type: u2
      - id: vu1_vif_list_offset_from_end_in_quadwords
        type: u2
      - id: vertex_offset
        type: u4
      - id: thing4
        type: u4
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
  vif_data:
    seq:
      - id: use_bin_vif_to_parse_this
        type: u1
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
  thing_2_obj:
    seq:
      - id: unknown_0
        type: u4
      - id: unknown_4
        type: u4
      - id: unknown_8
        type: u4
      - id: ptr_c
        type: u4
    instances:
      ptr_c_inst:
        pos: ptr_c
        type: u4
  texture_list:
    seq:
      - id: count
        type: u4
      - id: textures
        type: texture
        repeat: expr
        repeat-expr: count
  texture:
    seq:
      - id: header_offset
        type: u4
    instances:
      header:
        pos: header_offset
        type: pif_header
  pif_header:
    seq:
      - id: magic
        type: str
        size: 4
