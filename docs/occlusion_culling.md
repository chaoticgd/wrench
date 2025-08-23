# Occlusion Culling

To improve performance, the game wants to skip drawing objects that are entirely occluded (covered up by) other objects. To achieve this occlusion information is computed ahead of time and is stored in the games files.

The playable space of a level is divided up into a grid of 4x4x4 cubes called octants. For each of these octants, a Potentially Visible Set occlusion algorithm determines which objects are visible and which are occluded. This information is stored in a bit mask which is 128 bytes in size (multiple objects are merged into a single bit) and multiple octants may reference the same bit mask. Additionally, a tree is stored to lookup these masks given a position.

The gameplay file contains a section that maps the occlusion indices of tfrags in addition to tie and moby instances to a bit index which is used to lookup if said instance is visible. Note that this system does not apply to shrub instances are they are usually only visible up close.

## Computing Visibility

Wrench implements an OpenGL renderer to compute the occlusion information for a level. For each corner of each octant, 6 square renders are performed with 90 degree FOVs. The visible objects are then enumerated to produce a bit mask of all the objects in the scene. These masks for all the corners of an octant are OR'd together to produce the mask for the octant. These masks are then compressed down to fit in the specified memory budget.
