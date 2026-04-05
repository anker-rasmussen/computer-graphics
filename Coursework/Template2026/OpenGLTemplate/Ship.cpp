#include "Common.h"

#define _USE_MATH_DEFINES

#include "Ship.h"
#include <math.h>
#include <vector>

CShip::CShip()
	: m_vao(0), m_vboVertices(0), m_vboIndices(0),
	  m_hasHullTexture(false), m_hasSailTexture(false),
	  m_numTriangles(0), m_hullTriangles(0), m_thrustTriangles(0),
	  m_vertexCount(0)
{}

CShip::~CShip()
{}

// Interleaved layout: pos(vec3) | normal(vec3) | uv(vec2) | colour(vec3) = 44 bytes/vertex
unsigned int CShip::AddVertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal, glm::vec3 colour)
{
	auto append = [&](const void* ptr, size_t size) {
		const BYTE* p = (const BYTE*)ptr;
		m_vertexData.insert(m_vertexData.end(), p, p + size);
	};
	append(&pos, sizeof(glm::vec3));
	append(&normal, sizeof(glm::vec3));
	append(&uv, sizeof(glm::vec2));
	append(&colour, sizeof(glm::vec3));
	return m_vertexCount++;
}

void CShip::AddTriangle(unsigned int i0, unsigned int i1, unsigned int i2)
{
	auto append = [&](const void* ptr, size_t size) {
		const BYTE* p = (const BYTE*)ptr;
		m_indexData.insert(m_indexData.end(), p, p + size);
	};
	append(&i0, sizeof(unsigned int));
	append(&i1, sizeof(unsigned int));
	append(&i2, sizeof(unsigned int));
	m_numTriangles++;
}

float CShip::HullProfile(float t) const
{
	if (t <= 0.0f || t >= 1.0f) return 0.0f;
	float raw = powf(t, 0.5f) * powf(1.0f - t, 0.8f);
	float peak = powf(5.0f / 13.0f, 0.5f) * powf(8.0f / 13.0f, 0.8f);
	return raw / peak;
}

glm::vec3 CShip::CubicBezier(glm::vec3 P0, glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, float t) const
{
	float u = 1.0f - t;
	return u*u*u*P0 + 3.0f*u*u*t*P1 + 3.0f*u*t*t*P2 + t*t*t*P3;
}

// ---------------------------------------------------------------------------
// Hull — surface of revolution with teardrop cross-section
// ---------------------------------------------------------------------------
void CShip::CreateHull(float hullLength, float maxRadius, int slices, int stacks)
{
	float widthScale = 1.2f;
	float heightScale = 0.8f;
	glm::vec3 hullColour(0.75f, 0.75f, 0.8f);

	unsigned int baseVertex = m_vertexCount;

	for (int j = 0; j <= stacks; j++) {
		float t = (float)j / (float)stacks;
		float z = hullLength * (0.5f - t);
		float r = maxRadius * HullProfile(t);

		for (int i = 0; i <= slices; i++) {
			float theta = (float)i / (float)slices * 2.0f * (float)M_PI;
			float x = r * cosf(theta) * widthScale;
			float y = r * sinf(theta) * heightScale;
			glm::vec3 pos(x, y, z);

			glm::vec3 normal;
			if (r > 0.001f) {
				normal = glm::normalize(glm::vec3(
					cosf(theta) / widthScale,
					sinf(theta) / heightScale,
					0.0f
				));
				float dt = 0.01f;
				float r0 = HullProfile(glm::clamp(t - dt, 0.0f, 1.0f));
				float r1 = HullProfile(glm::clamp(t + dt, 0.0f, 1.0f));
				float slope = (r1 - r0) / (2.0f * dt) * maxRadius;
				float zNorm = slope / hullLength;
				normal = glm::normalize(normal + glm::vec3(0.0f, 0.0f, zNorm));
			} else {
				normal = glm::vec3(0.0f, 0.0f, (t < 0.5f) ? 1.0f : -1.0f);
			}

			glm::vec2 uv((float)i / slices, t);
			AddVertex(pos, uv, normal, hullColour);
		}
	}

	for (int j = 0; j < stacks; j++) {
		for (int i = 0; i < slices; i++) {
			unsigned int v0 = baseVertex + j * (slices + 1) + i;
			unsigned int v1 = baseVertex + (j + 1) * (slices + 1) + i;
			unsigned int v2 = baseVertex + j * (slices + 1) + (i + 1);
			unsigned int v3 = baseVertex + (j + 1) * (slices + 1) + (i + 1);
			AddTriangle(v0, v1, v2);
			AddTriangle(v2, v1, v3);
		}
	}
}

