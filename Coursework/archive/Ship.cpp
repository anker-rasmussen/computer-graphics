#include "Common.h"

#define _USE_MATH_DEFINES

#include "Ship.h"
#include <math.h>
#include <vector>

CShip::CShip()
	// Initialise everything to safe defaults.
	// m_vao = 0 means "no VAO allocated yet" (OpenGL reserves 0 as the null handle).
	: m_vao(0), m_hasTexture(false), m_numTriangles(0), m_vertexCount(0)
{}

CShip::~CShip()
{}

// ---------------------------------------------------------------------------
// AddVertex — the fundamental building block.
//
// Our vertex format is: [vec3 position][vec2 texcoord][vec3 normal]
// This must match the vertex attribute pointers we set up in Create()
// and the 'layout(location = ...)' declarations in the vertex shader.
//
// We return the index so callers can say:
//   uint a = AddVertex(...);
//   uint b = AddVertex(...);
//   uint c = AddVertex(...);
//   AddTriangle(a, b, c);
// ---------------------------------------------------------------------------
uint CShip::AddVertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal)
{
	// AddVertexData copies raw bytes into a CPU-side buffer.
	// Later, UploadDataToGPU sends it all to the GPU in one go.
	m_vbo.AddVertexData(&pos, sizeof(glm::vec3));    // 12 bytes (3 floats)
	m_vbo.AddVertexData(&uv, sizeof(glm::vec2));     //  8 bytes (2 floats)
	m_vbo.AddVertexData(&normal, sizeof(glm::vec3));  // 12 bytes (3 floats)
	// Total stride per vertex: 32 bytes

	return m_vertexCount++;
}

// ---------------------------------------------------------------------------
// AddTriangle — records three vertex indices forming one triangle.
//
// Winding order matters! OpenGL uses counter-clockwise (CCW) winding to
// determine the front face. Since GL_CULL_FACE is enabled, triangles whose
// vertices appear clockwise from the camera's viewpoint are discarded.
//
// So when you look at a triangle from the OUTSIDE of the model, its three
// vertices should go counter-clockwise. If you get it backwards, the
// triangle becomes invisible (culled) from the outside and visible from
// inside — not what we want!
// ---------------------------------------------------------------------------
void CShip::AddTriangle(uint i0, uint i1, uint i2)
{
	m_vbo.AddIndexData(&i0, sizeof(uint));
	m_vbo.AddIndexData(&i1, sizeof(uint));
	m_vbo.AddIndexData(&i2, sizeof(uint));
	m_numTriangles++;
}

// ---------------------------------------------------------------------------
// HullProfile — defines the ship's silhouette.
//
// Given a parameter t in [0, 1] (0 = nose tip, 1 = tail tip), returns the
// cross-section radius at that point, normalised to [0, 1].
//
// The formula  t^0.5 * (1-t)^0.8  produces a teardrop:
//   - At t=0 (nose): 0^0.5 = 0, so the radius is 0 → pointy nose
//   - Rises quickly (the 0.5 exponent means a sqrt-like fast rise)
//   - Peaks around t ≈ 0.385 (found by setting the derivative to zero)
//   - Falls gradually (the 0.8 exponent gives a gentler taper than the rise)
//   - At t=1 (tail): (1-1)^0.8 = 0, so the radius is 0 → closed tail
//
// We normalise by dividing by the peak value so the return range is [0, 1],
// making it easy to multiply by any desired maxRadius.
// ---------------------------------------------------------------------------
float CShip::HullProfile(float t) const
{
	// Clamp to avoid NaN from negative bases in pow()
	if (t <= 0.0f || t >= 1.0f) return 0.0f;

	float raw = powf(t, 0.5f) * powf(1.0f - t, 0.8f);

	// Pre-computed peak: occurs at t = 0.5 / (0.5 + 0.8) = 5/13
	// peak_value = (5/13)^0.5 * (8/13)^0.8
	float peak = powf(5.0f / 13.0f, 0.5f) * powf(8.0f / 13.0f, 0.8f);

	return raw / peak;  // Now in [0, 1]
}

