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







# Lecture 8
### Intermediate / Advanced Graphics 1

### Toon shader with silhouette

Improve the toon shader (from lecture 4) by adding a black silhouette outline. A silhouette is the outline of the object as observed from the viewer. It changes if the camera moves but not if a light source moves.

To detect silhouette edges: compute the dot product of the *normal* and *viewing* vectors. If it is below a threshold, the fragment is on the edge -- set it to black:
```glsl
vec3 n = normalize(n);
vec3 v = normalize(-p.xyz);
float edgeMask = (dot(v, n) < 0.4) ? 0 : 1;
vec3 toonColour = edgeMask * quantisedColour;
```

### Camera shake

Perturbs the camera to simulate shake. Can be translation or rotation, but rotation is more effective since the skybox is stationary w.r.t. camera translations. Implement by computing small random rotation angles and linearly interpolating over time.

### Environment mapping

Texture mapping technique to simulate reflection or refraction by modelling the background environment. Uses a *cubemap texture* (six square 2D images forming a cube, like a skybox).

- **Reflection mapping**: texture coordinates simulate light reflecting off the surface (e.g. Terminator 2 chrome)
- **Refraction mapping**: texture coordinates simulate light refracting through the surface

**Cubemap textures**: Single texture object of six square images. Loaded via `glTexImage2D` with `GL_TEXTURE_CUBE_MAP_POSITIVE_X` through `NEGATIVE_Z` targets. Accessed in fragment shader with `samplerCube` using 3D texture coordinates [s, t, r] interpreted as a direction vector from the cube centre.

**Skybox rendering with cubemap**: Vertex shader passes `worldPosition = inPosition` (cube centred at origin). Fragment shader samples `texture(CubeMapTex, worldPosition)`.

**Reflection implementation**: Use GLSL `reflect` function. The reflected vector must be in world coordinates. Two approaches:
1. Reflect in world coordinates (needs model matrix separate from view matrix)
2. Reflect in eye coordinates, then rotate back to world using the inverse view matrix

```glsl
// Vertex shader (eye-space approach)
vec3 n = normalize(matrices.normalMatrix * inNormal);
vec3 p = (matrices.modelViewMatrix * vec4(inPosition, 1.0f)).xyz;
reflected = (matrices.inverseViewMatrix * vec4(reflect(p, n), 1)).xyz;
// Fragment shader
vOutputColour = texture(cubeMapTex, normalize(reflected));
```

**Refraction**: Light changes direction through materials with different refractive indices. Governed by Snell's law: sin(theta_r)/sin(theta_i) = n_i/n_r = eta. For air/glass, eta = 0.66. Uses GLSL `refract(p, n, eta)` instead of `reflect`.

### Alpha maps

Load an RGBA texture and `discard` fragments whose alpha is below a threshold. Creates complex-looking cutout shapes from simple geometry.

### Lightmapping

Stores pre-computed light intensity as a texture (*lightmap*). Applied via multi-texturing: lightmap * texture = lit texture. First used in Quake. Efficient for simulating many lights on static geometry. The pre-computation process is called *baking*.

### Bump mapping (normal mapping)

Simulates bumps and wrinkles by perturbing the surface normal per-fragment. The geometry itself is unchanged -- only the normals used in lighting calculations change. Implemented with multi-texturing: load a normal displacement texture (RGB values represent displacements in tangent space). During rendering, displaced normals are used for lighting.

### Fog

Adds realism and prevents geometry from popping into view at the far clip plane. Computed in the fragment shader by blending scene colour **S** with fog colour **F**:
**C** = w**S** + (1-w)**F**

Fog models (w = fraction of original colour):
- **Linear**: w = (f_e - d) / (f_e - f_s) where f_s/f_e are start/end fog distances
- **Exponential**: w = exp(-rho * d)
- **Square exponential**: w = exp(-(rho * d)^2)

Where d = distance from camera (length of position in eye coordinates), rho = fog density. Use GLSL `mix(fogColour, sceneColour, w)` for blending.

### Shadows

Shadows enhance realism and illustrate spatial relationships. OpenGL implements a local lighting model (each primitive rendered independently), so shadows are not built-in. Terminology: *occluder* blocks light, *receiver* receives the shadow.

**1. Planar shadows**: Project occluder primitives from the light source onto a receiver plane. Solve ray/plane intersection: given light **l**, vertex **v**, direction **q** = **v** - **l**, and plane ax + by + cz + d = 0, solve for t and compute projected point. This can be expressed as a *shadow matrix*: **M** = (**p** dot **l**)**I** - **l****p**^T. In GLM: `glm::dot(plane, lightPos) * glm::mat4(1) - glm::outerProduct(lightPos, plane)`.

Issues: z-fighting (shadow coplanar with ground), shadow same colour as object. Fix 1: alpha blend a dark colour, disable depth testing -- but causes *multiple shadowing* (overlapping shadow polygons darken twice). Fix 2: use the stencil buffer to only darken each fragment once.

Limitations of planar shadows: only works on planar receivers, no self-shadowing, anti-shadows.