// ---------------------------------------------------------------------------
// Nacelles — 4 tapered engine pods
// ---------------------------------------------------------------------------
void CShip::CreateNacelles(float maxNacelleRadius, float length, int count)
{
	int slices = 16;
	int stacks = 20;
	float angles[] = { 35.0f, 125.0f, 215.0f, 305.0f };

	float hullLength = 10.0f;
	float maxRadius = 1.5f;
	float widthScale = 1.2f;
	float heightScale = 0.8f;

	float ht = 0.75f;
	float centerZ = hullLength * (0.5f - ht);
	float hullR = maxRadius * HullProfile(ht);
	float cutT = 0.5f;

	glm::vec3 nacelleColour(0.6f, 0.62f, 0.65f);
	glm::vec3 exhaustColour(0.3f, 0.35f, 0.5f);

	for (int n = 0; n < count; n++) {
		float angleRad = angles[n] * (float)M_PI / 180.0f;
		float xOff = hullR * cosf(angleRad) * widthScale;
		float yOff = hullR * sinf(angleRad) * heightScale;

		float noseZ = centerZ + length * 0.5f;
		float exhaustZ = centerZ - length * 0.5f;

		unsigned int baseVertex = m_vertexCount;

		for (int j = 0; j <= stacks; j++) {
			float t = (float)j / stacks * cutT;
			float z = noseZ - (float)j / stacks * length;
			float r = maxNacelleRadius * HullProfile(t) / HullProfile(cutT);

			for (int i = 0; i <= slices; i++) {
				float theta = (float)i / slices * 2.0f * (float)M_PI;
				float lx = xOff + r * cosf(theta);
				float ly = yOff + r * sinf(theta);
				glm::vec3 pos(lx, ly, z);

				glm::vec3 normal;
				if (r > 0.001f) {
					normal = glm::normalize(glm::vec3(cosf(theta), sinf(theta), 0.0f));
					float dt = 0.01f;
					float r0 = HullProfile(glm::clamp(t - dt, 0.0f, 1.0f));
					float r1 = HullProfile(glm::clamp(t + dt, 0.0f, 1.0f));
					float slope = (r1 - r0) / (2.0f * dt);
					normal = glm::normalize(normal + glm::vec3(0, 0, slope * 0.5f));
				} else {
					normal = glm::vec3(0, 0, 1);
				}

				AddVertex(pos, glm::vec2((float)i / slices, (float)j / stacks), normal, nacelleColour);
			}
		}

		for (int j = 0; j < stacks; j++) {
			for (int i = 0; i < slices; i++) {
				unsigned int v0 = baseVertex + j * (slices + 1) + i;
				unsigned int v1 = baseVertex + (j + 1) * (slices + 1) + i;
				unsigned int v2 = baseVertex + j * (slices + 1) + (i + 1);
				unsigned int v3 = baseVertex + (j + 1) * (slices + 1) + (i + 1);
				AddTriangle(v0, v1, v2);
				AddTriangle(v2, v1, v3);
			}
		}

		// Flat exhaust cap
		glm::vec3 exhaustCenter(xOff, yOff, exhaustZ);
		unsigned int centerIdx = AddVertex(exhaustCenter, glm::vec2(0.5f), glm::vec3(0, 0, -1), exhaustColour);
		for (int i = 0; i < slices; i++) {
			float theta0 = (float)i / slices * 2.0f * (float)M_PI;
			float theta1 = (float)(i + 1) / slices * 2.0f * (float)M_PI;
			unsigned int ci = AddVertex(
				exhaustCenter + maxNacelleRadius * glm::vec3(cosf(theta0), sinf(theta0), 0),
				glm::vec2(0.5f), glm::vec3(0, 0, -1), exhaustColour);
			unsigned int cn = AddVertex(
				exhaustCenter + maxNacelleRadius * glm::vec3(cosf(theta1), sinf(theta1), 0),
				glm::vec2(0.5f), glm::vec3(0, 0, -1), exhaustColour);
			AddTriangle(centerIdx, cn, ci);
		}
	}
}

