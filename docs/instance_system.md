# Instance System

The instance system stores and manages all of the objects in each game world.

## Instances File

On disk the instances are stored in [Wrench Text Format](wrench_text_format.md) files referenced from .asset files. This file consists of a level_settings node for global parameters and nodes for each instance.

## Types of Instance

See [Instance Reference](instance_reference.md).

## Pvars

Each moby instance, camera instance and sound instance can have extra data associated with it. The structure of this data depends on the class of the respective instance. Wrench defines this data in the form of C++ type definitions which are stored in asset banks under the src/game_common/ and src/game_\<game\>/ directories (where \<game\> is rac, gc, uya or dl).

These files are read using a custom C++ parser. See [C++ Parser](cpp_parser.md).

Some pvar instances start with a standard header, called the sub var header. This header contains pointers which points to sections within the pvar data which are shared between multiple classes.

### TargetVars

### npcVars

### SuckVars

### BogeyVars

### ReactVars

### ScriptVars

### MoveVars

### MoveVars_V2

### ArmorVars

### TransportVars

### EffectorVars

### CommandVars

### RoleVars

### FlashVars

### TransportVars

### SuckVars

### NavigationVars

### ObjectiveVars