// ---------------------------------------------------------------------------
// CreateHull — builds a surface of revolution.
//
// Imagine the hull profile as a curve on the right side of a vertical axis.
// We spin that curve 360° around the axis, sampling at 'slices' angles and
// 'stacks' positions along the length. This produces a grid of vertices that
// we connect into a mesh of quads (each split into two triangles).
//
// The cross-section is slightly elliptical (wider than tall) for a more
// aerodynamic/organic look — the Perhonen isn't a simple cylinder.
//
// Coordinate convention (model space, matching the template):
//   Z = forward (nose at +Z, tail at -Z)
//   X = right
//   Y = up
// ---------------------------------------------------------------------------
void CShip::CreateHull(float hullLength, float maxRadius, int slices, int stacks)
{
	// These scale factors make the cross-section an ellipse instead of a circle.
	// widthScale > 1  → hull is wider than a circle
	// heightScale < 1 → hull is flatter than a circle
	// The Perhonen should feel sleek and organic, not like a pipe.
	float widthScale = 1.2f;
	float heightScale = 0.8f;

	// Record which vertex index this part starts at.
	// Since the hull is created first, this will be 0.
	uint baseVertex = m_vertexCount;

	// --- Generate vertices ---
	// Outer loop: stacks along the hull length (nose to tail)
	// Inner loop: slices around the circumference
	for (int j = 0; j <= stacks; j++) {
		// t goes from 0 (nose) to 1 (tail)
		float t = (float)j / (float)stacks;

		// Z position: nose at +halfLength, tail at -halfLength
		float z = hullLength * (0.5f - t);

		// Cross-section radius at this stack, from our teardrop profile
		float r = maxRadius * HullProfile(t);

		for (int i = 0; i <= slices; i++) {
			// theta goes from 0 to 2π around the circumference.
			// We use <= slices (not <) so the last vertex coincides with the first,
			// giving correct texture coordinates (u=0 and u=1 are the same position
			// but different texcoords, avoiding a texture seam).
			float theta = (float)i / (float)slices * 2.0f * (float)M_PI;

			// Position: polar coords (r, theta) → cartesian (x, y), scaled elliptically
			float x = r * cosf(theta) * widthScale;
			float y = r * sinf(theta) * heightScale;
			glm::vec3 pos(x, y, z);

			// --- Normal computation ---
			// For lighting to work, the GPU needs to know which direction the surface
			// faces at each vertex. For a surface of revolution, the normal points
			// radially outward from the axis, but we need to account for:
			//   a) The elliptical scaling (an ellipse's normal isn't simply the radius)
			//   b) The profile slope (the hull curves along its length too)

			glm::vec3 normal;
			if (r > 0.001f) {
				// Radial component: for an ellipse (x/a)^2 + (y/b)^2 = 1, the outward
				// normal at angle theta is proportional to (cos(theta)/a, sin(theta)/b).
				// This is because stretching a circle into an ellipse tilts the normals.
				normal = glm::normalize(glm::vec3(
					cosf(theta) / widthScale,
					sinf(theta) / heightScale,
					0.0f
				));

				// Longitudinal component: the profile curves along Z, so the normal
				// tilts forward/backward. We approximate the slope with a finite difference.
				float dt = 0.01f;
				float r0 = HullProfile(glm::clamp(t - dt, 0.0f, 1.0f));
				float r1 = HullProfile(glm::clamp(t + dt, 0.0f, 1.0f));
				// slope = dr/dt (how fast the radius changes with length)
				float slope = (r1 - r0) / (2.0f * dt) * maxRadius;
				// Convert to a Z-direction normal contribution.
				// A positive slope (widening toward tail) tilts the normal toward +Z (nose).
				float zNorm = slope / hullLength;
				normal = glm::normalize(normal + glm::vec3(0.0f, 0.0f, zNorm));
			} else {
				// At the nose/tail tips (radius ≈ 0), all vertices collapse to one point.
				// The normal should point straight along the axis.
				normal = glm::vec3(0.0f, 0.0f, (t < 0.5f) ? 1.0f : -1.0f);
			}

			// Texture coordinates: u wraps around the circumference, v goes nose→tail
			glm::vec2 uv((float)i / slices, t);

			AddVertex(pos, uv, normal);
		}
	}

	// --- Generate triangle indices ---
	// Connect the vertex grid into quads, each split into two triangles.
	//
	// For each quad defined by four corners:
	//   v0 (this stack, this slice)     v2 (this stack, next slice)
	//   v1 (next stack, this slice)     v3 (next stack, next slice)
	//
	// We emit two triangles: (v0, v1, v2) and (v2, v1, v3).
	// This winding is CCW when viewed from outside (the normal points outward).
	for (int j = 0; j < stacks; j++) {
		for (int i = 0; i < slices; i++) {
			uint v0 = baseVertex + j * (slices + 1) + i;
			uint v1 = baseVertex + (j + 1) * (slices + 1) + i;
			uint v2 = baseVertex + j * (slices + 1) + (i + 1);
			uint v3 = baseVertex + (j + 1) * (slices + 1) + (i + 1);

			AddTriangle(v0, v1, v2);
			AddTriangle(v2, v1, v3);
		}
	}
}

