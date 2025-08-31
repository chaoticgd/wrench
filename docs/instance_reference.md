# Instance Reference

This file was generated from instance_schema.wtf and is for version 29 of the instance format.

## Instances

### Moby


*Fields*

| Name | Description | Type |
| - | - | - |
| mission |  | s8 |
| uid |  | s32 |
| bolts |  | s32 |
| update_distance |  | s32 |
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
| has_static_collision |  | bool |

### MobyGroup


*Fields*

| Name | Description | Type |
| - | - | - |
| members |  | mobylinks |

### Tie


*Fields*

| Name | Description | Type |
| - | - | - |
| occlusion_index |  | s32 |
| directional_lights |  | s32 |
| uid |  | s32 |
| ambient_rgbas |  | bytes |
| has_static_collision |  | bool |

### TieGroup


*Fields*

| Name | Description | Type |
| - | - | - |
| members |  | tielinks |

### Shrub


*Fields*

| Name | Description | Type |
| - | - | - |
| unknown_5c |  | s32 |
| dir_lights |  | s32 |
| unknown_64 |  | s32 |
| unknown_68 |  | s32 |
| unknown_6c |  | s32 |
| has_static_collision |  | bool |

### ShrubGroup


*Fields*

| Name | Description | Type |
| - | - | - |
| members |  | shrublinks |

### DirLight


*Fields*

| Name | Description | Type |
| - | - | - |
| col_a |  | vec4 |
| dir_a |  | vec4 |
| col_b |  | vec4 |
| dir_b |  | vec4 |

### PointLight


*Fields*

| Name | Description | Type |
| - | - | - |
| radius |  | f32 |

### EnvSamplePoint


*Fields*

| Name | Description | Type |
| - | - | - |
| hero_light |  | s32 |
| music_track |  | s16 |
| hero_col |  | vec3 |
| enable_reverb_params |  | bool |
| reverb_type |  | u8 |
| reverb_depth |  | s16 |
| reverb_delay |  | u8 |
| reverb_feedback |  | u8 |
| enable_fog_params |  | bool |
| fog_near_intensity |  | u8 |
| fog_far_intensity |  | u8 |
| fog_col |  | vec3 |
| fog_near_dist |  | s16 |
| fog_far_dist |  | s16 |

### EnvTransition


*Fields*

| Name | Description | Type |
| - | - | - |
| enable_hero |  | bool |
| hero_col_1 |  | vec3 |
| hero_col_2 |  | vec3 |
| hero_light_1 |  | s32 |
| hero_light_2 |  | s32 |
| enable_fog |  | bool |
| fog_col_1 |  | vec3 |
| fog_col_2 |  | vec3 |
| fog_near_dist_1 |  | f32 |
| fog_near_intensity_1 |  | f32 |
| fog_far_dist_1 |  | f32 |
| fog_far_intensity_1 |  | f32 |
| fog_near_dist_2 |  | f32 |
| fog_near_intensity_2 |  | f32 |
| fog_far_dist_2 |  | f32 |
| fog_far_intensity_2 |  | f32 |

### Cuboid


### Sphere


### Cylinder


### Pill


### Camera


### Sound


*Fields*

| Name | Description | Type |
| - | - | - |
| m_class |  | s16 |
| range |  | f32 |

### Path


### GrindPath


*Fields*

| Name | Description | Type |
| - | - | - |
| unknown_4 |  | s32 |
| wrap |  | s32 |
| inactive |  | s32 |

### Area


*Fields*

| Name | Description | Type |
| - | - | - |
| last_update_time |  | s32 |
| paths |  | pathlinks |
| cuboids |  | cuboidlinks |
| spheres |  | spherelinks |
| cylinders |  | cylinderlinks |
| negative_cuboids |  | cuboidlinks |

### SharedData