// ---------------------------------------------------------------------------
// Ion thrust — bright blue tapered cones extending behind each nacelle exhaust
// ---------------------------------------------------------------------------
void CShip::CreateIonThrust(int count)
{
	float angles[] = { 35.0f, 125.0f, 215.0f, 305.0f };

	float hullLength = 10.0f;
	float maxRadius = 1.5f;
	float widthScale = 1.2f;
	float heightScale = 0.8f;
	float nacelleLength = 3.5f;
	float maxNacelleRadius = 0.55f;

	float ht = 0.75f;
	float centerZ = hullLength * (0.5f - ht);
	float hullR = maxRadius * HullProfile(ht);
	float exhaustZ = centerZ - nacelleLength * 0.5f;

	// Thrust cone parameters
	float thrustLength = 2.5f;
	float thrustBaseR = maxNacelleRadius * 0.7f;
	int slices = 12;
	int stacks = 8;

	// Core is bright cyan, fades to transparent blue at tip
	glm::vec3 coreColour(0.3f, 0.8f, 1.0f);
	glm::vec3 tipColour(0.05f, 0.15f, 0.4f);

	for (int n = 0; n < count; n++) {
		float angleRad = angles[n] * (float)M_PI / 180.0f;
		float xOff = hullR * cosf(angleRad) * widthScale;
		float yOff = hullR * sinf(angleRad) * heightScale;

		glm::vec3 base(xOff, yOff, exhaustZ);
		glm::vec3 tip(xOff, yOff, exhaustZ - thrustLength);

		unsigned int baseVertex = m_vertexCount;

		// Generate tapered cone vertices
		for (int j = 0; j <= stacks; j++) {
			float t = (float)j / stacks;
			float z = exhaustZ - t * thrustLength;
			float r = thrustBaseR * (1.0f - t * t); // quadratic taper for flame shape
			glm::vec3 colour = glm::mix(coreColour, tipColour, t);

			for (int i = 0; i <= slices; i++) {
				float theta = (float)i / slices * 2.0f * (float)M_PI;
				float lx = xOff + r * cosf(theta);
				float ly = yOff + r * sinf(theta);
				glm::vec3 pos(lx, ly, z);

				// Normal points outward from cone axis
				glm::vec3 normal = glm::normalize(glm::vec3(cosf(theta), sinf(theta), 0.3f));

				AddVertex(pos, glm::vec2((float)i / slices, t), normal, colour);
			}
		}

		for (int j = 0; j < stacks; j++) {
			for (int i = 0; i < slices; i++) {
				unsigned int v0 = baseVertex + j * (slices + 1) + i;
				unsigned int v1 = baseVertex + (j + 1) * (slices + 1) + i;
				unsigned int v2 = baseVertex + j * (slices + 1) + (i + 1);
				unsigned int v3 = baseVertex + (j + 1) * (slices + 1) + (i + 1);
				AddTriangle(v0, v1, v2);
				AddTriangle(v2, v1, v3);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Solar Sails — 4 Coons-patch sails with Bezier edges + spindles
// ---------------------------------------------------------------------------
void CShip::CreateSolarSails(float sailHeight, float sailWidth, float curvature, int count)
{
	float sailAngles[] = { 80.0f, 170.0f, 260.0f, 350.0f };

	int gridN = 16;
	float spindleRadius = 0.012f;
	int spindleSlices = 8;

	float hullNoseZ = 7.0f;  // sails attach further forward, ahead of hull nose
	float lateralR = sailWidth;

	glm::vec3 frontColour(0.85f, 0.85f, 0.9f);
	glm::vec3 backColour(0.75f, 0.75f, 0.82f);
	glm::vec3 spindleColour(0.5f, 0.5f, 0.55f);

	for (int s = 0; s < count; s++) {
		float sailAngleRad = sailAngles[s] * (float)M_PI / 180.0f;

		float cx = cosf(sailAngleRad);
		float cy = sinf(sailAngleRad);
		float tx = -cy;
		float ty = cx;

		glm::vec3 aft(cx * lateralR, cy * lateralR, hullNoseZ);
		glm::vec3 forward(0.0f, 0.0f, 20.0f); // well ahead of hull
		glm::vec3 mid = (aft + forward) * 0.5f;
		float halfSpan = 3.5f; // sized to leave gaps between adjacent sails
		glm::vec3 lateralA = mid + glm::vec3(tx * halfSpan, ty * halfSpan, 0.0f);
		glm::vec3 lateralB = mid - glm::vec3(tx * halfSpan, ty * halfSpan, 0.0f);

		glm::vec3 bow = glm::vec3(cx, cy, 0.0f) * curvature * 0.5f; // less rigid edges

		glm::vec3 e0_P0 = aft, e0_P3 = lateralB;
		glm::vec3 e0_P1 = glm::mix(e0_P0, e0_P3, 1.0f/3.0f) + bow;
		glm::vec3 e0_P2 = glm::mix(e0_P0, e0_P3, 2.0f/3.0f) + bow;

		glm::vec3 e1_P0 = lateralB, e1_P3 = forward;
		glm::vec3 e1_P1 = glm::mix(e1_P0, e1_P3, 1.0f/3.0f) + bow;
		glm::vec3 e1_P2 = glm::mix(e1_P0, e1_P3, 2.0f/3.0f) + bow;

		glm::vec3 e2_P0 = forward, e2_P3 = lateralA;
		glm::vec3 e2_P1 = glm::mix(e2_P0, e2_P3, 1.0f/3.0f) + bow;
		glm::vec3 e2_P2 = glm::mix(e2_P0, e2_P3, 2.0f/3.0f) + bow;

		glm::vec3 e3_P0 = lateralA, e3_P3 = aft;
		glm::vec3 e3_P1 = glm::mix(e3_P0, e3_P3, 1.0f/3.0f) + bow;
		glm::vec3 e3_P2 = glm::mix(e3_P0, e3_P3, 2.0f/3.0f) + bow;

		auto evalPos = [&](float sv, float t) -> glm::vec3 {
			glm::vec3 bottom = CubicBezier(e0_P0, e0_P1, e0_P2, e0_P3, sv);
			glm::vec3 top = CubicBezier(e2_P0, e2_P1, e2_P2, e2_P3, 1.0f - sv);
			glm::vec3 left = CubicBezier(e3_P0, e3_P1, e3_P2, e3_P3, 1.0f - t);
			glm::vec3 right = CubicBezier(e1_P0, e1_P1, e1_P2, e1_P3, t);
			glm::vec3 bilinear = (1-sv)*(1-t)*aft + sv*(1-t)*lateralB
			                   + (1-sv)*t*lateralA + sv*t*forward;
			glm::vec3 pos = (1-t)*bottom + t*top + (1-sv)*left + sv*right - bilinear;
			// Billowing: sail belly sags backward (loose cloth catching solar wind)
			float belly = sinf((float)M_PI * sv) * sinf((float)M_PI * t);
			pos.z -= 2.0f * belly;
			// Gravity droop: centre of sail hangs lower, more at edges away from spars
			pos.y -= 0.6f * belly;
			return pos;
		};

		float eps = 1.0f / gridN;

		glm::vec3 refDs = evalPos(0.5f + eps, 0.5f) - evalPos(0.5f - eps, 0.5f);
		glm::vec3 refDt = evalPos(0.5f, 0.5f + eps) - evalPos(0.5f, 0.5f - eps);
		glm::vec3 refNormal = glm::normalize(glm::cross(refDs, refDt));

		// Front-face vertices
		unsigned int baseVert = m_vertexCount;
		for (int j = 0; j <= gridN; j++) {
			float t = (float)j / gridN;
			for (int i = 0; i <= gridN; i++) {
				float sv = (float)i / gridN;

				glm::vec3 dPds = evalPos(fminf(sv + eps, 1.0f), t) - evalPos(fmaxf(sv - eps, 0.0f), t);
				glm::vec3 dPdt = evalPos(sv, fminf(t + eps, 1.0f)) - evalPos(sv, fmaxf(t - eps, 0.0f));

				glm::vec3 normal = glm::cross(dPds, dPdt);
				float len = glm::length(normal);
				if (len > 0.0001f) normal /= len;
				else normal = glm::vec3(0, 1, 0);

				if (glm::dot(normal, refNormal) < 0.0f)
					normal = -normal;

				AddVertex(evalPos(sv, t), glm::vec2(sv, t), normal, frontColour);
			}
		}

		// Back-face vertices (flipped normals)
		unsigned int backBaseVert = m_vertexCount;
		for (int j = 0; j <= gridN; j++) {
			float t = (float)j / gridN;
			for (int i = 0; i <= gridN; i++) {
				float sv = (float)i / gridN;

				glm::vec3 dPds = evalPos(fminf(sv + eps, 1.0f), t) - evalPos(fmaxf(sv - eps, 0.0f), t);
				glm::vec3 dPdt = evalPos(sv, fminf(t + eps, 1.0f)) - evalPos(sv, fmaxf(t - eps, 0.0f));

				glm::vec3 normal = glm::cross(dPds, dPdt);
				float len = glm::length(normal);
				if (len > 0.0001f) normal /= len;
				else normal = glm::vec3(0, 1, 0);

				if (glm::dot(normal, refNormal) < 0.0f)
					normal = -normal;

				AddVertex(evalPos(sv, t), glm::vec2(sv, t), -normal, backColour);
			}
		}

		// Front-face triangles (CCW)
		for (int j = 0; j < gridN; j++) {
			for (int i = 0; i < gridN; i++) {
				unsigned int v0 = baseVert + j * (gridN + 1) + i;
				unsigned int v1 = baseVert + (j + 1) * (gridN + 1) + i;
				unsigned int v2 = baseVert + j * (gridN + 1) + (i + 1);
				unsigned int v3 = baseVert + (j + 1) * (gridN + 1) + (i + 1);
				AddTriangle(v0, v2, v1);
				AddTriangle(v2, v3, v1);
			}
		}

		// Back-face triangles (reversed winding)
		for (int j = 0; j < gridN; j++) {
			for (int i = 0; i < gridN; i++) {
				unsigned int v0 = backBaseVert + j * (gridN + 1) + i;
				unsigned int v1 = backBaseVert + (j + 1) * (gridN + 1) + i;
				unsigned int v2 = backBaseVert + j * (gridN + 1) + (i + 1);
				unsigned int v3 = backBaseVert + (j + 1) * (gridN + 1) + (i + 1);
				AddTriangle(v0, v1, v2);
				AddTriangle(v2, v1, v3);
			}
		}

		// Spindles connecting hull to sail lateral tips
		float spindleT = 0.5f; // attach from midpoint of hull for longer, looser spindles
		float spindleZ = 10.0f * (0.5f - spindleT);
		float spindleHullR = 1.5f * HullProfile(spindleT);
		glm::vec3 spindleOrigin(spindleHullR * cx * 1.2f,
		                        spindleHullR * cy * 0.8f, spindleZ);
		CreateCylinder(spindleOrigin, lateralA, spindleRadius, spindleRadius, spindleSlices, spindleColour);
		CreateCylinder(spindleOrigin, lateralB, spindleRadius, spindleRadius, spindleSlices, spindleColour);
	}
}

// ---------------------------------------------------------------------------
// CreateCylinder — frustum between two points with end caps
// ---------------------------------------------------------------------------
void CShip::CreateCylinder(glm::vec3 from, glm::vec3 to, float r0, float r1, int slices, glm::vec3 colour)
{
	glm::vec3 W = glm::normalize(to - from);
	glm::vec3 up = (fabsf(glm::dot(W, glm::vec3(0,1,0))) < 0.99f)
		? glm::vec3(0,1,0) : glm::vec3(1,0,0);
	glm::vec3 U = glm::normalize(glm::cross(W, up));
	glm::vec3 V = glm::cross(W, U);

	unsigned int baseFrom = m_vertexCount;
	for (int i = 0; i <= slices; i++) {
		float theta = (float)i / slices * 2.0f * (float)M_PI;
		glm::vec3 radial = cosf(theta) * U + sinf(theta) * V;
		AddVertex(from + r0 * radial, glm::vec2((float)i / slices, 0.0f), glm::normalize(radial), colour);
	}

	unsigned int baseTo = m_vertexCount;
	for (int i = 0; i <= slices; i++) {
		float theta = (float)i / slices * 2.0f * (float)M_PI;
		glm::vec3 radial = cosf(theta) * U + sinf(theta) * V;
		AddVertex(to + r1 * radial, glm::vec2((float)i / slices, 1.0f), glm::normalize(radial), colour);
	}

	for (int i = 0; i < slices; i++) {
		AddTriangle(baseFrom + i, baseTo + i, baseFrom + i + 1);
		AddTriangle(baseFrom + i + 1, baseTo + i, baseTo + i + 1);
	}

	unsigned int centerFrom = AddVertex(from, glm::vec2(0.5f), -W, colour);
	for (int i = 0; i < slices; i++) {
		float t0 = (float)i / slices * 2.0f * (float)M_PI;
		float t1 = (float)(i+1) / slices * 2.0f * (float)M_PI;
		unsigned int ci = AddVertex(from + r0 * (cosf(t0) * U + sinf(t0) * V), glm::vec2(0.5f), -W, colour);
		unsigned int cn = AddVertex(from + r0 * (cosf(t1) * U + sinf(t1) * V), glm::vec2(0.5f), -W, colour);
		AddTriangle(centerFrom, cn, ci);
	}

	unsigned int centerTo = AddVertex(to, glm::vec2(0.5f), W, colour);
	for (int i = 0; i < slices; i++) {
		float t0 = (float)i / slices * 2.0f * (float)M_PI;
		float t1 = (float)(i+1) / slices * 2.0f * (float)M_PI;
		unsigned int ci = AddVertex(to + r1 * (cosf(t0) * U + sinf(t0) * V), glm::vec2(0.5f), W, colour);
		unsigned int cn = AddVertex(to + r1 * (cosf(t1) * U + sinf(t1) * V), glm::vec2(0.5f), W, colour);
		AddTriangle(centerTo, ci, cn);
	}
}

// ---------------------------------------------------------------------------
// Create — build all geometry and upload to GPU
// ---------------------------------------------------------------------------
void CShip::Create(string hullTexDir, string hullTexFile,
                   string sailTexDir, string sailTexFile)
{
	// Load hull texture (neon circuit board)
	if (!hullTexFile.empty()) {
		m_hullTexture.Load(hullTexDir + hullTexFile);
		m_hullTexture.SetSamplerObjectParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		m_hullTexture.SetSamplerObjectParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		m_hullTexture.SetSamplerObjectParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
		m_hullTexture.SetSamplerObjectParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
		m_hasHullTexture = true;
	}

	// Load sail texture (iridescent)
	if (!sailTexFile.empty()) {
		m_sailTexture.Load(sailTexDir + sailTexFile);
		m_sailTexture.SetSamplerObjectParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		m_sailTexture.SetSamplerObjectParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		m_sailTexture.SetSamplerObjectParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
		m_sailTexture.SetSamplerObjectParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
		m_hasSailTexture = true;
	}

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	glGenBuffers(1, &m_vboVertices);
	glGenBuffers(1, &m_vboIndices);

	m_vertexCount = 0;
	m_numTriangles = 0;

	// Part 1: hull + nacelles
	CreateHull(10.0f, 1.5f, 24, 40);
	CreateNacelles(0.55f, 3.5f, 4);
	m_hullTriangles = m_numTriangles;

	// Part 2: ion thrust
	CreateIonThrust(4);
	m_thrustTriangles = m_numTriangles - m_hullTriangles;

	// Part 3: solar sails
	CreateSolarSails(9.0f, 8.0f, 1.5f, 4);

	// Upload
	glBindBuffer(GL_ARRAY_BUFFER, m_vboVertices);
	glBufferData(GL_ARRAY_BUFFER, m_vertexData.size(), m_vertexData.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIndices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexData.size(), m_indexData.data(), GL_STATIC_DRAW);

	GLsizei stride = 3 * sizeof(glm::vec3) + sizeof(glm::vec2); // 44 bytes

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(glm::vec3)));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(glm::vec3) + sizeof(glm::vec2)));
}