// ---------------------------------------------------------------------------
// CreateSolarSails — 4 diamond-shaped solar sails with Bézier-curved edges
// plus 8 connecting spindles (2 per nacelle).
//
// Sail angles: 80°, 170°, 260°, 350° (midpoints between adjacent nacelles).
// Diamond corners: forward tip at Z≈+9, aft tip at Z≈+1, lateral tips at
// adjacent nacelle radial positions.
//
// Each edge is a cubic Bézier bowed outward. The interior is tessellated as
// a Coons patch with a concavity dip so the sail billows like it catches
// solar wind. Double-sided triangles (both windings) for visibility from
// either side.
// ---------------------------------------------------------------------------
void CShip::CreateSolarSails(float sailHeight, float sailWidth, float curvature, int count)
{
	float attachmentAngles[] = { 35.0f, 125.0f, 215.0f, 305.0f };
	float sailAngles[] = { 80.0f, 170.0f, 260.0f, 350.0f };

	// Hull geometry params
	int gridN = 16; // tessellation resolution
	float spindleRadius = 0.02f;
	int spindleSlices = 8;

	// Hull nose is at Z = +hullLength/2 = +5
	float hullNoseZ = 5.0f;
	// Sail radial spread — how far the lateral tips extend from the axis
	float lateralR = sailWidth;

	for (int s = 0; s < count; s++) {
		float sailAngleRad = sailAngles[s] * (float)M_PI / 180.0f;

		// Radial and tangential directions for this sail
		float cx = cosf(sailAngleRad);
		float cy = sinf(sailAngleRad);
		// Tangent: perpendicular to radial in the XY plane
		float tx = -cy;
		float ty = cx;

		// Sail angles inward toward the axis as it goes forward — sails
		// converge ahead of the ship like a funnel catching solar wind.
		// Aft corner: on the hull surface, out radially (attachment point)
		glm::vec3 aft(cx * lateralR, cy * lateralR, hullNoseZ);
		// Forward corner: on the axis, shorter reach ahead
		glm::vec3 forward(0.0f, 0.0f, 13.0f);
		// Two lateral corners: offset in the tangential direction at the midpoint
		glm::vec3 mid = (aft + forward) * 0.5f;
		float halfSpan = 3.5f; // half-width of the sail — wider spread
		glm::vec3 lateralA = mid + glm::vec3(tx * halfSpan, ty * halfSpan, 0.0f);
		glm::vec3 lateralB = mid - glm::vec3(tx * halfSpan, ty * halfSpan, 0.0f);

		// Bézier curvature: bow edges outward (away from ship axis)
		auto bowOut = [&](glm::vec3 midpoint) -> glm::vec3 {
			return glm::vec3(cx, cy, 0.0f) * curvature;
		};

		// Bézier control points for each edge
		// Edge bottom: aft → lateralB
		glm::vec3 e0_P0 = aft, e0_P3 = lateralB;
		glm::vec3 e0_mid = (e0_P0 + e0_P3) * 0.5f;
		glm::vec3 e0_bow = bowOut(e0_mid);
		glm::vec3 e0_P1 = glm::mix(e0_P0, e0_P3, 1.0f/3.0f) + e0_bow;
		glm::vec3 e0_P2 = glm::mix(e0_P0, e0_P3, 2.0f/3.0f) + e0_bow;

		// Edge right: lateralB → forward
		glm::vec3 e1_P0 = lateralB, e1_P3 = forward;
		glm::vec3 e1_mid = (e1_P0 + e1_P3) * 0.5f;
		glm::vec3 e1_bow = bowOut(e1_mid);
		glm::vec3 e1_P1 = glm::mix(e1_P0, e1_P3, 1.0f/3.0f) + e1_bow;
		glm::vec3 e1_P2 = glm::mix(e1_P0, e1_P3, 2.0f/3.0f) + e1_bow;

		// Edge top: forward → lateralA
		glm::vec3 e2_P0 = forward, e2_P3 = lateralA;
		glm::vec3 e2_mid = (e2_P0 + e2_P3) * 0.5f;
		glm::vec3 e2_bow = bowOut(e2_mid);
		glm::vec3 e2_P1 = glm::mix(e2_P0, e2_P3, 1.0f/3.0f) + e2_bow;
		glm::vec3 e2_P2 = glm::mix(e2_P0, e2_P3, 2.0f/3.0f) + e2_bow;

		// Edge left: lateralA → aft
		glm::vec3 e3_P0 = lateralA, e3_P3 = aft;
		glm::vec3 e3_mid = (e3_P0 + e3_P3) * 0.5f;
		glm::vec3 e3_bow = bowOut(e3_mid);
		glm::vec3 e3_P1 = glm::mix(e3_P0, e3_P3, 1.0f/3.0f) + e3_bow;
		glm::vec3 e3_P2 = glm::mix(e3_P0, e3_P3, 2.0f/3.0f) + e3_bow;

		// Coons patch: bilinear blend of boundary curves
		// c_bottom(s) = edge0(s),  c_top(s) = edge2(1-s)
		// c_left(t) = edge3(1-t), c_right(t) = edge1(t)
		// Corners: P00=aft, P10=lateralB, P01=lateralA, P11=forward

		uint baseVert = m_vertexCount;

		for (int j = 0; j <= gridN; j++) {
			float t = (float)j / gridN;
			for (int i = 0; i <= gridN; i++) {
				float sv = (float)i / gridN;

				// Boundary curves
				glm::vec3 bottom = CubicBezier(e0_P0, e0_P1, e0_P2, e0_P3, sv);
				glm::vec3 top = CubicBezier(e2_P0, e2_P1, e2_P2, e2_P3, 1.0f - sv);
				glm::vec3 left = CubicBezier(e3_P0, e3_P1, e3_P2, e3_P3, 1.0f - t);
				glm::vec3 right = CubicBezier(e1_P0, e1_P1, e1_P2, e1_P3, t);

				// Bilinear interpolation of corners
				glm::vec3 bilinear = (1-sv)*(1-t)*aft + sv*(1-t)*lateralB
				                   + (1-sv)*t*lateralA + sv*t*forward;

				// Coons patch formula
				glm::vec3 pos = (1-t)*bottom + t*top + (1-sv)*left + sv*right - bilinear;

				// Concavity dip — sail belly sags aft
				pos.z -= 0.3f * sinf((float)M_PI * sv) * sinf((float)M_PI * t);

				// Normal via finite differences (computed after all vertices placed)
				// For now store position; we'll compute normals next
				glm::vec2 uv(sv, t);
				AddVertex(pos, uv, glm::vec3(0, 0, 1)); // placeholder normal
			}
		}

		// Compute proper normals via finite differences and fix them up
		// We need to re-evaluate positions for partial derivatives
		auto evalPos = [&](float sv, float t) -> glm::vec3 {
			glm::vec3 bottom = CubicBezier(e0_P0, e0_P1, e0_P2, e0_P3, sv);
			glm::vec3 top = CubicBezier(e2_P0, e2_P1, e2_P2, e2_P3, 1.0f - sv);
			glm::vec3 left = CubicBezier(e3_P0, e3_P1, e3_P2, e3_P3, 1.0f - t);
			glm::vec3 right = CubicBezier(e1_P0, e1_P1, e1_P2, e1_P3, t);
			glm::vec3 bilinear = (1-sv)*(1-t)*aft + sv*(1-t)*lateralB
			                   + (1-sv)*t*lateralA + sv*t*forward;
			glm::vec3 pos = (1-t)*bottom + t*top + (1-sv)*left + sv*right - bilinear;
			pos.z -= 0.3f * sinf((float)M_PI * sv) * sinf((float)M_PI * t);
			return pos;
		};

		// Now emit double-sided triangles using the stored vertices,
		// and also create the back-face copies with flipped normals
		uint backBaseVert = m_vertexCount;

		// Recompute normals and add back-face vertices
		float eps = 1.0f / gridN;
		for (int j = 0; j <= gridN; j++) {
			float t = (float)j / gridN;
			for (int i = 0; i <= gridN; i++) {
				float sv = (float)i / gridN;

				// Finite-difference partial derivatives
				glm::vec3 dPds = evalPos(fminf(sv + eps, 1.0f), t) - evalPos(fmaxf(sv - eps, 0.0f), t);
				glm::vec3 dPdt = evalPos(sv, fminf(t + eps, 1.0f)) - evalPos(sv, fmaxf(t - eps, 0.0f));

				glm::vec3 normal = glm::cross(dPds, dPdt);
				float len = glm::length(normal);
				if (len > 0.0001f) normal /= len;
				else normal = glm::vec3(0, 1, 0);

				// Orient normal away from ship axis
				glm::vec3 pos = evalPos(sv, t);
				glm::vec3 toPos = pos - glm::vec3(0, 0, pos.z);
				if (glm::dot(normal, toPos) < 0.0f)
					normal = -normal;

				// Back-face vertex: same position, flipped normal
				AddVertex(pos, glm::vec2(sv, t), -normal);
			}
		}

		// Front-face triangles (CCW)
		for (int j = 0; j < gridN; j++) {
			for (int i = 0; i < gridN; i++) {
				uint v0 = baseVert + j * (gridN + 1) + i;
				uint v1 = baseVert + (j + 1) * (gridN + 1) + i;
				uint v2 = baseVert + j * (gridN + 1) + (i + 1);
				uint v3 = baseVert + (j + 1) * (gridN + 1) + (i + 1);
				AddTriangle(v0, v2, v1);
				AddTriangle(v2, v3, v1);
			}
		}

		// Back-face triangles (reversed winding)
		for (int j = 0; j < gridN; j++) {
			for (int i = 0; i < gridN; i++) {
				uint v0 = backBaseVert + j * (gridN + 1) + i;
				uint v1 = backBaseVert + (j + 1) * (gridN + 1) + i;
				uint v2 = backBaseVert + j * (gridN + 1) + (i + 1);
				uint v3 = backBaseVert + (j + 1) * (gridN + 1) + (i + 1);
				AddTriangle(v0, v1, v2);
				AddTriangle(v2, v1, v3);
			}
		}

		// --- Spindles: emanate from ~33% along the hull, spread to lateral tips ---
		// t=0.33 → Z = 10*(0.5-0.33) = +1.7, on the hull surface at the sail angle
		float spindleT = 0.33f;
		float spindleZ = 10.0f * (0.5f - spindleT);
		float spindleHullR = 1.5f * HullProfile(spindleT);
		glm::vec3 spindleOrigin(spindleHullR * cx * 1.2f,
		                        spindleHullR * cy * 0.8f, spindleZ);
		CreateCylinder(spindleOrigin, lateralA, spindleRadius, spindleRadius, spindleSlices);
		CreateCylinder(spindleOrigin, lateralB, spindleRadius, spindleRadius, spindleSlices);
	}
}