**2. Shadow mapping**: Models shadow casting as a *visibility problem* from the light source. Render a depth map from the light's perspective (the *shadow map*). Each pixel stores distance to nearest surface. When rendering the scene from the camera, compare each fragment's distance to the light against the shadow map value. If fragment distance > shadow map value, the fragment is in shadow (something closer is blocking the light). Allows self-shadows and shadows on arbitrary shapes. Accuracy limited by shadow map resolution.

**3. Shadow volumes**: Construct polyhedral regions occluded from a light source using the stencil buffer. Divides scene into shadowed/unshadowed regions. Used famously in Doom 3.

### Instanced rendering

Efficiently render many instances of the same object (grass, trees, armies). Instead of looping N calls to `glDrawArrays`, use a single `glDrawArraysInstanced(GL_TRIANGLES, 0, m_iNumVertices, N)`. In the vertex shader, `gl_InstanceID` provides the instance index for per-instance transformations. Example: 1000 horses at 1581 triangles each -- instanced: 60 FPS, non-instanced: 40 FPS.

### Anti-aliasing

*Jaggies* are staircasing artefacts from limited framebuffer resolution (undersampling).

**SSAA (Supersample anti-aliasing)**: Render to a higher resolution framebuffer, then downsample. Best quality but very costly (2x width+height = 4x fragments). Often unworkable for real-time.

**MSAA (Multi-sample anti-aliasing)**: Generate several fragments with slightly different sub-pixel locations and average. Runs the fragment program *once* per rasterised fragment (much cheaper than SSAA). Sample locations are pseudo-random. Enable by allocating an MSAA buffer (`WGL_SAMPLE_BUFFERS_ARB`, `WGL_SAMPLES_ARB`) and calling `glEnable(GL_MULTISAMPLE)`. Example: 8x MSAA at 300 FPS vs no AA at 875 FPS.

# Lecture 9
### Intermediate / Advanced Graphics 2

### Geometry shaders

Geometry shaders sit between the vertex and fragment shader in the pipeline. They are optional.

- Vertex shaders process one *vertex* at a time
- Geometry shaders process one *primitive* at a time
- Fragment shaders process one *fragment* at a time

Vertex shaders are strictly one-in one-out: they cannot create new vertices or discard the vertex. Fragment shaders cannot access other fragments' info and cannot create new fragments (but can `discard`). Geometry shaders have access to **all vertices in the primitive** and can change the primitive type, create new primitives, and discard primitives. However, they can only output one primitive *type*.

To use a geometry shader in the template, load it from file (`.geom`) and add it to the shader program alongside the `.vert` and `.frag` shaders.

**Pass-through geometry shader structure:**

Inputs from vertex shader are arrays (since the geometry shader sees all vertices of the primitive):
```glsl
in vec3 vColourPass[];
in vec2 vTexCoordPass[];
out vec3 vColour;
out vec2 vTexCoord;
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
```

Input primitive types: `points`, `lines`, `triangles`, `lines_adjacency`, `triangles_adjacency`

Output primitive types: `points`, `line_strip`, `triangle_strip`

The main loop iterates over vertices, sets `gl_Position` from `gl_in[i].gl_Position`, copies attributes, calls `EmitVertex()`, then `EndPrimitive()`.

**Duplication shader:** Duplicates geometry by emitting primitives twice (once normally, once translated). Requires `max_vertices = 6` for triangles. The vertex shader passes raw positions (`gl_Position = vec4(inPosition, 1.0f)`) and the geometry shader applies the modelview/projection matrices, allowing it to manipulate positions before projection.

**Explosion shader:** Displaces each triangle along its face normal by an `explodeFactor` uniform. Computes the triangle normal from edge cross products, then offsets each vertex position before applying modelview/projection.

**Porcupine rendering:** Renders a mesh twice -- once normally, then again with a geometry shader that draws lines along vertex normals (useful for debugging normals).

### Computer animation

Digital successor to traditional techniques (frame-by-frame, cel animation, stop motion).

**Types:**
- **Keyframing** -- animator specifies key poses, intermediate frames are interpolated (*tweening*). State can include position, shape, colour, etc.
- **Motion capture (mocap)** -- captures real-world motion data (optical or magnetic/radio sensors) and transfers it to a digital model. Primary technique for game animation. Challenges: data acquisition, associating motion to geometry, editing data. Snippets are arranged so endpoints match for smooth transitions.
- **Procedural** -- animation described algorithmically as a function of parameters (e.g. clock hands based on time, computational fluid dynamics)

### Skeletal animation

Articulated character models use a set of *bones* in a kinematic chain (typically 20-25 bones for a human). This is called skeletal (bone) animation, supported by MD3/MD5 formats.

**Rigging** is building the animation controls -- defining the skeleton and associating it to the mesh. Bones provide a simple way to set poses at different keyframes.

**Skinning** (skeletal subspace deformation): mesh vertices are the "skin" around bones. When the skeleton moves, the mesh deforms smoothly. Near joints, vertices are associated with multiple bones using weights (e.g. V2 = 0.75 of B1, 0.25 of B2).

### Kinematics

