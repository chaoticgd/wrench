# Instance Reference

This file was generated from instance_schema.wtf and is for version 21 of the instance format.

## Instances

### Moby

Moving, interactive, or otherwise dynamic objects with an associated update function.
*Fields*

| Name | Description | Type |
| - | - | - |
| mission |  | s8 |
| uid |  | s32 |
| bolts |  | s32 |
| o_class |  | s32 |
| update_distance |  | s32 |
| group |  | s32 |
| is_rooted |  | bool |
| rooted_distance |  | f32 |
| occlusion |  | s32 |
| mode_bits |  | s32 |
| light |  | s32 |
| rac1_unknown_4 |  | s32 |
| rac1_unknown_8 |  | s32 |
| rac1_unknown_c |  | s32 |
| rac1_unknown_10 |  | s32 |
| rac1_unknown_14 |  | s32 |
| rac1_unknown_18 |  | s32 |
| rac1_unknown_1c |  | s32 |
| rac1_unknown_20 |  | s32 |
| rac1_unknown_24 |  | s32 |
| rac1_unknown_54 |  | s32 |
| rac1_unknown_60 |  | s32 |
| rac1_unknown_74 |  | s32 |
| rac23_unknown_8 |  | s32 |
| rac23_unknown_c |  | s32 |
| rac23_unknown_18 |  | s32 |
| rac23_unknown_1c |  | s32 |
| rac23_unknown_20 |  | s32 |
| rac23_unknown_24 |  | s32 |
| rac23_unknown_38 |  | s32 |
| rac23_unknown_3c |  | s32 |
| rac23_unknown_4c |  | s32 |
| rac23_unknown_84 |  | s32 |

### Tie

Large static objects.
*Fields*

| Name | Description | Type |
| - | - | - |
| o_class |  | s32 |
| occlusion_index |  | s32 |
| directional_lights |  | s32 |
| uid |  | s32 |
| ambient_rgbas |  | bytes |

### Shrub

Small static objects with only a single LOD level and optionally a billboard.
*Fields*

| Name | Description | Type |
| - | - | - |
| o_class |  | s32 |
| unknown_5c |  | s32 |
| dir_lights |  | s32 |
| unknown_64 |  | s32 |
| unknown_68 |  | s32 |
| unknown_6c |  | s32 |

### DirLight

A directional light. This is the main type of light.
*Fields*

| Name | Description | Type |
| - | - | - |
| col_a |  | vec4 |
| dir_a |  | vec4 |
| col_b |  | vec4 |
| dir_b |  | vec4 |

### PointLight

A point light that only affects moby instances. In R&C1, it only affects Ratchet.
*Fields*

| Name | Description | Type |
| - | - | - |
| radius |  | f32 |

### EnvSamplePoint

Sets up lighting, fogging, and sound parameters for an area. The nearest env sample point is used.
*Fields*

| Name | Description | Type |
| - | - | - |
| hero_light |  | s32 |
| music_track |  | s16 |
| hero_colour_r |  | u8 |
| hero_colour_g |  | u8 |
| hero_colour_b |  | u8 |
| enable_reverb_params |  | bool |
| reverb_type |  | u8 |
| reverb_depth |  | s16 |
| reverb_delay |  | u8 |
| reverb_feedback |  | u8 |
| enable_fog_params |  | bool |
| fog_near_intensity |  | u8 |
| fog_far_intensity |  | u8 |
| fog_r |  | u8 |
| fog_g |  | u8 |
| fog_b |  | u8 |
| fog_near_dist |  | s16 |
| fog_far_dist |  | s16 |

### EnvTransition

A volume in which there is a transition in lighting and fogging e.g. a doorway.
*Fields*

| Name | Description | Type |
| - | - | - |
| enable_hero |  | bool |
| hero_colour_1 |  | Rgb32 |
| hero_colour_2 |  | Rgb32 |
| hero_light_1 |  | s32 |
| hero_light_2 |  | s32 |
| enable_fog |  | bool |
| fog_colour_1 |  | Rgb32 |
| fog_colour_2 |  | Rgb32 |
| fog_near_dist_1 |  | f32 |
| fog_near_intensity_1 |  | f32 |
| fog_far_dist_1 |  | f32 |
| fog_far_intensity_1 |  | f32 |
| fog_near_dist_2 |  | f32 |
| fog_near_intensity_2 |  | f32 |
| fog_far_dist_2 |  | f32 |
| fog_far_intensity_2 |  | f32 |

### Cuboid

A cuboid-shaped trigger volume.

### Sphere

A sphere-shaped trigger volume.

### Cylinder

A cylinder-shaped trigger volume.

### Pill

A pill-shaped trigger volume. Appears to be unused.

### Camera

A camera.
*Fields*

| Name | Description | Type |
| - | - | - |
| type |  | s32 |

### Sound

A sound instance.
*Fields*

| Name | Description | Type |
| - | - | - |
| o_class |  | s16 |
| m_class |  | s16 |
| range |  | f32 |

### Path

A spline.

### GrindPath

A spline used for generating a grind rail.
*Fields*

| Name | Description | Type |
| - | - | - |
| unknown_4 |  | s32 |
| wrap |  | s32 |
| inactive |  | s32 |

