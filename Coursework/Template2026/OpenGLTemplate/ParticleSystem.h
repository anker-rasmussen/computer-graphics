#pragma once

#include "Common.h"
#include "Shaders.h"

struct Particle {
	glm::vec3 position;
	glm::vec3 velocity;
	float life;
	float maxLife;
	float size;
	glm::vec3 colour;
	bool active;
};

class CParticleSystem {
public:
	CParticleSystem();
	~CParticleSystem();

	void Create(CShaderProgram *shader);
	void Spawn(glm::vec3 origin, int count = 40);
	void Spawn(glm::vec3 origin, int count, glm::vec3 colour, float minSize, float maxSize, float minSpeed, float maxSpeed, float minLife, float maxLife);
	void Update(float dt);
	void Render(glm::mat4 viewMatrix, glm::mat4 projMatrix);
	void Release();
	bool HasActiveParticles() const;

private:
	static const int MAX_PARTICLES = 1000;
	Particle m_particles[MAX_PARTICLES];
	GLuint m_vao, m_vbo;
	GLuint m_gradientTex;
	CShaderProgram *m_pShader;

	void GenerateGradientTexture();
};
