meta:
  id: armor
  file-extension: WAD
  application: Ratchet & Clank 2
  endian: le
  encoding: ascii
  imports:
    - moby_model_submodel
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
      - id: texture_applications_offset
        type: u4
    instances:
      main_submodel_table:
        pos: submodel_table_offset
        type: moby_model_submodel
        repeat: expr
        repeat-expr: main_submodel_count
      submodel_table_2:
        pos: submodel_table_offset + main_submodel_count * 0x10
        type: moby_model_submodel
        repeat: expr
        repeat-expr: submodel_count_2
      submodel_table_3:
        pos: submodel_table_offset + (main_submodel_count + submodel_count_2) * 0x10
        type: moby_model_submodel
        repeat: expr
        repeat-expr: submodel_count_3
      texture_applications:
        pos: texture_applications_offset
        type: texture_application
        repeat: until
        repeat-until: _.composite_pointer_and_end_condition >= 0x80000000
  texture_application:
    seq:
      - id: texture_indices
        type: u1
        repeat: expr
        repeat-expr: 12
      - id: composite_pointer_and_end_condition
        type: u4
    instances:
      texture_data:
        pos: composite_pointer_and_end_condition & 0x00ffffff
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
