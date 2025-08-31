# Asset Reference

This file was generated from asset_schema.wtf and is for version 29 of the asset format.

## Index

- [Index](#index)
- [General](#general)
	- [Reference](#reference)
	- [Collection](#collection)
	- [Placeholder](#placeholder)
	- [Binary](#binary)
	- [Build](#build)
	- [PrimaryVolumeDescriptor](#primaryvolumedescriptor)
	- [File](#file)
	- [Mesh](#mesh)
	- [Texture](#texture)
	- [FlatWad](#flatwad)
	- [Subtitle](#subtitle)
	- [ElfFile](#elffile)
- [Globals](#globals)
	- [GlobalWad](#globalwad)
	- [ArmorWad](#armorwad)
	- [AudioWad](#audiowad)
	- [HelpAudio](#helpaudio)
	- [BonusWad](#bonuswad)
	- [GadgetWad](#gadgetwad)
	- [HudWad](#hudwad)
	- [MiscWad](#miscwad)
	- [IrxWad](#irxwad)
	- [BootWad](#bootwad)
	- [MpegWad](#mpegwad)
	- [Mpeg](#mpeg)
	- [OnlineWad](#onlinewad)
	- [OnlineDataWad](#onlinedatawad)
	- [SceneWad](#scenewad)
	- [SpaceWad](#spacewad)
- [Level](#level)
	- [Level](#level)
	- [LevelWad](#levelwad)
	- [Chunk](#chunk)
	- [Mission](#mission)
	- [LevelAudioWad](#levelaudiowad)
	- [LevelSceneWad](#levelscenewad)
	- [Scene](#scene)
- [Classes](#classes)
	- [MobyClass](#mobyclass)
	- [MobyClassCore](#mobyclasscore)
	- [TieClass](#tieclass)
	- [TieClassCore](#tieclasscore)
	- [ShrubClass](#shrubclass)
	- [ShrubClassCore](#shrubclasscore)
	- [ShrubBillboard](#shrubbillboard)
- [Miscellaneous](#miscellaneous)
	- [Tfrags](#tfrags)
	- [TfragsCore](#tfragscore)
	- [Material](#material)
	- [Collision](#collision)
	- [CollisionMaterial](#collisionmaterial)
	- [Sky](#sky)
	- [SkyShell](#skyshell)
	- [Occlusion](#occlusion)
	- [Instances](#instances)

## General

### Reference

A reference to an asset that can be put in place of any other asset.

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| asset | The asset being referenced. | AssetLink | Yes | RAC/GC/UYA/DL |

*Children*

No children.

### Collection

A generic container for other assets.

*Attributes*

No attributes.

*Children*

No children.

*Hints*

| Syntax | Example | Description |
| - | - | - |
| `texlist,<texture hint>` | `texlist,pif,8,unswizzled` | A list of textures, where \<texture hint\> is the hint used by each Texture asset in the list. |
| `matlist,<texture hint>` | `matlist,pif,8,unswizzled` | A list of materials, where \<texture hint\> is the hint used by each Texture asset. |
| `subtitles` | `subtitles` | A collection of subtitles. |
| `missionclasses` | `missionclasses` | A set of moby classes to be packed in the files for a mission in Deadlocked. |

### Placeholder

The asset type generated when a dot is used in a tag.

*Attributes*

No attributes.

*Children*

No children.

### Binary

A raw binary file. Used for assets that wrench cannot unpack.

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| src | The file path of the binary file, relative to the .asset file. | FilePath | No | RAC/GC/UYA/DL |

*Children*

No children.

*Example*

```
Binary code {
	src: "code.bin"
}
```

*Hints*

| Syntax | Example | Description |
| - | - | - |
| `ext,<extension>` | `ext,wad` | A binary file with a custom \<extension\> (not .bin). |

### Build

A build of the game.

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| game | The game being built. Possible values: "rac", "gc", "uya", "dl". | String | Yes | RAC/GC/UYA/DL |
| region | The region. Possible values: "us", "eu", "japan". | String | Yes | RAC/GC/UYA/DL |
| version | The version number, as specified in the SYSTEM.CNF file. | String | Yes | RAC/GC/UYA/DL |
| ps2_logo_key | The key used to "encrypt" the PS2 logo. | Integer | No | RAC/GC/UYA/DL |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| ps2_logo_ntsc | The NTSC PS2 logo stored on the first 12 sectors of the disc. | Texture | No | RAC/GC/UYA/DL |
| ps2_logo_pal | The PAL PS2 logo stored on the first 12 sectors of the disc. | Texture | No | RAC/GC/UYA/DL |
| primary_volume_descriptor | *Not yet documented.* | PrimaryVolumeDescriptor | Yes | RAC/GC/UYA/DL |
| boot_elf | The boot ELF, as specified in the SYSTEM.CNF file. | ElfFile, File | Yes | RAC/GC/UYA/DL |
| global | *Not yet documented.* | GlobalWad, FlatWad, Binary | Yes | RAC |
| armor | *Not yet documented.* | ArmorWad, FlatWad, Binary | Yes | GC/UYA/DL |
| audio | *Not yet documented.* | AudioWad, FlatWad, Binary | Yes | GC/UYA/DL |
| bonus | *Not yet documented.* | BonusWad, FlatWad, Binary | Yes | GC/UYA/DL |
| gadget | *Not yet documented.* | GadgetWad, FlatWad, Binary | Yes | GC/UYA |
| hud | *Not yet documented.* | HudWad, FlatWad, Binary | Yes | GC/UYA/DL |
| misc | *Not yet documented.* | MiscWad, FlatWad, Binary | Yes | GC/UYA/DL |
| mpeg | *Not yet documented.* | MpegWad, FlatWad, Binary | Yes | GC/UYA/DL |
| online | *Not yet documented.* | OnlineWad, FlatWad, Binary | Yes | DL |
| scene | *Not yet documented.* | SceneWad, FlatWad, Binary | Yes | GC |
| space | *Not yet documented.* | SpaceWad, FlatWad, Binary | Yes | GC/UYA/DL |
| levels | The list of levels to include in the build. | Collection | Yes | RAC/GC/UYA/DL |
| files | Additional files to include in the build. | Collection | Yes | RAC/GC/UYA/DL |
| moby_classes | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| tie_classes | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| shrub_classes | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| particle_textures | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |


*Hints*

| Syntax | Example | Description |
| - | - | - |
| `release` | `release` | A full build of the game. |
| `testlf,<level>,<flags>` | `testlf,4,nompegs` | Only the level with asset tag \<level\> is included (or leave it empty for normal level packing), and various \<flags\> are set. Flags are seperated by a pipe (\|) character. Available flags: "nompegs" |

### PrimaryVolumeDescriptor

A filesystem structure that contains metadata.

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| system_identifier | *Not yet documented.* | String | *Not yet documented.* | *Not yet documented.*  |
| volume_identifier | *Not yet documented.* | String | *Not yet documented.* | *Not yet documented.*  |
| volume_set_identifier | *Not yet documented.* | String | *Not yet documented.* | *Not yet documented.*  |
| publisher_identifier | *Not yet documented.* | String | *Not yet documented.* | *Not yet documented.*  |
| data_preparer_identifier | *Not yet documented.* | String | *Not yet documented.* | *Not yet documented.*  |
| application_identifier | *Not yet documented.* | String | *Not yet documented.* | *Not yet documented.*  |
| copyright_file_identifier | *Not yet documented.* | String | *Not yet documented.* | *Not yet documented.*  |
| abstract_file_identifier | *Not yet documented.* | String | *Not yet documented.* | *Not yet documented.*  |
| bibliographic_file_identifier | *Not yet documented.* | String | *Not yet documented.* | *Not yet documented.*  |

*Children*

No children.

### File

An additional file to be included in the built ISO.

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| src | The path of the file, relative to the .asset file. | FilePath | *Not yet documented.* | *Not yet documented.*  |
| path | Where to put the file on the filesystem of the built ISO. | String | *Not yet documented.* | *Not yet documented.*  |

*Children*

No children.

*Example*

```
File cnf_icon_en_sys {
	src: "cnf/icon_en.sys"
	path: "cnf/icon_en.sys"
}
```

### Mesh

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| name | The name of the node to use. This is the same as the Blender node name. | String | Yes | RAC/GC/UYA/DL |
| src | The file path of the model file to use. | FilePath | Yes | RAC/GC/UYA/DL |

*Children*

No children.

### Texture

A reference to an image file. Only PNG images are currently supported.

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| src | *Not yet documented.* | FilePath | *Not yet documented.* | *Not yet documented.*  |

*Children*

No children.

*Example*

```
Texture my_texture {
	src: "my_texture.png"
}
```

*Hints*

| Syntax | Example | Description |
| - | - | - |
| `pif,<palette size>,<mip levels>,<swizzled>` | `pif,8,1,unswizzled` | A paletted PIF image with a \<palette size\> of "4" or "8" bits with 1 or more mip levels (where 1 is just a single image), where the pixel data is either "swizzled" or "unswizzled". |
| `rgba` | `rgba` | An RGBA image with a header that encodes the width and height. |
| `rawrgba,<width>,<height>` | `rawrgba,512,416` | A headerless RGBA image of size \<width\> by \<height\>. |

### FlatWad

A WAD file where each child represents a different data segment referenced by the header. The tag of each child is converted to a number which is used to determine at what position to write the sector offset and size into the header. This type is mainly used for debugging.

*Attributes*

No attributes.

*Children*

No children.

*Example*

```
FlatWad online {
	Binary 0x8 {
		src: "data.bin"
	}
	
	Binary 0x10 {
		src: "transition_backgrounds/0.bin"
	}
	
	Binary 0x18 {
		src: "transition_backgrounds/1.bin"
	}
	
	Binary 0x20 {
		src: "transition_backgrounds/2.bin"
	}
	
	Binary 0x28 {
		src: "transition_backgrounds/3.bin"
	}
	
	Binary 0x30 {
		src: "transition_backgrounds/4.bin"
	}
	
	Binary 0x38 {
		src: "transition_backgrounds/5.bin"
	}
	
	Binary 0x40 {
		src: "transition_backgrounds/6.bin"
	}
	
	Binary 0x48 {
		src: "transition_backgrounds/7.bin"
	}
	
	Binary 0x50 {
		src: "transition_backgrounds/8.bin"
	}
	
	Binary 0x58 {
		src: "transition_backgrounds/9.bin"
	}
	
	Binary 0x60 {
		src: "transition_backgrounds/10.bin"
	}
}
```

### Subtitle

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| start_time | The time, in seconds, since the beginning of the cutscene, when the subtitle should appear. | Float | Yes | RAC/GC/UYA/DL |
| stop_time | The time, in seconds, since the beginning of the cutscene, when the subtitle should disappear. | Float | Yes | RAC/GC/UYA/DL |
| text_e | The English subtitle text. | String | No | RAC/GC/UYA/DL |
| text_f | The French subtitle text. | String | No | RAC/GC/UYA/DL |
| text_g | The German subtitle text. | String | No | RAC/GC/UYA/DL |
| text_s | The Spanish subtitle text. | String | No | RAC/GC/UYA/DL |
| text_i | The Italian subtitle text. | String | No | RAC/GC/UYA/DL |
| text_j | The Japanese subtitle text. | String | No | UYA/DL |
| text_k | The Korean subtitle text. | String | No | UYA/DL |
| encoding_e | "raw" means the text is in the format accepted by the game, "utf8" means the text is in UTF-8 (which is not yet supported). | String | No | RAC/GC/UYA/DL |
| encoding_f | See encoding_e. | String | No | RAC/GC/UYA/DL |
| encoding_g | See encoding_e. | String | No | RAC/GC/UYA/DL |
| encoding_s | See encoding_e. | String | No | RAC/GC/UYA/DL |
| encoding_i | See encoding_e. | String | No | RAC/GC/UYA/DL |
| encoding_j | See encoding_e. | String | No | UYA/DL |
| encoding_k | See encoding_e. | String | No | UYA/DL |

*Children*

No children.

### ElfFile

An ELF executable file. Currently used for the frontend and level code.

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| name | The name of ELF file on the filesystem of the built ISO (if relevant). | String | No | RAC/GC/UYA/DL |
| src | The path of the ELF file, relative to the .asset file. | FilePath | Yes | RAC/GC/UYA/DL |

*Children*

No children.

*Hints*

| Syntax | Example | Description |
| - | - | - |
| `ratchetexecutable` | `ratchetexecutable` | Convert the ELF file to and from a packed Ratchet executable while packing/unpacking. |

## Globals

### GlobalWad

Holds all of the global assets (those that are not specific to a given level) for R&C1. Not used for GC, UYA or DL.

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| debug_font | *Not yet documented.* | Texture, Binary | *Not yet documented.* | *Not yet documented.* |
| save_game | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| ratchet_seqs | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| hud_seqs | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| vendor | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| vendor_audio | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| help_controls | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| help_moves | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| help_weapons | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| help_gadgets | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| help_ss | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| options_ss | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| frontbin | *Not yet documented.* | ElfFile, Binary | *Not yet documented.* | *Not yet documented.* |
| mission_ss | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| planets | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| stuff2 | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| goodies_images | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| character_sketches | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| character_renders | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| skill_images | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| epilogue_english | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| epilogue_french | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| epilogue_italian | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| epilogue_german | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| epilogue_spanish | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| sketchbook | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| commercials | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| item_images | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| irx | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| sound_bank | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| wad_14e0 | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| music | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| hud_header | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| hud_banks | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| all_text | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| post_credits_helpdesk_girl_seq | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| post_credits_audio | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| credits_images_ntsc | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| credits_images_pal | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| wad_things | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| mpegs | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| help_audio | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| qwark_boss_audio | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| spaceships | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| anim_looking_thing_2 | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| space_plates | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| transition | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| space_audio | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| things | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |


*Hints*

| Syntax | Example | Description |
| - | - | - |
| `nompegs` | `nompegs` | This option skips writing out the mpegs to make packing faster (or provide no hint to write them out as normal). |

### ArmorWad

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| armors | The armors/skins. | MobyClass\[\] | Yes | GC/UYA/DL |
| wrenches | The different wrench upgrades. | MobyClass\[\] | Yes | GC/UYA |
| multiplayer_armors | The multiplayer skins. | MobyClass\[\] | Yes | UYA |
| clank_textures | Textures for Clank. | Texture\[\], Binary\[\] | Yes | UYA |
| bot_textures | Textures for Merc and Green. | Texture\[\] | Yes | DL |
| landstalker_textures | Textures for the landstalker. | Collection | Yes | DL |
| dropship_textures | Textures for the dropship. | Collection | Yes | DL |


### AudioWad

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| vendor | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| global_sfx | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| help | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |


### HelpAudio

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| english | *Not yet documented.* | Sound, Binary | *Not yet documented.* | *Not yet documented.* |
| french | *Not yet documented.* | Sound, Binary | *Not yet documented.* | *Not yet documented.* |
| german | *Not yet documented.* | Sound, Binary | *Not yet documented.* | *Not yet documented.* |
| spanish | *Not yet documented.* | Sound, Binary | *Not yet documented.* | *Not yet documented.* |
| italian | *Not yet documented.* | Sound, Binary | *Not yet documented.* | *Not yet documented.* |


### BonusWad

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| goodies_images | Images that appear in the R&C1 goodies menu, also included in R&C2 for some reason. | Texture\[\] | Yes | GC |
| character_sketches | Part of a cut R&C1 credits screen. | Texture\[\] | Yes | GC |
| character_renders | Part of a cut R&C1 credits screen, also included in R&C2 for some reason. | Texture\[\] | Yes | GC |
| old_skill_images | The skill point images from R&C1, also included in R&C2 for some reason. | Texture\[\] | Yes | GC |
| epilogue_english | The english epilogue images from R&C1, also included in R&C2 for some reason. | Texture\[\] | Yes | GC |
| epilogue_french | The french epilogue images from R&C1, also included in R&C2 for some reason. | Texture\[\] | Yes | GC |
| epilogue_italian | The italian epilogue images from R&C1, also included in R&C2 for some reason. | Texture\[\] | Yes | GC |
| epilogue_german | The german epilogue images from R&C1. | Texture\[\] | Yes | GC |
| epilogue_spanish | The spanish epilogue images from R&C1. | Texture\[\] | Yes | GC |
| sketchbook | *Not yet documented.* | Texture\[\] | Yes | GC |
| commercials | *Not yet documented.* | Texture\[\] | Yes | GC |
| item_images | *Not yet documented.* | Texture\[\] | Yes | GC |
| random_stuff | *Not yet documented.* | Texture\[\] | Yes | GC |
| movie_images | *Not yet documented.* | Texture\[\] | Yes | GC |
| cinematic_images | *Not yet documented.* | Texture\[\] | Yes | GC |
| clanks_day_at_insomniac | *Not yet documented.* | Texture\[\] | Yes | GC |
| endorsement_deals | *Not yet documented.* | Texture\[\] | Yes | GC |
| short_cuts | *Not yet documented.* | Texture\[\] | Yes | GC |
| paintings | *Not yet documented.* | Texture\[\] | Yes | GC |
| credits_text | The text used in the credits sequence. | Binary\[\] | Yes | GC/UYA/DL |
| credits_images | The background images used in the credits sequence, each 512 by 416. | Texture\[\] | Yes | GC/UYA/DL |
| credits_images_pal | The background images used in the PAL R&C1 credits sequence, each 512 by 448. | Texture\[\] | Yes | RAC |
| demo_menu | *Not yet documented.* | Texture\[\]\[\] | Yes | UYA/DL |
| demo_exit | *Not yet documented.* | Texture\[\]\[\] | Yes | UYA/DL |
| cheat_images | The images used on the cheats screen. | Texture\[\] | Yes | UYA/DL |
| skill_images | The images used on the skill points screen. | Texture\[\] | Yes | GC/UYA/DL |
| trophy_image | *Not yet documented.* | Texture, Binary | Yes | UYA/DL |
| dige | *Not yet documented.* | Binary | Yes | DL |


### GadgetWad

*Attributes*

No attributes.

*Children*

No children.

### HudWad

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| online_images | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| ratchet_seqs | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| hud_seqs | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| vendor | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| all_text | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| hudw3d | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| e3_level_ss | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| nw_dnas_image | *Not yet documented.* | Texture, Binary | *Not yet documented.* | *Not yet documented.* |
| split_screen_texture | *Not yet documented.* | Texture, Binary | *Not yet documented.* | *Not yet documented.* |
| radar_maps | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| weapon_plates_large | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| mission_plates_large | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| gui_plates | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| vendor_plates | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| loading_screen | *Not yet documented.* | Texture, Binary | *Not yet documented.* | *Not yet documented.* |
| planets | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| cinematics | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| equip_large | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| equip_small | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| moves | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| save_level | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| save_empty | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| skills | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| reward_back | *Not yet documented.* | Texture, Binary | *Not yet documented.* | *Not yet documented.* |
| complete_back | *Not yet documented.* | Texture, Binary | *Not yet documented.* | *Not yet documented.* |
| complete_back_coop | *Not yet documented.* | Texture, Binary | *Not yet documented.* | *Not yet documented.* |
| rewards | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| leaderboard | *Not yet documented.* | Texture, Binary | *Not yet documented.* | *Not yet documented.* |
| cutaways | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| sketchbook | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| character_epilogues | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| character_cards | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| equip_plate | *Not yet documented.* | Texture, Binary | *Not yet documented.* | *Not yet documented.* |
| hud_flythru | *Not yet documented.* | Texture, Binary | *Not yet documented.* | *Not yet documented.* |
| mp_maps | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| tourney_plates_large | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |


### MiscWad

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| debug_font | *Not yet documented.* | Texture, Binary | Yes | GC/UYA/DL |
| irx | *Not yet documented.* | IrxWad | Yes | GC/UYA/DL |
| save_game | *Not yet documented.* | Binary | Yes | GC/UYA/DL |
| frontbin | *Not yet documented.* | ElfFile, Binary | *Not yet documented.* | *Not yet documented.* |
| frontbin_net | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| frontend | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| exit | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| boot | *Not yet documented.* | BootWad | *Not yet documented.* | *Not yet documented.* |
| gadget | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |


### IrxWad

A container for IOP modules.

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| image | *Not yet documented.* | Texture, Binary | Yes | GC |
| sio2man | *Not yet documented.* | Binary | Yes | GC/UYA/DL |
| mcman | Memory card manager. | Binary | Yes | GC/UYA/DL |
| mcserv | Memory card server. | Binary | Yes | GC/UYA/DL |
| dbcman | *Not yet documented.* | Binary | Yes | GC |
| sio2d | *Not yet documented.* | Binary | Yes | GC |
| ds2u | *Not yet documented.* | Binary | Yes | GC |
| padman | Gamepad manager. | Binary | Yes | UYA/DL |
| mtapman | Multitap manager. | Binary | Yes | UYA/DL |
| libsd | Sound library. | Binary | Yes | GC/UYA/DL |
| 989snd | Sound library. | Binary | Yes | GC/UYA/DL |
| stash | *Not yet documented.* | Binary | Yes | UYA/DL |
| inet | Networking library. | Binary | Yes | UYA/DL |
| netcnf | Networking library. | Binary | Yes | UYA/DL |
| inetctl | *Not yet documented.* | Binary | Yes | UYA/DL |
| msifrpc | RPC library. | Binary | Yes | UYA/DL |
| dev9 | HDD/ethernet/modem adapter library. | Binary | Yes | UYA/DL |
| smap | Ethernet driver. | Binary | Yes | UYA/DL |
| libnetb | Networking library. | Binary | Yes | UYA/DL |
| ppp | Point to Point Protcol (PPP) library. | Binary | Yes | UYA/DL |
| pppoe | Point to Point Protcol over Ethernet (PPPoE) library. | Binary | Yes | UYA/DL |
| usbd | USB library. | Binary | Yes | UYA/DL |
| lgaud | *Not yet documented.* | Binary | Yes | UYA/DL |
| eznetcnf | *Not yet documented.* | Binary | Yes | UYA/DL |
| eznetctl | *Not yet documented.* | Binary | Yes | UYA/DL |
| lgkbm | *Not yet documented.* | Binary | Yes | UYA/DL |
| streamer | *Not yet documented.* | Binary | Yes | DL |
| astrm | *Not yet documented.* | Binary | Yes | DL |


### BootWad

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| english | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| french | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| german | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| spanish | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| italian | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| hud_header | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| hud_banks | *Not yet documented.* | Binary\[\] | *Not yet documented.* | *Not yet documented.* |
| boot_plates | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| sram | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |


### MpegWad

The file containing all of the MPEG cutscenes.

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| mpegs | A list of all the MPEG cutscenes. | Mpeg\[\] | *Not yet documented.* | *Not yet documented.* |


*Hints*

| Syntax | Example | Description |
| - | - | - |
| `nompegs` | `nompegs` | This option skips writing out the mpegs to make packing faster (or provide no hint to write them out as normal). |

### Mpeg

An MPEG cutscene with subtitles.

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| video_ntsc | The NTSC version of the video file. 512x416, 29.97 FPS. | Binary | No | RAC/GC/UYA/DL |
| video_pal | The PAL version of the video file. 512x416, 25 FPS. | Binary | No | RAC/GC/UYA/DL |
| subtitles | The subtitles. | Subtitle\[\], Binary | No | GC/UYA/DL |


### OnlineWad

Container for assets used in the mutliplayer mode.

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| data | *Not yet documented.* | OnlineDataWad, Binary | Yes | DL |
| transition_backgrounds | Background images used for the multiplayer loading screen. | Texture\[\], Binary\[\] | Yes | DL |


### OnlineDataWad

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| onlinew3d | *Not yet documented.* | Binary | Yes | DL |
| images | *Not yet documented.* | Texture\[\], Binary\[\] | Yes | DL |
| moby_classes | *Not yet documented.* | Binary\[\] | Yes | DL |


### SceneWad

*Attributes*

No attributes.

*Children*

No children.

### SpaceWad

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| transitions | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |


## Level

### Level

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| name | A human-friendly string that identifies the level e.g. "Pokitaru". | String | No | RAC/GC/UYA/DL |
| category | A human-friendly string that identifies the type of the level e.g. "Multiplayer". | String | No | RAC/GC/UYA/DL |
| index | The index of the level in the level table. Note that this is different to the id attribute of the associated LevelWad asset. | Integer | Yes | RAC/GC/UYA/DL |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| level | *Not yet documented.* | LevelWad, Binary | *Not yet documented.* | *Not yet documented.* |
| audio | *Not yet documented.* | LevelAudioWad, Binary | *Not yet documented.* | *Not yet documented.* |
| scene | *Not yet documented.* | LevelSceneWad, Binary | *Not yet documented.* | *Not yet documented.* |


### LevelWad

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| id | The ID number of the level, referenced frequently at runtime. Note that this is different to the index attribute of the associated Level asset. | Integer | Yes | RAC/GC/UYA/DL |
| reverb | *Not yet documented.* | Integer | Yes | GC/UYA/DL |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| sound_bank | The main 989snd sound bank for the level. | Binary | Yes | RAC/GC/UYA/DL |
| gameplay | The gameplay file for the level. | Instances, Binary | Yes | RAC/GC/UYA/DL |
| art_instances | Similar thing as with the gameplay file. | Instances, Binary | Yes | DL |
| help_messages_us_english | *Not yet documented.* | Binary | No | RAC/GC/UYA/DL |
| help_messages_uk_english | *Not yet documented.* | Binary | No | RAC/GC/UYA/DL |
| help_messages_french | *Not yet documented.* | Binary | No | RAC/GC/UYA/DL |
| help_messages_german | *Not yet documented.* | Binary | No | RAC/GC/UYA/DL |
| help_messages_spanish | *Not yet documented.* | Binary | No | RAC/GC/UYA/DL |
| help_messages_italian | *Not yet documented.* | Binary | No | RAC/GC/UYA/DL |
| help_messages_japanese | *Not yet documented.* | Binary | No | RAC/GC/UYA/DL |
| help_messages_korean | *Not yet documented.* | Binary | No | RAC/GC/UYA/DL |
| chunks | The chunks (see Chunk asset). | Collection | Yes | RAC/GC/UYA/DL |
| missions | *Not yet documented.* | Collection | *Not yet documented.* | DL |
| moby8355_pvars | *Not yet documented.* | Binary | Yes | DL |
| overlay | The level code overlay. Contains most of the code for the game and engine. | ElfFile, Binary | Yes | RAC/GC/UYA/DL |
| hud_header | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| hud_banks | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| transition_textures | Textures that are shown during a transition to the given level. | Texture\[\], Binary | No | GC/UYA |
| global_nav_data | *Not yet documented.* | Binary | Yes | DL |
| occlusion | *Not yet documented.* | Occlusion | *Not yet documented.* | *Not yet documented.* |
| sky | The sky. | Sky, Binary | Yes | RAC/GC/UYA/DL |
| moby_classes | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| tie_classes | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| shrub_classes | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| particle_textures | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| fx_textures | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |
| sound_remap | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| moby_sound_remap | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| ratchet_seqs | *Not yet documented.* | Collection | *Not yet documented.* | RAC/GC/UYA |
| light_cuboids | *Not yet documented.* | Collection | *Not yet documented.* | DL |
| gadgets | *Not yet documented.* | MobyClass\[\] | *Not yet documented.* | *Not yet documented.* |


### Chunk

A chunk consists of tfrags, collision and a 989snd sound bank. These assets can be swapped out for those of another chunk without doing a full level load. R&C1 doesn't support chunks, so for Wrench's source format we only allow a single chunk (chunk 0) without a sound bank. The other games support 3 chunks per level (0, 1 and 2). The gamplay file contains planes that specify the boundaries of each chunk.

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| tfrags | See Tfrags asset. | Tfrags | *Not yet documented.* | RAC/GC/UYA/DL |
| collision | See Collision asset. | Collision, Binary | *Not yet documented.* | RAC/GC/UYA/DL |
| sound_bank | The 989snd sound bank for this chunk. | Binary | *Not yet documented.* | GC/UYA/DL |


### Mission

A Deadlocked mission.

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| instances | Moby gameplay instances and pvar data for this mission. | Instances, Binary | *Not yet documented.* | DL |
| classes | Moby classes for this mission. | Collection, Binary | *Not yet documented.* | DL |
| sound_bank | Sound bank for this mission. | Binary | *Not yet documented.* | DL |


### LevelAudioWad

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| bin_data | *Not yet documented.* | Collection | *Not yet documented.* | RAC/GC/UYA/DL |
| upgrade_sample | *Not yet documented.* | Binary | *Not yet documented.* | GC/UYA/DL |
| thermanator_freeze | *Not yet documented.* | Binary | *Not yet documented.* | GC |
| thermanator_thaw | *Not yet documented.* | Binary | *Not yet documented.* | GC |
| platinum_bolt | *Not yet documented.* | Binary | *Not yet documented.* | UYA/DL |
| spare | *Not yet documented.* | Binary | *Not yet documented.* | UYA/DL |


### LevelSceneWad

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| scenes | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |


### Scene

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| speech_english_left | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| speech_english_right | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| subtitles | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| speech_french_left | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| speech_french_right | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| speech_german_left | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| speech_german_right | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| speech_spanish_left | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| speech_spanish_right | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| speech_italian_left | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| speech_italian_right | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| moby_load | *Not yet documented.* | Binary | *Not yet documented.* | *Not yet documented.* |
| chunks | *Not yet documented.* | Collection | *Not yet documented.* | *Not yet documented.* |


## Classes

### MobyClass

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| name | A human-friendly string that identifies the class e.g. "Gold Bolt". | String | No | RAC/GC/UYA/DL |
| category | A human-friendly string that identifies the type of the class e.g. "Gameplay". | String | No | RAC/GC/UYA/DL |
| id | The integer used to reference this moby class at runtime. | Integer | Yes | RAC/GC/UYA/DL |
| has_moby_table_entry | *Not yet documented.* | Boolean | *Not yet documented.* | *Not yet documented.*  |
| stash_textures | Whether or not the textures for this class should be permanently resident in GS memory. This is typically used for items attached to Ratchet such as the heli-pack and helmet. | Boolean | No | GC/UYA/DL |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| core | *Not yet documented.* | MobyClassCore, Binary | *Not yet documented.* | *Not yet documented.* |
| materials | The materials used by this class. | Material\[\] | Yes | RAC/GC/UYA/DL |
| static_collision | The collision to be be built into the main static level collision. | Collision | No | RAC/GC/UYA/DL |
| editor_mesh | The mesh shown in the editor. This is currently used as a hack since we haven't got the moby exporter working, but in the future it could be used for invisible mobies. | Mesh | No | RAC/GC/UYA/DL |
| editor_icon | The icon shown in the 3D view in the editor if a mesh is not available. | Texture | No | RAC/GC/UYA/DL |


*Hints*

| Syntax | Example | Description |
| - | - | - |
| `phat` | `phat` | A full moby class. This is the default if no hint is specified. |
| `meshonly,<scale>,<animated>` | `meshonly,0.5,true` | A stripped down moby class that only includes the high LOD, low LOD and multipass metal meshes. Only use this for singleplayer armors for GC and UYA. The \<scale\> and \<animated\> parameters are only used by the unpacker, so can be omitted or set to junk values for packing. |

### MobyClassCore

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| scale | The scale multiplier used to account for the quantization of the components of the vertex positions are signed 16 bit integers. This cannot be automatically computed since animation is done in this space which can be affected by manipulators/tweakers at runtime. | Float | *Not yet documented.* | *Not yet documented.*  |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| mesh | *Not yet documented.* | Mesh | *Not yet documented.* | *Not yet documented.* |
| low_lod_mesh | *Not yet documented.* | Mesh | *Not yet documented.* | *Not yet documented.* |


### TieClass

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| name | A human-friendly string that identifies the class e.g. "Snowy Mountain". | String | No | RAC/GC/UYA/DL |
| category | A human-friendly string that identifies the type of the class e.g. "Mountains". | String | No | RAC/GC/UYA/DL |
| id | The integer used to reference this tie class at runtime. | Integer | Yes | RAC/GC/UYA/DL |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| core | *Not yet documented.* | TieClassCore, Binary | *Not yet documented.* | *Not yet documented.* |
| materials | The materials used by this class. | Material\[\] | Yes | RAC/GC/UYA/DL |
| static_collision | The collision to be be built into the main static level collision. | Collision | No | RAC/GC/UYA/DL |
| editor_mesh | The mesh shown in the editor. We're using this as a hack since we haven't got the tie exporter working. | Mesh | No | RAC/GC/UYA/DL |


### TieClassCore

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| mesh | The mesh. | Mesh | *Not yet documented.* | *Not yet documented.* |


### ShrubClass

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| name | A human-friendly string that identifies the class e.g. "Yellow Flower". | String | No | RAC/GC/UYA/DL |
| category | A human-friendly string that identifies the type of the class e.g. "Flowers". | String | No | RAC/GC/UYA/DL |
| id | The integer used to reference this shrub class at runtime. | Integer | Yes | RAC/GC/UYA/DL |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| core | *Not yet documented.* | ShrubClassCore, Binary | *Not yet documented.* | *Not yet documented.* |
| billboard | Cheap billboard, to be drawn when the camera is far away. | ShrubBillboard | No | RAC/GC/UYA/DL |
| materials | The materials used by this class. | Material\[\] | Yes | RAC/GC/UYA/DL |
| static_collision | The collision to be be built into the main static level collision. | Collision | No | RAC/GC/UYA/DL |


### ShrubClassCore

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| mip_distance | *Not yet documented.* | Float | Yes | RAC/GC/UYA/DL |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| mesh | The mesh. | Mesh | *Not yet documented.* | *Not yet documented.* |


### ShrubBillboard

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| fade_distance | The distance from the camera in which the shrub turns transparent and disappears. | Float | Yes | RAC/GC/UYA/DL |
| width | The width of the billboard. | Float | Yes | RAC/GC/UYA/DL |
| height | The height of the billboard. | Float | Yes | RAC/GC/UYA/DL |
| z_offset | ... | Float | Yes | RAC/GC/UYA/DL |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| texture | The texture to show on the billboard. | Texture | Yes | RAC/GC/UYA/DL |


## Miscellaneous

### Tfrags

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| core | *Not yet documented.* | TfragsCore, Binary | *Not yet documented.* | *Not yet documented.* |
| editor_mesh | The mesh shown in the editor. We're using this as a hack since we haven't got the tfrag exporter working. | Mesh | No | RAC/GC/UYA/DL |
| materials | The materials used by the tfrags. | Material\[\] | No | RAC/GC/UYA/DL |


### TfragsCore

*Attributes*

No attributes.

*Children*

No children.

### Material

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| name | The name of the material being referenced. | String | Yes | RAC/GC/UYA/DL |
| wrap_mode | The UV wrapping mode, stored as an array of two strings. Possible values are ["repeat" "repeat"] (the default), ["repeat" "clamp"], ["clamp" "repeat"] and ["clamp" "clamp"]. | Array | No | RAC/GC/UYA/DL |
| glass | Only for mobies. Make the material shiny, like glass? | Boolean | *Not yet documented.* | *Not yet documented.*  |
| chrome | Only for mobies. Make the material shiny, like chrome? | Boolean | *Not yet documented.* | *Not yet documented.*  |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| diffuse | The diffuse map (main texture). | Texture | Yes | RAC/GC/UYA/DL |


### Collision

The world space collision mesh.

*Attributes*

No attributes.

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| mesh | The main collision mesh. | Mesh | Yes | RAC/GC/UYA/DL |
| hero_groups | Collision meshes that will only effect Ratchet. Keep all vertices in a given group close together to optimise runtime performance. | Mesh\[\] | No | RAC/GC/UYA/DL |
| materials | A mapping of the materials used by the mesh to collision ID numbers. | *Not yet documented.* | Yes | RAC/GC/UYA/DL |


### CollisionMaterial

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| name | The name of the material being referenced. | String | Yes | RAC/GC/UYA/DL |
| id | The collision ID to use for this material. This controls the type of the surface. | Integer | Yes | RAC/GC/UYA/DL |

*Children*

No children.

### Sky

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| colour | *Not yet documented.* | Colour | No | RAC/GC/UYA |
| clear_screen | *Not yet documented.* | Boolean | No | RAC/GC/UYA/DL |
| maximum_sprite_count | Controls how much memory to allocate for sprites. | Integer | No | RAC/GC/UYA/DL |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| shells | The different meshes/layers that make up the sky. | SkyShell\[\] | Yes | RAC/GC/UYA/DL |
| materials | The materials used by the sky shell meshes. | Material\[\], Texture\[\] | Yes | RAC/GC/UYA/DL |
| fx | The FX textures. | Texture\[\] | Yes | RAC/GC/UYA/DL |


### SkyShell

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| bloom | Only works for textured shells. | Boolean | No | UYA/DL |
| starting_rotation | The starting rotation. Each component is in radians. | Vector3 | No | UYA/DL |
| angular_velocity | The change in rotation. Each component is in radians per second. | Vector3 | No | UYA/DL |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| mesh | The mesh. | Mesh | Yes | RAC/GC/UYA/DL |


### Occlusion

Occlusion information needed by the game at runtime to determine which objects are visible.		The playable space of a level is divided into 4x4x4 cubes called octants that dictate which objects are visible at a given time.

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| memory_budget | The maximum size of the computed collision database in bytes, including both the octant lookup tree and the visibility masks. | Integer | No | RAC/GC/UYA/DL |
| octants | A path to a CSV file containing a list of all the octants for which occlusion should be generated. Each line is a list of 3 integers X, Y and Z separated by commas. Each coordinate must be multiplied by 4 to obtain world coordinates. | FilePath | Yes | RAC/GC/UYA/DL |
| grid | The raw occlusion grid data used by the game. Either unpacked by the build tool or generated by the visibility tool. | FilePath | Yes | RAC/GC/UYA/DL |
| mappings | Mappings between the instances in the gameplay file and the occlusion grid. Either unpacked by the build tool or generated by the visibility tool. | FilePath | Yes | RAC/GC/UYA/DL |

*Children*

No children.

### Instances

The instances.

*Attributes*

| Name | Description | Type | Required | Games |
| - | - | - | - | - |
| src | A path to the instances file. | FilePath | Yes | RAC/GC/UYA/DL |

*Children*

| Name | Description | Allowed Types | Required | Games |
| - | - | - | - | - |
| core | For Deadlocked mission instance assets, this should be a reference to the gameplay core file. Otherwise, this should be omitted. | Instances | No | DL |


*Hints*

| Syntax | Example | Description |
| - | - | - |
| `gameplay` | `gameplay` | A gameplay file. |
| `art` | `art` | A Deadlocked art instances file. |
| `mission,<core moby count>` | `mission,123` | A Deadlocked mission instances file. For unpacking operations the core moby count needs to be equal to the number of moby instances in the gameplay core file. For packing operations this value can be omitted. |