// ---------------------------------------------------------------------------
// CreateNacelles — 4 tapered engine pods at 35°, 125°, 215°, 305° around the hull.
//
// Each nacelle is a frustum extending radially outward from the hull's aft
// section, tilted slightly backward. Built with CreateCylinder().
// ---------------------------------------------------------------------------
void CShip::CreateNacelles(float maxNacelleRadius, float length, int count)
{
	int slices = 16;
	int stacks = 20;

	// Nacelle angles
	float angles[] = { 35.0f, 125.0f, 215.0f, 305.0f };

	// Hull parameters matching CreateHull call
	float hullLength = 10.0f;
	float maxRadius = 1.5f;
	float widthScale = 1.2f;
	float heightScale = 0.8f;

	// Nacelle center sits at hull t≈0.75 (further aft)
	float ht = 0.75f;
	float centerZ = hullLength * (0.5f - ht);
	float hullR = maxRadius * HullProfile(ht);

	// Teardrop profile for the nacelle: use the hull profile from t=0 (nose)
	// to t=0.5 (peak radius), then cut flat. This gives a pointy front
	// and a flat exhaust at the back.
	float cutT = 0.5f; // cut at peak — flat exhaust end

	for (int n = 0; n < count; n++) {
		float angleRad = angles[n] * (float)M_PI / 180.0f;

		// Nacelle center position: on hull surface
		float xOff = hullR * cosf(angleRad) * widthScale;
		float yOff = hullR * sinf(angleRad) * heightScale;

		// Nacelle runs along Z: nose at front, flat exhaust at back
		float noseZ = centerZ + length * 0.5f;
		float exhaustZ = centerZ - length * 0.5f;

		uint baseVertex = m_vertexCount;

		// Generate vertices: surface of revolution of teardrop profile
		for (int j = 0; j <= stacks; j++) {
			// t goes from 0 (nose) to cutT (exhaust)
			float t = (float)j / stacks * cutT;
			float z = noseZ - (float)j / stacks * length;

			// Nacelle cross-section radius from teardrop profile
			float r = maxNacelleRadius * HullProfile(t) / HullProfile(cutT);
			// Normalize so peak = maxNacelleRadius

			for (int i = 0; i <= slices; i++) {
				float theta = (float)i / slices * 2.0f * (float)M_PI;
				float lx = xOff + r * cosf(theta);
				float ly = yOff + r * sinf(theta);
				glm::vec3 pos(lx, ly, z);

				// Normal: radial direction from nacelle axis
				glm::vec3 normal;
				if (r > 0.001f) {
					normal = glm::normalize(glm::vec3(cosf(theta), sinf(theta), 0.0f));
					// Add longitudinal tilt from profile slope
					float dt = 0.01f;
					float r0 = HullProfile(glm::clamp(t - dt, 0.0f, 1.0f));
					float r1 = HullProfile(glm::clamp(t + dt, 0.0f, 1.0f));
					float slope = (r1 - r0) / (2.0f * dt);
					normal = glm::normalize(normal + glm::vec3(0, 0, slope * 0.5f));
				} else {
					normal = glm::vec3(0, 0, 1); // nose tip points forward
				}

				AddVertex(pos, glm::vec2((float)i / slices, (float)j / stacks), normal);
			}
		}

		// Side triangles
		for (int j = 0; j < stacks; j++) {
			for (int i = 0; i < slices; i++) {
				uint v0 = baseVertex + j * (slices + 1) + i;
				uint v1 = baseVertex + (j + 1) * (slices + 1) + i;
				uint v2 = baseVertex + j * (slices + 1) + (i + 1);
				uint v3 = baseVertex + (j + 1) * (slices + 1) + (i + 1);
				AddTriangle(v0, v1, v2);
				AddTriangle(v2, v1, v3);
			}
		}

		// Flat exhaust cap (disc at the back, normal pointing -Z)
		glm::vec3 exhaustCenter(xOff, yOff, exhaustZ);
		uint centerIdx = AddVertex(exhaustCenter, glm::vec2(0.5f), glm::vec3(0, 0, -1));
		for (int i = 0; i < slices; i++) {
			float theta0 = (float)i / slices * 2.0f * (float)M_PI;
			float theta1 = (float)(i + 1) / slices * 2.0f * (float)M_PI;
			uint ci = AddVertex(
				exhaustCenter + maxNacelleRadius * glm::vec3(cosf(theta0), sinf(theta0), 0),
				glm::vec2(0.5f), glm::vec3(0, 0, -1));
			uint cn = AddVertex(
				exhaustCenter + maxNacelleRadius * glm::vec3(cosf(theta1), sinf(theta1), 0),
				glm::vec2(0.5f), glm::vec3(0, 0, -1));
			AddTriangle(centerIdx, cn, ci);
		}
	}
}

