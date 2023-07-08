# Collision Recovery

The collision for all the ties and shrub instances for a level is built into a single world-space mesh. This means that the local space collision meshes for these assets are not stored on the disc, and are hence not unpacked automatically by Wrench.

To address this issue the Wrench editor includes a 'Collision Fixer' tool, which uses an approximate algorithm to recover instanced collision. This isn't perfect, but depending on the class it may prevent you from having to recreate these collision meshes from scratch.

Only the assets from the base game are used for this process, so if you're reaching for this tool because you've already transformed some objects in the editor and the result is incorrect, it should still work.

## Algorithm

For each instance in each level of the specified type and class we transform every vertex of every face in the relevant world-space collision mesh using the inverse model matrix of said instance, and we count how many faces match between the different instances. If the specified threshold is reached, the face is included.

Additionally, we merge vertices that are close together (see below) and reject any faces outside of a bounding box which by default is generated from the regular mesh of the specified tie or shrub class.

## Parameters

### Asset

The tie or shrub class for which you want to recover an instanced collision mesh for.

### Threshold

The minimum number of instances for which a face must exist for it to be included.

### Merge Distance

Two vertices that are closer together than the merge distance will be treated as the same vertex. This is to correct for quantization artifacts introduced by how the game stores the world-space collision meshes.

### Reject Faces Outside BB

If this checkbox is ticked, all faces outside of the bounding box (shown in the preview windows) will not be included. This is helpful to prevent random junk vertices from being included in the generated mesh.

### Bounding Box Origin

Origin position of the previously mentioned bounding box.

### Bounding Box Size

Size of the previously mentioned bounding box.

## Output

The generated mesh will be written to the current mod folder. If a .asset file for the tie or shrub class doesn't already exist it will be auomatically created.

Note that this process does not automatically remove the recovered faces from the world-space collision files and it will not automatically enable the static collision flags on the relevant instances.
