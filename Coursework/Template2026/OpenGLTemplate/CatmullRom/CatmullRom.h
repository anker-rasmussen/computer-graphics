#pragma once

#include "Common.h"
#include "Texture.h"

struct PathStartingGuide {
	glm::vec3 position;
	glm::vec3 up;
};

class CCatmullRom
{
public:
	CCatmullRom();
	~CCatmullRom();

	void CreateCentreline();
	void CreateCentrelineEscape();  // long non-looping escape path
	void CreateOffsetCurves();
	void CreateTrack(string directory, string filename);

	void RenderCentreline();
	void RenderOffsetCurves();
	void RenderTrack();

	// Sample the path at a given distance, returning position and up vector
	bool Sample(float d, glm::vec3 &p, glm::vec3 &up);

	// Sample position + full TNB frame at distance d
	bool SampleTNB(float d, glm::vec3 &p, glm::vec3 &T, glm::vec3 &N, glm::vec3 &B);

	void Release();

	float GetTotalLength() const { return m_totalLength; }
	bool IsLoop() const { return m_isLoop; }

private:
	void SetControlPoints();
	void SetControlPointsEscape();

	bool m_isLoop;
	float m_halfWidth; // half-width of the track

	glm::vec3 Interpolate(glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2, glm::vec3 &p3, float t);

	void UniformlySampleControlPoints(int numSamples);

	vector<glm::vec3> m_controlPoints;
	vector<glm::vec3> m_controlUpVectors;

	vector<glm::vec3> m_centrelinePoints;
	vector<glm::vec3> m_centrelineUpVectors;

	vector<glm::vec3> m_leftOffsetPoints;
	vector<glm::vec3> m_rightOffsetPoints;

	float m_totalLength;

	UINT m_vaoCentreline;
	UINT m_vaoLeftOffsetCurve;
	UINT m_vaoRightOffsetCurve;
	UINT m_vaoTrack;

	GLuint m_vboCentreline;
	GLuint m_vboLeftOffsetCurve;
	GLuint m_vboRightOffsetCurve;
	GLuint m_vboTrack;

	CTexture m_texture;

	int m_numCentrelinePoints;
	int m_numTrackPoints;
};
