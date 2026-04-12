#include "Common.h"
#include "CatmullRom.h"
#include <cmath>

CCatmullRom::CCatmullRom()
	: m_vaoCentreline(0), m_vaoLeftOffsetCurve(0), m_vaoRightOffsetCurve(0),
	  m_vaoTrack(0), m_vboCentreline(0), m_vboLeftOffsetCurve(0),
	  m_vboRightOffsetCurve(0), m_vboTrack(0),
	  m_totalLength(0.0f), m_numCentrelinePoints(0), m_numTrackPoints(0),
	  m_isLoop(true), m_halfWidth(5.0f)
{}

CCatmullRom::~CCatmullRom()
{}

glm::vec3 CCatmullRom::Interpolate(glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2, glm::vec3 &p3, float t)
{
	float t2 = t * t;
	float t3 = t2 * t;

	glm::vec3 a = -p0 + 3.0f * p1 - 3.0f * p2 + p3;
	glm::vec3 b = 2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3;
	glm::vec3 c = -p0 + p2;
	glm::vec3 d = 2.0f * p1;

	return 0.5f * (a * t3 + b * t2 + c * t + d);
}

void CCatmullRom::SetControlPoints()
{
	// Nonlinear space track with S-curves, elevation changes, and varied radius
	// Forms a large twisted loop through space
	auto addPt = [&](float x, float y, float z) {
		m_controlPoints.push_back(glm::vec3(x, y, z));
		m_controlUpVectors.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	};

	// Start: straight run out from the ship area
	addPt(   0.0f,  40.0f,  150.0f);
	addPt(  60.0f,  45.0f,  250.0f);
	// Climbing right turn
	addPt( 150.0f,  70.0f,  300.0f);
	addPt( 200.0f,  90.0f,  250.0f);
	// Descending S-curve
	addPt( 180.0f,  60.0f,  150.0f);
	addPt( 100.0f,  30.0f,   50.0f);
	// Sharp left, diving low
	addPt(  20.0f,  15.0f,  -50.0f);
	addPt( -80.0f,  20.0f, -120.0f);
	// Sweeping left arc at depth
	addPt(-180.0f,  35.0f, -100.0f);
	addPt(-220.0f,  55.0f,  -20.0f);
	// Climbing back up, wide right
	addPt(-200.0f,  80.0f,   80.0f);
	addPt(-140.0f,  95.0f,  160.0f);
	// Crest and descent back toward start
	addPt( -60.0f,  85.0f,  200.0f);
	addPt( -20.0f,  60.0f,  180.0f);
}

void CCatmullRom::SetControlPointsEscape()
{
	// Long non-looping escape path through an asteroid field
	// ~150 control points, ~12,000 arc-length units total
	auto addPt = [&](float x, float y, float z) {
		m_controlPoints.push_back(glm::vec3(x, y, z));
		m_controlUpVectors.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	};

	float z = 150.0f;
	float x = 0.0f;
	float y = 40.0f;

	for (int i = 0; i < 150; i++) {
		addPt(x, y, z);
		z += 60.0f + 40.0f * sinf(i * 0.3f);
		x += 50.0f * sinf(i * 0.17f);
		y = 40.0f + 30.0f * sinf(i * 0.11f);
	}

	// Duplicate first and last points as padding for Catmull-Rom endpoint interpolation
	glm::vec3 first = m_controlPoints.front();
	glm::vec3 last = m_controlPoints.back();
	glm::vec3 firstDir = m_controlPoints[1] - first;
	glm::vec3 lastDir = last - m_controlPoints[m_controlPoints.size() - 2];
	m_controlPoints.insert(m_controlPoints.begin(), first - firstDir);
	m_controlUpVectors.insert(m_controlUpVectors.begin(), glm::vec3(0, 1, 0));
	m_controlPoints.push_back(last + lastDir);
	m_controlUpVectors.push_back(glm::vec3(0, 1, 0));
}