// ---------------------------------------------------------------------------
// CubicBezier — evaluate a cubic Bézier curve at parameter t in [0,1].
// ---------------------------------------------------------------------------
glm::vec3 CShip::CubicBezier(glm::vec3 P0, glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, float t) const
{
	float u = 1.0f - t;
	return u*u*u*P0 + 3.0f*u*u*t*P1 + 3.0f*u*t*t*P2 + t*t*t*P3;
}

// ---------------------------------------------------------------------------
// CreateCylinder — builds a frustum (tapered cylinder) between two 3D points.
//
// Constructs an orthonormal frame (U, V, W) where W = normalised direction
// from 'from' to 'to', then sweeps a circle of linearly interpolated radius
// along the axis. Adds end caps at both ends.
// ---------------------------------------------------------------------------
void CShip::CreateCylinder(glm::vec3 from, glm::vec3 to, float r0, float r1, int slices)
{
	glm::vec3 W = glm::normalize(to - from);

	// Pick an arbitrary vector not parallel to W to build the frame
	glm::vec3 up = (fabsf(glm::dot(W, glm::vec3(0,1,0))) < 0.99f)
		? glm::vec3(0,1,0) : glm::vec3(1,0,0);
	glm::vec3 U = glm::normalize(glm::cross(W, up));
	glm::vec3 V = glm::cross(W, U);

	uint baseFrom = m_vertexCount; // first ring start index
	// Bottom ring (at 'from', radius r0)
	for (int i = 0; i <= slices; i++) {
		float theta = (float)i / slices * 2.0f * (float)M_PI;
		glm::vec3 radial = cosf(theta) * U + sinf(theta) * V;
		glm::vec3 pos = from + r0 * radial;
		glm::vec3 normal = glm::normalize(radial);
		glm::vec2 uv((float)i / slices, 0.0f);
		AddVertex(pos, uv, normal);
	}

	uint baseTo = m_vertexCount; // second ring start index
	// Top ring (at 'to', radius r1)
	for (int i = 0; i <= slices; i++) {
		float theta = (float)i / slices * 2.0f * (float)M_PI;
		glm::vec3 radial = cosf(theta) * U + sinf(theta) * V;
		glm::vec3 pos = to + r1 * radial;
		glm::vec3 normal = glm::normalize(radial);
		glm::vec2 uv((float)i / slices, 1.0f);
		AddVertex(pos, uv, normal);
	}

	// Side triangles
	for (int i = 0; i < slices; i++) {
		uint a0 = baseFrom + i;
		uint a1 = baseFrom + i + 1;
		uint b0 = baseTo + i;
		uint b1 = baseTo + i + 1;
		AddTriangle(a0, b0, a1);
		AddTriangle(a1, b0, b1);
	}

	// End cap at 'from' (normal points along -W)
	uint centerFrom = AddVertex(from, glm::vec2(0.5f), -W);
	for (int i = 0; i < slices; i++) {
		uint ci = AddVertex(from + r0 * (cosf((float)i / slices * 2.0f * (float)M_PI) * U +
			sinf((float)i / slices * 2.0f * (float)M_PI) * V), glm::vec2(0.5f), -W);
		uint cn = AddVertex(from + r0 * (cosf((float)(i+1) / slices * 2.0f * (float)M_PI) * U +
			sinf((float)(i+1) / slices * 2.0f * (float)M_PI) * V), glm::vec2(0.5f), -W);
		AddTriangle(centerFrom, cn, ci);
	}

	// End cap at 'to' (normal points along +W)
	uint centerTo = AddVertex(to, glm::vec2(0.5f), W);
	for (int i = 0; i < slices; i++) {
		uint ci = AddVertex(to + r1 * (cosf((float)i / slices * 2.0f * (float)M_PI) * U +
			sinf((float)i / slices * 2.0f * (float)M_PI) * V), glm::vec2(0.5f), W);
		uint cn = AddVertex(to + r1 * (cosf((float)(i+1) / slices * 2.0f * (float)M_PI) * U +
			sinf((float)(i+1) / slices * 2.0f * (float)M_PI) * V), glm::vec2(0.5f), W);
		AddTriangle(centerTo, ci, cn);
	}
}

