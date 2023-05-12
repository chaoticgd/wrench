# Renderers

The R&C PS2 engine uses multiple different renderers for drawing different types of objects.

## Graphics on the PS2

For most games, the high-level flow of data when the PS2 is drawing graphics looks something like this:

```
+-----+    +------+    +-----+       +-----+    +----+
|     |--->|      |--->|     | PATH1 |     |    |    |
| RAM |    | VIF1 |    | VU1 |------>| GIF |--->| GS |---> Video Out
|     |-+  |      |-+  |     |       |     |    |    |
+-----+ |  +------+ |  +-----+       |     |    +----+
        |           |          PATH2 |     |
        |           +--------------->|     |
        |                            |     |
        +--------------------------->|     |
                               PATH3 +-----+
```

Model data is stored in RAM in the form of a VIF DMA packet. Static data can be left in memory between frames, however dynamic data will usually be written out as a new command list every frame. This command list is then sent through DMA channel 1 to Vector unit InterFace 1 (VIF1). This unit will interpret the command list, decompressing data and code into VU1's data memory and program memory respectively. It will also control microprogram execution on the VU.

The actual VU1 microprogram will usually apply a matrix transformation to all the vertices and then write them out to a GS packet which is sent to the Graphics InterFace (GIF). This unit will read the GS packet and use it to control the GS during rendering. The Graphics Synthesizer is a fixed-function unit that draws triangles. It also has 2MB of on-board memory which is used to store textures and framebuffers.

This is known as PATH1 rendering. PATH2 rendering bypasses VU1 using a `DIRECT` VIF code. PATH3 rendering bypasses VIF1 entirely.

## [Moby Renderer](moby_renderer.md)

Used for drawing animated or interactive objects such as the player and enemies.

## [Shrub Renderer](shrub_renderer.md)

Used for drawing small bits of scenery that should only be visible up close. Inherited from Naughty Dog.

## Sky Renderer

Used for drawing the sky.

## [Tfrag Renderer](tfrag_renderer.md)

Used for drawing large non-instanced geometry. Inherited from Naughty Dog.

## Tie Renderer

Used for drawing large instanced geometry. The LOD system is similar to that of the tfrag renderer. Also inherited from Naughty Dog.
