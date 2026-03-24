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

# Lecture 2

### The OpenGL state machine
OpenGL is a state machine. It has an inherent state represented by internal vars. Vars are in their default state and get changed via OpenGL API calls. It's designed this way so fewer args are passed to OpenGL on each function call.


To enable a state variable, one can call 
 `glEnable(GLenum capability);`
To disable the capability, one calls 
 `glDisable(GLenum capability);`
One can test if a state variable is enabled with
 `glIsEnabled(GLenum capability);`

 ### OpenGL datatypes.

| Data type | Minimum bit width | Description |
  |-----------|-------------------|-------------|
  | GLboolean | 1 | true or false boolean |
  | GLbyte | 8 | Signed 8 bit integer |
  | GLubyte | 8 | Unsigned 8 bit integer |
  | GLint | 32 | Signed 32 bit integer |
  | GLuint | 32 | Unsigned 32 bit integer |
  | GLfloat | 32 | 32 bit floating point |
  | GLclampf | 32 | 32 bit float in the range [0, 1] |
  | (many more) | | | 

### Errors

OpenGL has error flags, and each represents different types of info. When an err occurs, the flag is set and retained until retrieved with the command glGetError().

| Error code | Description |
  |-----------|-------------------|
 | GL_INVALID_ENUM | An enum argument is out of range |
GL_INVALID_VALUE | A numeric argument is out of range
GL_INVALID_OPERATION |The operation is illegal in the current state
GL_OUT_OF_MEMORY | Not enough memory is available 
GL_NO_ERROR | No error has occurred

### Debugging
glGetError() keeps track of errors since OpenGL was initialized, or the last call to glGetError().

| Source code | Error flag |
|-----------|-------------------|
Initialise OpenGL | GL_NO_ERROR (= 0)
Good OpenGL calls | GL_NO_ERROR
Bad OpenGL call | GL_INVALID_OPERATION
Good OpenGL calls | GL_INVALID_OPERATION
GLenum retCode = glGetError();| return GL_INVALID_OPERATION; reset to GL_NO_ERROR

When trying to debug a newline, it's best to make sure GL_NO_ERROR is the flag before calling the code. 

### Primitives
`GL_POINTS` (draws points)

`GL_LINES` (draws lines between points)

`GL_LINE_STRIP` (draws lines between an array of points)

`GL_LINE_LOOP` (draws connections between each instance of a point in an array - circular linked list style)

`GL_TRIANGLES` (shades between 3 points)

`GL_TRIANGLE_STRIP` (shades between 3 points n times)

`GL_TRIANGLE_FAN` (similar to GL_LINE_LOOP).

### Rendering primitives

Rendering in modern OpenGL is done using glDrawArrays (GLenum mode, GLint first, GLsizei count);

The vertices comprising the primitive are stored in the buffer on the GPU.

GLenum mode is the primitive type (one of)
- GL_POINTS
- GL_LINES
- GL_LINE_STRIP
- GL_LINE_LOOP
- GL_TRIANGLES
- GL_TRIANGLE_STRIP
- GL_TRIANGLE_FAN

`GLint first` is teh index to the first element to render, `GLsizei count `is the number of elements to render

### Rendering points

GL_POINTS: each vertex is a point on the screen. Points are 1 px in size, to change this, call `glPointSize(GLfloat size);`

### Rendering Lines

`GL_LINES` : Vertices define line segments
`GL_LINE_STRIP`: Line segments are drawn from the first vertex to each successive vertex
`GL_LINE_LOOP` : Same as strip but first and last are connected.

### Rendering triangles

`GL_TRIANGLES`: Every 3 vertices define a triangle

`GL_TRIANGLE_STRIP`: Triangles share vertices along a strip.

`GL_TRIANGLE_FAN`: Triangles fan out from an origin, sharing adjacent vertices

nb. GPU is optimized for triangle rendering.

### Triangle winding

Triangles have two sides. The front and back are determined using winding (order in which the vertices are given). Counterclockwise vs clockwise.

It's possible to give the front and back sides of the polygon a different appearance, or even not render the backside of the polygon! 

By default, OpenGL assumes triangles with a counterclockwise winding are front facing. This can be changed using glFrontFace(GL_CW); and back again using glFrontFace(GL_CCW);

### Triangles

On the GPU, vertices are stored in a buffer. When one calls glDrawArrays with GL_TRIANGLES, every 3 vertices are rendered as a triangle. (xyz coord for 3 points.)