void CCatmullRom::UniformlySampleControlPoints(int numSamples)
{
	int n = (int)m_controlPoints.size();
	vector<glm::vec3> rawPoints;
	vector<float> distances;

	int samplesPerSegment = 100;
	float totalDist = 0.0f;
	glm::vec3 prev = m_controlPoints[0];
	rawPoints.push_back(prev);
	distances.push_back(0.0f);

	int numSegments = m_isLoop ? n : n - 1;
	for (int seg = 0; seg < numSegments; seg++) {
		glm::vec3 p0, p1, p2, p3;
		if (m_isLoop) {
			p0 = m_controlPoints[(seg - 1 + n) % n];
			p1 = m_controlPoints[seg];
			p2 = m_controlPoints[(seg + 1) % n];
			p3 = m_controlPoints[(seg + 2) % n];
		} else {
			p0 = m_controlPoints[glm::clamp(seg - 1, 0, n - 1)];
			p1 = m_controlPoints[seg];
			p2 = m_controlPoints[glm::clamp(seg + 1, 0, n - 1)];
			p3 = m_controlPoints[glm::clamp(seg + 2, 0, n - 1)];
		}

		for (int j = 1; j <= samplesPerSegment; j++) {
			float t = (float)j / samplesPerSegment;
			glm::vec3 pt = Interpolate(p0, p1, p2, p3, t);
			totalDist += glm::length(pt - prev);
			rawPoints.push_back(pt);
			distances.push_back(totalDist);
			prev = pt;
		}
	}

	m_totalLength = totalDist;

	// Resample at uniform arc-length spacing
	float spacing = totalDist / numSamples;
	int rawIdx = 0;
	m_centrelinePoints.clear();
	m_centrelineUpVectors.clear();

	for (int i = 0; i < numSamples; i++) {
		float targetDist = i * spacing;

		while (rawIdx < (int)distances.size() - 2 && distances[rawIdx + 1] < targetDist)
			rawIdx++;

		float segLen = distances[rawIdx + 1] - distances[rawIdx];
		float frac = 0.0f;
		if (segLen > 0.0001f)
			frac = (targetDist - distances[rawIdx]) / segLen;

		glm::vec3 pt = glm::mix(rawPoints[rawIdx], rawPoints[rawIdx + 1], frac);
		m_centrelinePoints.push_back(pt);
		m_centrelineUpVectors.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	}
}

void CCatmullRom::CreateCentreline()
{
	SetControlPoints();
	UniformlySampleControlPoints(500);

	m_numCentrelinePoints = (int)m_centrelinePoints.size();

	glGenVertexArrays(1, &m_vaoCentreline);
	glBindVertexArray(m_vaoCentreline);

	// Stage vertex data: pos(vec3) + uv(vec2) + normal(vec3)
	std::vector<BYTE> data;
	glm::vec2 dummyUV(0.0f, 0.0f);
	for (int i = 0; i < m_numCentrelinePoints; i++) {
		auto append = [&](const void* ptr, size_t sz) {
			const BYTE* p = (const BYTE*)ptr;
			data.insert(data.end(), p, p + sz);
		};
		append(&m_centrelinePoints[i], sizeof(glm::vec3));
		append(&dummyUV, sizeof(glm::vec2));
		append(&m_centrelineUpVectors[i], sizeof(glm::vec3));
	}

	glGenBuffers(1, &m_vboCentreline);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboCentreline);
	glBufferData(GL_ARRAY_BUFFER, data.size(), data.data(), GL_STATIC_DRAW);

	GLsizei stride = 2 * sizeof(glm::vec3) + sizeof(glm::vec2);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)sizeof(glm::vec3));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));
}

