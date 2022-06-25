meta:
  id: world_wad
  application: Ratchet & Clank
  endian: le
seq:
  - id: ship_data_offset
    type: u4
  - id: directional_lights_offset
    type: u4
  - id: unknown_8
    type: u4
  - id: unknown_c
    type: u4
  - id: english_strings_offset
    type: u4
  - id: unknown_14
    type: u4
  - id: french_strings_offset
    type: u4
  - id: german_strings_offset
    type: u4
  - id: spanish_strings_offset
    type: u4
  - id: italian_strings_offset
    type: u4
  - id: unknown_strings_offset
    type: u4
  - id: unknown_2c
    type: u4
  - id: unknown_30
    type: u4
  - id: tie_table_offset
    type: u4
  - id: unknown_38
    type: u4
  - id: unknown_3c
    type: u4
  - id: shrub_table_offset
    type: u4
  - id: unknown_44
    type: u4
  - id: unknown_48
    type: u4
  - id: moby_table_offset
    type: u4
  - id: unknown_50
    type: u4
  - id: unknown_54
    type: u4
  - id: unknown_58
    type: u4
  - id: unknown_5c
    type: u4
  - id: unknown_60
    type: u4
  - id: unknown_64
    type: u4
  - id: unknown_68
    type: u4
  - id: unknown_6c
    type: u4
  - id: unknown_70
    type: u4
  - id: unknown_74
    type: u4
  - id: spline_offset
    type: u4
  
instances:
  ship_data:
    pos: ship_data_offset
    type: ship_data
  directional_lights:
    pos: directional_lights_offset
    type: directional_lights
  english_strings:
    pos: english_strings_offset
    type: string_table
  french_strings:
    pos: french_strings_offset
    type: string_table
  german_strings:
    pos: german_strings_offset
    type: string_table
  spanish_strings:
    pos: spanish_strings_offset
    type: string_table
  italian_strings:
    pos: italian_strings_offset
    type: string_table
  unknown_strings:
    pos: unknown_strings_offset
    type: string_table
  ties:
    pos: tie_table_offset
    type: tie_table
  shrubs:
    pos: shrub_table_offset
    type: shrub_table
  mobies:
    pos: moby_table_offset
    type: moby_table
types:
  ship_data:
    seq:
      - id: a
        type: u4
  directional_lights:
    seq:
      - id: b
        type: u4
  string_table:
    seq:
      - id: num_strings
        type: u4
      - id: unknown_4
        type: u4
      - id: elements
        type: string_table_entry
        repeat: expr
        repeat-expr: num_strings
  string_table_entry:
    seq:
      - id: ptr
        type: u4
      - id: id
        type: u4
      - id: unknown_8
        type: u4
      - id: unknown_c
        type: u4
    instances:
      string:
        pos: _parent._parent._io.pos + ptr
        type: strz
        encoding: ASCII
  tie_table:
    seq:
      - id: tie_count
        type: u4
      - id: pad
        type: u4
        repeat: expr
        repeat-expr: 3
      - id: ties
        type: tie
        repeat: expr
        repeat-expr: tie_count
  tie:
    seq:
      - id: unknown_0
        type: u4
      - id: unknown_4
        type: u4
      - id: unknown_8
        type: u4
      - id: unknown_c
        type: u4
      - id: local_to_world
        type: racmat
      - id: unknown_4c
        type: u4
      - id: unknown_50
        type: u4
      - id: uid
        type: s4
      - id: unknown_58
        type: u4
      - id: unknown_5c
        type: u4
  shrub_table:
    seq:
      - id: shrub_count
        type: u4
      - id: pad
        type: u4
        repeat: expr
        repeat-expr: 3
      - id: shrubs
        type: shrub
        repeat: expr
        repeat-expr: shrub_count
  shrub:
    seq:
      - id: unknown_0
        type: u4
      - id: unknown_4
        type: u4
      - id: unknown_8
        type: u4
      - id: unknown_c
        type: u4
      - id: local_to_world
        type: racmat
      - id: unknown_4c
        type: u4
      - id: unknown_50
        type: u4
      - id: unknown_54
        type: u4
      - id: unknown_58
        type: u4
      - id: unknown_5c
        type: u4
      - id: unknown_60
        type: u4
      - id: unknown_64
        type: u4
      - id: unknown_68
        type: u4
      - id: unknown_6c
        type: u4
  moby_table:
    seq:
      - id: moby_count
        type: u4
      - id: pad
        type: u4
        repeat: expr
        repeat-expr: 3
      - id: mobies
        type: moby
        repeat: expr
        repeat-expr: moby_count
  moby:
    seq:
      - id: size
        type: u4
      - id: unknown_4
        type: u4
      - id: unknown_8
        type: u4
      - id: unknown_c
        type: u4
      - id: uid
        type: s4
      - id: unknown_14
        type: u4
      - id: unknown_18
        type: u4
      - id: unknown_1c
        type: u4
      - id: unknown_20
        type: u4
      - id: unknown_24
        type: u4
      - id: class_num
        type: u4 
      - id: scale
        type: f4
      - id: unknown_30
        type: u4
      - id: unknown_34
        type: u4
      - id: unknown_38
        type: u4
      - id: unknown_3c
        type: u4
      - id: position
        type: vec3f
      - id: rotation
        type: vec3f
      - id: unknown_58
        type: u4
      - id: unknown_5c
        type: u4
      - id: unknown_60
        type: u4
      - id: unknown_64
        type: u4
      - id: unknown_68
        type: u4
      - id: unknown_6c
        type: u4
      - id: unknown_70
        type: u4
      - id: unknown_74
        type: u4
      - id: unknown_78
        type: u4
      - id: unknown_7c
        type: u4
      - id: unknown_80
        type: u4
      - id: unknown_84
        type: u4
  vec3f:
    seq:
      - id: x
        type: f4
      - id: y
        type: f4
      - id: z
        type: f4
  racmat:
    seq:
      - id: col_i
        type: f4
        repeat: expr
        repeat-expr: 4
      - id: col_j
        type: f4
        repeat: expr
        repeat-expr: 4
      - id: col_k
        type: f4
        repeat: expr
        repeat-expr: 4
      - id: col_t
        type: f4
        repeat: expr
        repeat-expr: 3