### Triangle strips 

To maintain a consistent winding, the vertices aren't traversed in the order specified. 

To draw N triangles, GL_Triangles requires 3N vertices.

### Indexed vertices

Useful for modeling geometry. List of vertices and a list of vertex indices.

### Matrices

Important in any engine.

Matrices are a rect array of nums that have n rows m columns. size n*m.

Matrices for which n=m are called square.

Matrices are typically used to represent transformations.

Matrices are often square in graphics programming with size 3x3 or 4x4.

Vectors are matrices with either m or n being 1.

Two matrices are equal if and only if all elements within the matrix are equal (this requires both matrices to be the same size!)

### Submatrices

A submatrix is a part of a matrix (taking various rows/cols from a larger matrix)

Consider the 3x4 matrix. You can create 2 disjoint 2x2 matrices.

### Matrix diagonal
The matrix diagonal is the set of matrix elements along the diagonal of the square matrix.

A diagonal matrix is the set of matrix elements along the diagonal.

### Matrix transposition

Transpose: Switch rows with cols

Addition/subtraction are performed elementwise (need matrix to be the same size)

### Scalar/matrix multiplication

Scalar/matrix multiplication multiplied each element with a scalar.

Two matrices can be multiplied only when the number of clumns in the matrix on the left equal the number of rows in the matrix on the right.

Note: Matrix multiplication is not commutative!! AB !=BA. 

### Identity matrix 
Diagonal 1 - creates the same matrix after multiplying a matrix with it.

### Inverse matrix

Scalar multiplication

(1/a)*a=1

The inverse of a matrix **A** is denoted by A<sup>-1</sup>.

### SoRT

- Scaling
- Rotation
- Translation

# Lecture 3
### Viewing, VBOs, and Splines
### Matrices, matrices, matrices! 
Matrix inversion. Take the determinant, then do 1/ad-bc.

In the case of a rotation matrix, the transpose is equal to the inverse. 

In 3D as in 2D, rotations satisfy orthogonality.
Columns or rows of *R* form an orthonormal basis
  - Each column is a vector of length 1
  - The three columns are orthogonal
  - The columns define a set of axes that span
  - Can be seen as a rotation from one coordinate system to another (source & target space)
The inverse represents the inverrse transformation (rotation from target coordinate space back to source space)

3D rotations are about an axis. Rotations about the coordinate axes 

There are always at least 2 sets of angles that produce the same rotation matrix.

### Viewing and the Synthetic camera model

Imagine you're taking a picture with your camera.
Objects may be best described in a local coordinate system unique to each object. 

Objects have position and orientation, and so does your camera.

Your camera has settings such as size, FoV, and resolution, which helps to define how the final image is rendered.

The synthetic camera model mimics this process, using transformations between different coordinate systems to render an image. These transformations are implemented using matrices.

Modelling matrix -> Viewing matrix -> Projection matrix
-> Viewport Matrix

World coordinates

The camera is implemented using a left handed coordinate system, but all geometry is specified using a right handed coordinate system.

glm::lookAt() provides a simple interface for viewing. Requires (in world coordinates) an eye point, view point, and up vector for "up" for the camera/rendered image.

Produces a viewing matrix (which is used in a vertex shader) to transform points.

To fly through a scene, change the eyepoint and redraw.


Normalize the lookAt vectors!

glm::perspective creates a 4x4 perspective projection matrix used in a shader to transform points.

Often, one sets up the projection transformation and does not change it (unless the user changes the window)

`znear` and `zfar` are distances *from* the camera. A 45 degree vertical fov is fine.

Orthographic projection: Parallel lines remain parallel, and distance to camera does not affect rendering. Objects retain their size regardless of how far they are away from the camera (used in HUDs and CAD)

### Viewport

Specifies dimensions of the 2d window where rendering occurs. The viewport is set to the window size when the rendering context is created.

One sets the viewport when changing the window size.


Rendering a frame
  - Clear the buffers
    (glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);)
  - Set the view (or modelview) matrix
    (glm::lookAt();)
  - Set the model (or modelview) matrix
    glm::translate(), glm::rotate(), glm::scale()
  - Pass matrices to shader
  - Render object
    glDrawArrays()
  - Swap buffers to display rendered frame.

  RGB color
  
  RGB is a color space. (Red, Green, Blue axes)

  Values are from 0.0 to 1.0. Points can be represented as a vector

  RGB uses additive color mixing, because it describes type of light emitted to produce a color. Light is added to create form.

  ### Vertex attributes

  A vertex may have several attributes
  - Position
  - Color
  - Texture coordinate
  - Normal

  Two vertices are equivalent if and only if all their attributes are equal.