void CCatmullRom::CreateCentrelineEscape()
{
	m_isLoop = false;
	m_halfWidth = 100.0f; // 10 lanes of 20 units each
	SetControlPointsEscape();
	UniformlySampleControlPoints(5000);

	m_numCentrelinePoints = (int)m_centrelinePoints.size();

	glGenVertexArrays(1, &m_vaoCentreline);
	glBindVertexArray(m_vaoCentreline);

	std::vector<BYTE> data;
	glm::vec2 dummyUV(0.0f, 0.0f);
	for (int i = 0; i < m_numCentrelinePoints; i++) {
		auto append = [&](const void* ptr, size_t sz) {
			const BYTE* p = (const BYTE*)ptr;
			data.insert(data.end(), p, p + sz);
		};
		append(&m_centrelinePoints[i], sizeof(glm::vec3));
		append(&dummyUV, sizeof(glm::vec2));
		append(&m_centrelineUpVectors[i], sizeof(glm::vec3));
	}

	glGenBuffers(1, &m_vboCentreline);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboCentreline);
	glBufferData(GL_ARRAY_BUFFER, data.size(), data.data(), GL_STATIC_DRAW);

	GLsizei stride = 2 * sizeof(glm::vec3) + sizeof(glm::vec2);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)sizeof(glm::vec3));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));
}

void CCatmullRom::CreateOffsetCurves()
{
	float halfWidth = m_halfWidth;

	m_leftOffsetPoints.clear();
	m_rightOffsetPoints.clear();

	int n = m_numCentrelinePoints;
	for (int i = 0; i < n; i++) {
		int prev, next;
		if (m_isLoop) {
			prev = (i - 1 + n) % n;
			next = (i + 1) % n;
		} else {
			prev = glm::max(i - 1, 0);
			next = glm::min(i + 1, n - 1);
		}

		glm::vec3 T = glm::normalize(m_centrelinePoints[next] - m_centrelinePoints[prev]);
		glm::vec3 N = glm::normalize(glm::cross(T, m_centrelineUpVectors[i]));

		// Track sits 2 units below the camera path
		glm::vec3 trackPt = m_centrelinePoints[i];
		trackPt.y -= 2.0f;
		m_leftOffsetPoints.push_back(trackPt + N * halfWidth);
		m_rightOffsetPoints.push_back(trackPt - N * halfWidth);
	}

	GLsizei stride = 2 * sizeof(glm::vec3) + sizeof(glm::vec2);
	glm::vec2 dummyUV(0.0f, 0.0f);
	glm::vec3 upNorm(0.0f, 1.0f, 0.0f);

	auto buildVBO = [&](UINT &vao, GLuint &vbo, vector<glm::vec3> &points) {
		std::vector<BYTE> data;
		auto append = [&](const void* ptr, size_t sz) {
			const BYTE* p = (const BYTE*)ptr;
			data.insert(data.end(), p, p + sz);
		};

		for (int i = 0; i < n; i++) {
			append(&points[i], sizeof(glm::vec3));
			append(&dummyUV, sizeof(glm::vec2));
			append(&upNorm, sizeof(glm::vec3));
		}

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size(), data.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)sizeof(glm::vec3));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));
	};

	buildVBO(m_vaoLeftOffsetCurve, m_vboLeftOffsetCurve, m_leftOffsetPoints);
	buildVBO(m_vaoRightOffsetCurve, m_vboRightOffsetCurve, m_rightOffsetPoints);
}

void CCatmullRom::CreateTrack(string directory, string filename)
{
	m_texture.Load(directory + filename);
	m_texture.SetSamplerObjectParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	m_texture.SetSamplerObjectParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_texture.SetSamplerObjectParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
	m_texture.SetSamplerObjectParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);

	int n = m_numCentrelinePoints;
	int trackVerts = m_isLoop ? n + 1 : n;
	m_numTrackPoints = trackVerts * 2;

	glGenVertexArrays(1, &m_vaoTrack);
	glBindVertexArray(m_vaoTrack);

	std::vector<BYTE> data;
	auto append = [&](const void* ptr, size_t sz) {
		const BYTE* p = (const BYTE*)ptr;
		data.insert(data.end(), p, p + sz);
	};

	glm::vec3 upNorm(0.0f, 1.0f, 0.0f);

	for (int i = 0; i < trackVerts; i++) {
		int idx = m_isLoop ? (i % n) : i;
		float v = (float)i / n * 10.0f;

		glm::vec2 uvLeft(0.0f, v);
		glm::vec2 uvRight(1.0f, v);

		// Right then left -- CCW winding from above
		append(&m_rightOffsetPoints[idx], sizeof(glm::vec3));
		append(&uvRight, sizeof(glm::vec2));
		append(&upNorm, sizeof(glm::vec3));

		append(&m_leftOffsetPoints[idx], sizeof(glm::vec3));
		append(&uvLeft, sizeof(glm::vec2));
		append(&upNorm, sizeof(glm::vec3));
	}

	glGenBuffers(1, &m_vboTrack);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboTrack);
	glBufferData(GL_ARRAY_BUFFER, data.size(), data.data(), GL_STATIC_DRAW);

	GLsizei stride = 2 * sizeof(glm::vec3) + sizeof(glm::vec2);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)sizeof(glm::vec3));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));
}

