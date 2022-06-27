meta:
  id: level
  application: Ratchet & Clank 2
  endian: le
seq:
  - id: header_type
    type: u4
  - id: unknown_4
    type: u4
  - id: unknown_8
    type: u4
  - id: unknown_c
    type: u4
  - id: primary_header_sectors
    type: u4
  - id: unknown_14
    type: u4
  - id: unknown_18
    type: u4
  - id: unknown_1c
    type: u4
  - id: world_segment_sectors
    type: u4
instances:
  primary_header:
    pos: primary_header_sectors * 0x800
    type: primary_header
types:
  primary_header:
    seq:
      - id: code_segment_offset
        type: u4
      - id: code_segment_size
        type: u4
      - id: asset_header_offset
        type: u4
      - id: asset_header_size
        type: u4
      - id: tex_pixel_data_base
        type: u4
      - id: unk_000
        type: u4
      - id: hud_header_offset
        type: u4
      - id: unknown_1c
        type: u4
      - id: hud_bank_0_offset
        type: u4
      - id: hud_bank_0_size
        type: u4
      - id: hud_bank_1_offset
        type: u4
      - id: hud_bank_1_size
        type: u4
      - id: hud_bank_2_offset
        type: u4
      - id: hud_bank_2_size
        type: u4
      - id: hud_bank_3_offset
        type: u4
      - id: hud_bank_3_size
        type: u4
      - id: hud_bank_4_offset
        type: u4
      - id: hud_bank_4_size
        type: u4
      - id: asset_wad
        type: u4
    instances:
      asset_header:
        pos: _parent.primary_header_sectors * 0x800 + asset_header_offset
        type: asset_header
      hud:
        pos: _parent.primary_header_sectors * 0x800 + hud_header_offset
        type: hud_header
        size: unk_000
  asset_header:
    seq:
      - id: mipmap_count
        type: u4
      - id: mipmap_offset
        type: u4
      - id: ptr_into_asset_wad_8
        type: u4
      - id: ptr_into_asset_wad_c
        type: u4
      - id: ptr_into_asset_wad_10
        type: u4
      - id: models_something
        type: u4
      - id: num_models
        type: u4
      - id: models
        type: u4
      - id: unknown_20
        type: u4
      - id: unknown_24
        type: u4
      - id: unknown_28
        type: u4
      - id: unknown_2c
        type: u4
      - id: terrain_texture_count
        type: u4
      - id: terrain_texture_offset
        type: u4
      - id: moby_texture_count
        type: u4
      - id: moby_texture_offset
        type: u4
      - id: tie_texture_count
        type: u4
      - id: tie_texture_offset
        type: u4
      - id: shrub_texture_count
        type: u4
      - id: shrub_texture_offset
        type: u4
      - id: some2_texture_count
        type: u4
      - id: some2_texture_offset
        type: u4
      - id: sprite_texture_count
        type: u4
      - id: sprite_texture_offset
        type: u4
      - id: tex_data_int_asset_wad
        type: u4
      - id: ptr_into_asset_wad_64
        type: u4
      - id: ptr_into_asset_wad_68
        type: u4
      - id: rel_to_asset_header_6c
        type: u4
      - id: rel_to_asset_header_70
        type: u4
      - id: unknown_74
        type: u4
      - id: rel_to_asset_header_78
        type: u4
        id: unknown_7c
        type: u4
        id: index_into_som1_texs
        type: u4
        id: unknown_84
        type: u4
        id: unknown_88
        type: u4
        id: unknown_8c
        type: u4
        id: unknown_90
        type: u4
        id: unknown_94
        type: u4
        id: unknown_98
        type: u4
        id: unknown_9c
        type: u4
        id: unknown_a0
        type: u4
        id: ptr_into_asset_wad_a4
        type: u4
        id: unknown_a8
        type: u4
        id: unknown_ac
        type: u4
        id: ptr_into_asset_wad_b0
        type: u4
    instances:
      terrain_textures:
        pos: _parent._parent.primary_header_sectors * 0x800 +
          _parent.asset_header_offset + terrain_texture_offset
        type: texture_entry
        repeat: expr
        repeat-expr: terrain_texture_count
      moby_textures:
        pos: _parent._parent.primary_header_sectors * 0x800 +
          _parent.asset_header_offset + moby_texture_offset
        type: texture_entry
        repeat: expr
        repeat-expr: moby_texture_count
      tie_textures:
        pos: _parent._parent.primary_header_sectors * 0x800 +
          _parent.asset_header_offset + tie_texture_offset
        type: texture_entry
        repeat: expr
        repeat-expr: tie_texture_count
      shrub_textures:
        pos: _parent._parent.primary_header_sectors * 0x800 +
          _parent.asset_header_offset + shrub_texture_offset
        type: texture_entry
        repeat: expr
        repeat-expr: shrub_texture_count
      some2_textures:
        pos: _parent._parent.primary_header_sectors * 0x800 +
          _parent.asset_header_offset + some2_texture_offset
        type: texture_entry
        repeat: expr
        repeat-expr: some2_texture_count
  texture_entry:
    seq:
      - id: ptr
        type: u4
      - id: width
        type: u2
      - id: height
        type: u2
      - id: unknown_8
        type: u2
      - id: palette
        type: u2
      - id: unknown_c
        type: u4
  hud_header:
    seq:
      - id: unknown_0
        type: u4