// ---------------------------------------------------------------------------
// Create — build the complete ship geometry and upload to the GPU.
// ---------------------------------------------------------------------------
void CShip::Create(string directory, string filename)
{
	// Optionally load a texture
	if (!filename.empty()) {
		m_texture.Load(directory + filename);
		m_texture.SetSamplerObjectParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		m_texture.SetSamplerObjectParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		m_texture.SetSamplerObjectParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
		m_texture.SetSamplerObjectParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
		m_hasTexture = true;
	}

	// Generate and bind the VAO
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	// Create and bind the indexed VBO
	m_vbo.Create();
	m_vbo.Bind();

	// Reset counters
	m_vertexCount = 0;
	m_numTriangles = 0;

	// Build all ship parts into the shared VBO
	CreateHull(10.0f, 1.5f, 24, 40);
	CreateNacelles(0.55f, 3.5f, 4);
	CreateSolarSails(9.0f, 5.0f, 1.5f, 4);

	// Upload everything to the GPU
	m_vbo.UploadDataToGPU(GL_STATIC_DRAW);

	// Set up vertex attribute pointers (stride = 32 bytes per vertex)
	GLsizei stride = 2 * sizeof(glm::vec3) + sizeof(glm::vec2); // 32 bytes

	// Attribute 0: position (vec3, offset 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);

	// Attribute 1: texcoord (vec2, offset 12)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)sizeof(glm::vec3));

	// Attribute 2: normal (vec3, offset 20)
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));
}