**Forward kinematics:** Animator specifies joint angles, bone positions are computed. For a 2-bone chain:
- x = L1 cos(theta1) + L2 cos(theta1 + theta2)
- y = L1 sin(theta1) + L2 sin(theta1 + theta2)

Not always intuitive for animators.

**Inverse kinematics:** Animator specifies where the end effector should be (e.g. hand position), and the system solves for joint angles. Generally requires solving complex non-linear equations -- typically done as an optimisation problem searching configuration space for minimum error. May have multiple global minima.

### Particle animation

Collections of point masses moving in 3D subject to rules (usually physics-based). Used for fire, smoke, sparks, magic effects, clouds.

*Particle systems* consist of an *emitter* (source in 3D space setting initial velocity, colour, etc.). Particles change over time: fade out, change colour/alpha.

**Minimum particle properties:** life, position, velocity. Optional: acceleration, size, rotation, colour sequence.

**CPU vs GPU particles:**
- CPU: simulate physics on CPU, apply transforms to modelview matrix, render each particle
- GPU (direct): send initial position, start time, velocity; evaluate equation of motion in vertex shader
- GPU (integration): numerical integration in vertex shader each time step

### Ambient occlusion

The ambient term in the lighting equation models indirect light. Standard ambient is constant everywhere. In reality, creases and gaps between objects *occlude* ambient light.

AO introduces an *accessibility* factor measuring how much ambient light reaches a surface point:
AO(p) = (1/pi) * integral over hemisphere of rho(p, omega) * n dot omega * d_omega

**Baked AO:** Pre-calculate the AO map in a tool like Blender and store as a texture. Efficient at runtime but static -- doesn't account for other moving objects.

**SSAO (Screen Space Ambient Occlusion):** Real-time AO, first used in Crysis. Uses Monte-Carlo sampling in a multi-pass approach:
1. Render scene to texture
2. Randomly sample the depth buffer around each point p; count closer samples that block light. Encode occlusion as [0, 1]
3. Blur the result and darken fragments based on AO term

### Framerate considerations

Target 30-60 FPS. Too low breaks the illusion of motion; too high wastes frames beyond the monitor refresh rate. V-sync locks the frame rate to the display refresh.

### Performance optimisation

**Profile first** (e.g. NVidia Nsight Explorer).

Hardware optimisations:
- Use element arrays / triangle strips to reduce VBO vertices
- Use built-in GLSL functions; eliminate unnecessary GLSL code
- Move fragment shader computations to vertex shader where possible
- Instanced rendering
- Minimise state changes (use VAOs, texture samplers)

Software optimisations:
- Level of Detail (LOD) -- use lower-res meshes for distant objects. Tessellation shaders help here
- Reduce multi-pass algorithm passes
- Reduce/disable texture anisotropy
- Cull objects outside view frustum
- Enable backface culling

### Forward vs deferred rendering

**Forward rendering:** Lighting computed before the depth test. Every fragment (even occluded ones) evaluates the lighting equation for all lights. Wasteful with many lights.

**Deferred rendering (deferred shading):** Two-pass approach:
1. Render geometry to a *g-buffer* (colour, normals, depth, etc. for visible fragments only)
2. Compute lighting only for fragments in the g-buffer

Pros: scales better with many light sources. Cons: additional storage for the g-buffer; harder when different materials are needed.

### Culling and clipping

Primitives completely outside the view frustum can be *culled* (skipped). Partially visible primitives are *clipped*. The frustum has 6 planar sides.

**Trivial acceptance test:** Use point/plane test (sign of **p** dot **v**). If all 8 vertices of an object's bounding box are in the negative half-space of any frustum plane, skip the entire object.

For non-trivial cases (partial overlap), clipping algorithms like Cohen-Sutherland handle 2D/3D clipping.

### Spatial organisation

For large scenes, use spatial data structures to limit rendering to visible portions:

**Octree:** Hierarchical tree partitioning 3D space. Start with a bounding box around all objects (root). Subdivide into 8 child voxels. Place objects in respective nodes. Recursively subdivide until MAX_LEVEL or node is empty. For visibility queries, only test nodes whose bounding boxes intersect the view frustum -- avoids testing distant/irrelevant objects.

BSP trees, portals, and hierarchical occlusion maps are other approaches.

### OOP with OpenGL

OpenGL itself has no notion of OOP -- it's just a hardware interface. But C++ programmers can wrap 3D objects in classes with `initialise()`, `render()`, `translate()`, `rotate()`, `setcolor()` methods. A base `GraphicalObject` class with virtual `Render()` can be subclassed (e.g. `Triangle : public GraphicalObject`).

### Scene graphs

A tree data structure for scene representation. Node types:
- **Shape**: 3D geometric objects
- **Transform**: affect current transformation
- **Property**: appearance, texture, etc.
- **Group**: collection of sub-graphs

The OpenGL state is updated during traversal (top to bottom, left to right). Parent operations propagate to all children. Useful for articulated figures -- transformation matrices concatenate at each group level.

# CWK stuff:
Make a more advanced shape and you'll get a better mark.

The harder it is the better mark you get.