#pragma once

#include "Texture.h"
#include "VertexBufferObjectIndexed.h"

// Procedural spaceship inspired by the Perhonen from "The Quantum Thief" by Hannu Rajaniemi.
// The Perhonen is described as having a sleek organic hull with gossamer butterfly-like
// solar sail wings. We build it entirely from code — no 3D model file needed.
//
// The geometry is constructed from three parts:
//   1. Hull  — a surface of revolution with a teardrop cross-section profile
//   2. Wings — butterfly-shaped flat meshes defined by "ribs" at different span positions
//   3. Fin   — a simple triangular vertical stabiliser at the tail
//
// All parts share one VAO and one indexed VBO, so a single glDrawElements call renders
// the entire ship.
class CShip
{
public:
	CShip();
	~CShip();

	// Build the ship geometry and upload it to the GPU.
	// directory + filename point to an optional texture (pass empty strings for untextured).
	void Create(string directory, string filename);

	// Bind the VAO and issue the draw call.
	void Render();

	// Free GPU resources (VAO, VBO, texture).
	void Release();

private:
	// --- GPU handles ---
	// A VAO (Vertex Array Object) stores the *layout* of our vertex data:
	// which attributes are enabled, their types, strides, and offsets.
	// Once set up, binding the VAO restores all that state in one call.
	UINT m_vao;

	// The indexed VBO holds two buffers on the GPU:
	//   - a vertex buffer (positions, texture coords, normals interleaved)
	//   - an index buffer  (triplets of vertex indices forming triangles)
	// Using indices lets multiple triangles share vertices, saving memory.
	CVertexBufferObjectIndexed m_vbo;

	// Optional texture that gets sampled in the fragment shader.
	CTexture m_texture;

	// Whether a texture was successfully loaded (controls whether we bind it).
	bool m_hasTexture;

	// Total number of triangles across all ship parts.
	// glDrawElements needs the *index count* (m_numTriangles * 3).
	int m_numTriangles;

	// Running count of vertices added so far. Each helper function uses this
	// as a base offset so its indices don't collide with previous parts.
	unsigned int m_vertexCount;

	// --- Geometry helper: add one vertex ---
	// Every vertex in this project has the same layout (matching the main shader):
	//   attribute 0: vec3 position   (where the vertex sits in model space)
	//   attribute 1: vec2 texCoord   (where to sample the texture)
	//   attribute 2: vec3 normal     (surface direction, used for lighting)
	// Returns the index of the newly added vertex so we can reference it in triangles.
	unsigned int AddVertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal);

	// --- Geometry helper: add one triangle ---
	// Takes three vertex indices (counter-clockwise winding = front face in OpenGL).
	// OpenGL's default front-face convention is GL_CCW, and the template enables
	// GL_CULL_FACE, so back-facing triangles are discarded. Getting winding right matters!
	void AddTriangle(unsigned int i0, unsigned int i1, unsigned int i2);

	// --- Hull shape function ---
	// Returns a value in [0, 1] representing the hull's cross-section radius
	// at parameter t, where t=0 is the nose and t=1 is the tail.
	// The formula produces a teardrop: rises quickly from the nose, peaks around t≈0.38,
	// then tapers gradually toward the tail — like a streamlined aircraft fuselage.
	float HullProfile(float t) const;

	// --- Part builders (each appends vertices + triangles to the shared VBO) ---

	// Hull: a surface of revolution. We sweep the teardrop profile around the Z axis.
	// 'slices' = subdivisions around the circumference, 'stacks' = subdivisions along the length.
	// More slices/stacks = smoother surface but more triangles.
	void CreateHull(float length, float maxRadius, int slices, int stacks);

	void CreateNacelles(float radius, float length, int count=4);

	void CreateSolarSails(float sailHeight, float sailWidth, float curvature, int count=4);

	// Builds a frustum (tapered cylinder) between two 3D points with given radii.
	void CreateCylinder(glm::vec3 from, glm::vec3 to, float r0, float r1, int slices);

	// Evaluates a cubic Bézier curve at parameter t.
	glm::vec3 CubicBezier(glm::vec3 P0, glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, float t) const;
};