### Immediate vs Retained mode. 
(Imediate has been deprecated). Now we use VertexBufferObjects.

These buffers are stored within the GPU. A buffer object is an OpenGl container used to store generic data, and a vertex butter object is one that stores vertex attributes. 

VBOs store per-vertex data that is used by the vertex shader during rendering. 

To use a VBO there are 3 steps
  - Generate a name for thebuffer
    - Bind (activate the buffer)
    - Store data in the buffer (on the gpu!)
  - Render using the buffer (0-n times)
  - Destroy the buffer

  
# Lecture 4
### Shaders
Server side computer program (lives on the GPU), produces rendering effects on graphics hardware. Implements lighting and shading, but can do much more. 
Shaders are more flexible than the fixed function pipeline, but you can create difficult/impossible effects to produce with fixed function pipeline.

Non linear spatial transformation, non-photorealistic rendering, and per-fragment lighting can be handled.

Modern CPUs can do shaders! It's not exclusively running on GPUs.

Shaders were originally written in ASM. Nowadays there's GLSL which allows for a high level implementation.

# Lecture 5
### Surfaces and texture mapping

`Uniform` vs `In` variables

Uniform are sent from the c++ to the GPU. Uniforms are visible in all shaders.

Variables that are `in` are sent from another shader stage (or from the VBO).

Don't load precompiled shaders (it's unlikely a shader compiled on another system will work on another due to drivers or hardware mismatch. Compile the shaders at runtime!)

You can choose not to render backfaces. When outside an opaque closed surface, the backfaces are never visible as they are occluded by nearer faces. They can be culled, therefore not rendering it.

Culling is enabled using `glEnable(GL_CULL_FACE);`

And the side to cull is specified by an enum `glCullFace(GLenum mode)`
- `GL_FRONT`
- `GL_BACK`(default)
- `GL_FRONT_AND_BACK`

Culling backfaces will increase the rendering efficiency (and is enabled by default in the template).

Vertex normal is the average of triangle normals. 

There are many mesh formats as well as model loading libraries. Use any mesh format or loader, as long as it is compatible with the opengl core.

Examples
- 3DS: 3D studio format
- OBJ: Wavefront format
- MD2..MD5: Quake model format
- MAX: 3D Studio Max format
- X3D: XML based format

Open asset import library is a popular loader that supports numerous mesh fomrats and is used in the template.

OBJ format is useful (human readable), Visual Studio has a built in OBJ mesh editor. Does not support animation and OBJ may be slower to load than a binary format like 3DS. 

Texture mapping increases realism without having to geometrically model fine details. If it looks right, it is right.
Texturemapping can be used for many purposes including
- Simulating materials
- Reducing geometric complexity (fewer polygons)
- Image processing
- Reflections & shadows

In texture mapping the visual detail is in the image, not the geometry. 

A game will use many textures. You can make a texture current by binding it, similar to binding a VAO.

Binding is done before the texture is sent to vram. Texture is also bound during rendering so opengl knows which texture to use. Use `glBindTexture(target, id);`
The target is GL_TEXTURE_2D
And once bound, the texture becomes the active texture and is used by all texturing calls until the next glBindTexture() is called.

It's easier and better to use a sampler object, beacuse it stores sampling parameters, providing a simpler way to set texturing parameters than glTexParameter functions.

Aliasing is an artefact that results from undersampling a signal. It is observed as spatial and temporal aliasing.

Can have high frequencies aliasing as low frequencies

Mipmaps remove the high frequencies using a stack of images.

# Anisotropic filtering
Not part of the core OpenGL spec, but widely available as an extension.
Dramatically improves the quality of texture filtering operations.

The standard texture filtering mdoes use texture coordinates to average texel colors.

OpenGL is inherently a 3D api. Sometimes it's convenient to switch to 2D mode based on the screen size in pixels. 

Render the 2D last! 2.5D text could be cool. 







# CWK stuff:
Make a more advanced shape and you'll get a better mark.

The harder it is the better mark you get. 