void CShip::RenderHull()
{
	glBindVertexArray(m_vao);
	if (m_hasHullTexture)
		m_hullTexture.Bind();
	glDrawElements(GL_TRIANGLES, m_hullTriangles * 3, GL_UNSIGNED_INT, 0);
}

void CShip::RenderThrust()
{
	glBindVertexArray(m_vao);
	// Thrust starts right after hull indices
	GLvoid* offset = (GLvoid*)(m_hullTriangles * 3 * sizeof(unsigned int));
	glDrawElements(GL_TRIANGLES, m_thrustTriangles * 3, GL_UNSIGNED_INT, offset);
}

void CShip::RenderSails()
{
	glBindVertexArray(m_vao);
	if (m_hasSailTexture)
		m_sailTexture.Bind();
	int sailStart = (m_hullTriangles + m_thrustTriangles) * 3;
	int sailCount = (m_numTriangles - m_hullTriangles - m_thrustTriangles) * 3;
	GLvoid* offset = (GLvoid*)(sailStart * sizeof(unsigned int));
	glDrawElements(GL_TRIANGLES, sailCount, GL_UNSIGNED_INT, offset);
}

void CShip::Release()
{
	if (m_hasHullTexture)
		m_hullTexture.Release();
	if (m_hasSailTexture)
		m_sailTexture.Release();
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vboVertices);
	glDeleteBuffers(1, &m_vboIndices);
}
