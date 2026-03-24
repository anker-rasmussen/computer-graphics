#include "Common.h"
#include "Ship.h"
#include <vector>

CShip::CShip()
	: m_vao(0), m_vboVertices(0), m_vboIndices(0),
	  m_hasTexture(false), m_numTriangles(0), m_vertexCount(0)
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

glm::vec3 CShip::CubicBezier(glm::vec3 P0, glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, float t) const
{
	float u = 1.0f - t;
	return u*u*u*P0 + 3.0f*u*u*t*P1 + 3.0f*u*t*t*P2 + t*t*t*P3;
}

void CShip::CreateSolarSail(float sailHeight, float sailWidth, float curvature)
{
	int gridN = 4;

	float lateralR = sailWidth;
	float sailAngleRad = 90.0f * (float)M_PI / 180.0f;

	float cx = cosf(sailAngleRad);
	float cy = sinf(sailAngleRad);
	float tx = -cy;
	float ty = cx;

	// Diamond corners
	glm::vec3 aft(cx * lateralR, cy * lateralR, 0.0f);
	glm::vec3 forward(0.0f, 0.0f, 13.0f);
	glm::vec3 mid = (aft + forward) * 0.5f;
	float halfSpan = 3.5f;
	glm::vec3 lateralA = mid + glm::vec3(tx * halfSpan, ty * halfSpan, 0.0f);
	glm::vec3 lateralB = mid - glm::vec3(tx * halfSpan, ty * halfSpan, 0.0f);

	// Offset to bow the Bezier edges outward
	glm::vec3 bow = glm::vec3(cx, cy, 0.0f) * curvature;

	// Edge 0: aft -> lateralB
	glm::vec3 e0_P0 = aft, e0_P3 = lateralB;
	glm::vec3 e0_P1 = glm::mix(e0_P0, e0_P3, 1.0f/3.0f) + bow;
	glm::vec3 e0_P2 = glm::mix(e0_P0, e0_P3, 2.0f/3.0f) + bow;

	// Edge 1: lateralB -> forward
	glm::vec3 e1_P0 = lateralB, e1_P3 = forward;
	glm::vec3 e1_P1 = glm::mix(e1_P0, e1_P3, 1.0f/3.0f) + bow;
	glm::vec3 e1_P2 = glm::mix(e1_P0, e1_P3, 2.0f/3.0f) + bow;

	// Edge 2: forward -> lateralA
	glm::vec3 e2_P0 = forward, e2_P3 = lateralA;
	glm::vec3 e2_P1 = glm::mix(e2_P0, e2_P3, 1.0f/3.0f) + bow;
	glm::vec3 e2_P2 = glm::mix(e2_P0, e2_P3, 2.0f/3.0f) + bow;

	// Edge 3: lateralA -> aft
	glm::vec3 e3_P0 = lateralA, e3_P3 = aft;
	glm::vec3 e3_P1 = glm::mix(e3_P0, e3_P3, 1.0f/3.0f) + bow;
	glm::vec3 e3_P2 = glm::mix(e3_P0, e3_P3, 2.0f/3.0f) + bow;

	// Coons patch: blend the four boundary curves and subtract the bilinear corners
	auto evalPos = [&](float sv, float t) -> glm::vec3 {
		glm::vec3 bottom = CubicBezier(e0_P0, e0_P1, e0_P2, e0_P3, sv);
		glm::vec3 top    = CubicBezier(e2_P0, e2_P1, e2_P2, e2_P3, 1.0f - sv);
		glm::vec3 left   = CubicBezier(e3_P0, e3_P1, e3_P2, e3_P3, 1.0f - t);
		glm::vec3 right  = CubicBezier(e1_P0, e1_P1, e1_P2, e1_P3, t);
		glm::vec3 bilinear = (1-sv)*(1-t)*aft + sv*(1-t)*lateralB
		                   + (1-sv)*t*lateralA + sv*t*forward;
		glm::vec3 pos = (1-t)*bottom + t*top + (1-sv)*left + sv*right - bilinear;
		// Push the centre inward to simulate billowing; zero at edges, max at centre
		pos.z -= 0.3f * sinf((float)M_PI * sv) * sinf((float)M_PI * t);
		return pos;
	};

	// Step size for approximating partial derivatives
	float eps = 1.0f / gridN;

	// Compute a reference normal at the centre so all normals point the same way
	glm::vec3 refDs = evalPos(0.5f + eps, 0.5f) - evalPos(0.5f - eps, 0.5f);
	glm::vec3 refDt = evalPos(0.5f, 0.5f + eps) - evalPos(0.5f, 0.5f - eps);
	glm::vec3 refNormal = glm::normalize(glm::cross(refDs, refDt));

	glm::vec3 frontColour(0.85f, 0.85f, 0.9f);
	glm::vec3 backColour(0.75f, 0.75f, 0.82f);

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
}

void CShip::Create(string directory, string filename)
{
	if (!filename.empty()) {
		m_texture.Load(directory + filename);
		m_texture.SetSamplerObjectParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		m_texture.SetSamplerObjectParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		m_texture.SetSamplerObjectParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
		m_texture.SetSamplerObjectParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
		m_hasTexture = true;
	}

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_vboVertices);
	glGenBuffers(1, &m_vboIndices);

	m_vertexCount = 0;
	m_numTriangles = 0;

	CreateSolarSail(9.0f, 5.0f, 1.5f);

	// Upload vertex data
	glBindBuffer(GL_ARRAY_BUFFER, m_vboVertices);
	glBufferData(GL_ARRAY_BUFFER, m_vertexData.size(), m_vertexData.data(), GL_STATIC_DRAW);

	// Upload index data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIndices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexData.size(), m_indexData.data(), GL_STATIC_DRAW);

	// Stride for interleaved layout: position | normal | texcoord | colour
	GLsizei stride = 3 * sizeof(glm::vec3) + sizeof(glm::vec2); // 44 bytes

	// Attribute 0: position (vec3, offset 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

	// Attribute 1: normal (vec3, offset 12)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));

	// Attribute 2: texcoord (vec2, offset 24)
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(glm::vec3)));

	// Attribute 3: colour (vec3, offset 32)
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(glm::vec3) + sizeof(glm::vec2)));
}

void CShip::Render()
{
	glBindVertexArray(m_vao);
	if (m_hasTexture)
		m_texture.Bind();
	glDrawElements(GL_TRIANGLES, m_numTriangles * 3, GL_UNSIGNED_INT, 0);
}

void CShip::Release()
{
	if (m_hasTexture)
		m_texture.Release();
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vboVertices);
	glDeleteBuffers(1, &m_vboIndices);
}
