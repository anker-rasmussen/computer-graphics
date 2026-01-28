# Lecture 1
## Section 1
What are computer graphics?

Creating images by digital synthesis or manipulation.

The development has made computers easier to interact with, and easier understand/interpret data.

Developments in computer graphics have revolutionised media, movies, medical imaging, product design, and computer games.

Lots of linear algebra - how are images created? How is depth represented? Creating vertex buffers/vertex arrays, basic geometric primitives.

Different types of rendering - different approaches:

Rasterisation (3d objects into an image plane - 3d polygons become 2d polygons then filled in)

Raytracing (back projects rays through each pixel. determines where ray hits 3d objects in the scene (n bounces))

Radiosity (global illumination; computes light energy transferred among surgaces in the scene)

Volume rendering (Shoots rays from camera into scene which is composed of voxels (3d cubes) - each voxel has a color and transparancy value)


