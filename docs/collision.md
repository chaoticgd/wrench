# Collision

## Main Collision

The collision meshes for all static geometry are baked into a single logical mesh for a given chunk (or in the case of R&C1, a given level). This includes collision for tfrags, ties, and shrubs.

This single mesh is then broken down into 4x4x4 cubes called octants during packing, and a tree is generated for efficiently looking up a given octant from a world space position.

Faces in the main collision mesh can be either triangles or quads and have a type associated with them. This determines whether the player can walk on the face or slides off, the sound effect played when the player walks on the face, and whether the face is part of a death or magneboot surface.

## Hero Collision

Hero collision is collision that only affects the player. This is commonly used for invisible walls, fences and grates. Unlike with the main level collision, hero collision is broken down into groups which are stored in an array and each have a bounding sphere. This means that for optimal runtime performance, all vertices in a given group should be close together.

Faces in the hero collision mesh must be triangles and do not have a type associated with them.