bool CCatmullRom::Sample(float d, glm::vec3 &p, glm::vec3 &up)
{
	if (m_centrelinePoints.empty())
		return false;

	float dist;
	if (m_isLoop) {
		dist = fmod(d, m_totalLength);
		if (dist < 0.0f) dist += m_totalLength;
	} else {
		dist = glm::clamp(d, 0.0f, m_totalLength - 0.01f);
	}

	float spacing = m_totalLength / m_numCentrelinePoints;
	float idx_f = dist / spacing;
	int idx = (int)idx_f;
	float frac = idx_f - idx;

	int i0, i1;
	if (m_isLoop) {
		i0 = idx % m_numCentrelinePoints;
		i1 = (idx + 1) % m_numCentrelinePoints;
	} else {
		i0 = glm::clamp(idx, 0, m_numCentrelinePoints - 1);
		i1 = glm::clamp(idx + 1, 0, m_numCentrelinePoints - 1);
	}

	p = glm::mix(m_centrelinePoints[i0], m_centrelinePoints[i1], frac);
	up = glm::vec3(0.0f, 1.0f, 0.0f);

	return true;
}

bool CCatmullRom::SampleTNB(float d, glm::vec3 &p, glm::vec3 &T, glm::vec3 &N, glm::vec3 &B)
{
	glm::vec3 up;
	if (!Sample(d, p, up))
		return false;

	// Get a point slightly ahead for tangent direction
	glm::vec3 pAhead, upAhead;
	Sample(d + 0.5f, pAhead, upAhead);

	T = glm::normalize(pAhead - p);
	N = glm::normalize(glm::cross(T, glm::vec3(0.0f, 1.0f, 0.0f)));
	B = glm::normalize(glm::cross(N, T));
	return true;
}

void CCatmullRom::RenderCentreline()
{
	glBindVertexArray(m_vaoCentreline);
	glDrawArrays(GL_LINE_STRIP, 0, m_numCentrelinePoints);
}

void CCatmullRom::RenderOffsetCurves()
{
	glBindVertexArray(m_vaoLeftOffsetCurve);
	glDrawArrays(GL_LINE_STRIP, 0, m_numCentrelinePoints);
	glBindVertexArray(m_vaoRightOffsetCurve);
	glDrawArrays(GL_LINE_STRIP, 0, m_numCentrelinePoints);
}

void CCatmullRom::RenderTrack()
{
	glBindVertexArray(m_vaoTrack);
	m_texture.Bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, m_numTrackPoints);
}

void CCatmullRom::Release()
{
	m_texture.Release();
	glDeleteVertexArrays(1, &m_vaoCentreline);
	glDeleteVertexArrays(1, &m_vaoLeftOffsetCurve);
	glDeleteVertexArrays(1, &m_vaoRightOffsetCurve);
	glDeleteVertexArrays(1, &m_vaoTrack);
	glDeleteBuffers(1, &m_vboCentreline);
	glDeleteBuffers(1, &m_vboLeftOffsetCurve);
	glDeleteBuffers(1, &m_vboRightOffsetCurve);
	glDeleteBuffers(1, &m_vboTrack);
}