// ---------------------------------------------------------------------------
// Render — draw the ship.
//
// All we need to do is:
//   1. Bind the VAO (restores our vertex layout + VBO bindings)
//   2. Optionally bind the texture
//   3. Issue ONE draw call for the entire ship (hull + wings + fin)
//
// The caller (Game::Render) is responsible for setting up the modelview matrix,
// projection matrix, material properties, and bUseTexture uniform beforehand.
// ---------------------------------------------------------------------------
void CShip::Render()
{
	glBindVertexArray(m_vao);

	if (m_hasTexture)
		m_texture.Bind();

	// glDrawElements draws indexed geometry:
	//   - GL_TRIANGLES: every 3 indices form one triangle
	//   - m_numTriangles * 3: total number of indices
	//   - GL_UNSIGNED_INT: index data type
	//   - 0: offset into the index buffer (start from the beginning)
	glDrawElements(GL_TRIANGLES, m_numTriangles * 3, GL_UNSIGNED_INT, 0);
}

// ---------------------------------------------------------------------------
// Release — clean up GPU resources.
//
// OpenGL objects live on the GPU and aren't freed by C++ destructors.
// We must explicitly delete them or we leak VRAM.
// ---------------------------------------------------------------------------
void CShip::Release()
{
	if (m_hasTexture)
		m_texture.Release();

	// Delete the VAO (just the state object, not the data)
	glDeleteVertexArrays(1, &m_vao);

	// Delete the VBO (frees the actual vertex + index data on the GPU)
	m_vbo.Release();
}
