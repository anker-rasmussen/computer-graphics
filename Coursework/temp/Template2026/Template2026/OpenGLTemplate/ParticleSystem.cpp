#include "Common.h"
#include "ParticleSystem.h"
#include <cstdlib>
#include <cmath>

CParticleSystem::CParticleSystem()
	: m_vao(0), m_vbo(0), m_gradientTex(0), m_pShader(NULL)
{
	for (int i = 0; i < MAX_PARTICLES; i++)
		m_particles[i].active = false;
}

CParticleSystem::~CParticleSystem()
{}

void CParticleSystem::GenerateGradientTexture()
{
	const int size = 32;
	unsigned char data[size * size * 4];
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			float u = (x + 0.5f) / size * 2.0f - 1.0f;
			float v = (y + 0.5f) / size * 2.0f - 1.0f;
			float dist = sqrtf(u * u + v * v);
			float alpha = glm::clamp(1.0f - dist, 0.0f, 1.0f);
			alpha *= alpha; // softer falloff
			int idx = (y * size + x) * 4;
			data[idx + 0] = 255;
			data[idx + 1] = 255;
			data[idx + 2] = 255;
			data[idx + 3] = (unsigned char)(alpha * 255.0f);
		}
	}

	glGenTextures(1, &m_gradientTex);
	glBindTexture(GL_TEXTURE_2D, m_gradientTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void CParticleSystem::Create(CShaderProgram *shader)
{
	m_pShader = shader;
	GenerateGradientTexture();

	// Vertex format: vec3 pos + vec2 uv + vec4 colour = 9 floats per vertex, 6 verts per quad
	int maxVerts = MAX_PARTICLES * 6;
	int stride = 9 * sizeof(float);

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, maxVerts * stride, NULL, GL_DYNAMIC_DRAW);

	// position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	// uv
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	// colour+alpha
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));

	glBindVertexArray(0);
}

static float randFloat(float lo, float hi) {
	return lo + (float)rand() / RAND_MAX * (hi - lo);
}

void CParticleSystem::Spawn(glm::vec3 origin, int count)
{
	int spawned = 0;
	for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
		if (m_particles[i].active) continue;

		Particle &p = m_particles[i];
		p.active = true;
		p.position = origin;

		// Random spherical velocity
		float theta = randFloat(0.0f, 2.0f * 3.14159f);
		float phi = randFloat(-3.14159f * 0.5f, 3.14159f * 0.5f);
		float speed = randFloat(5.0f, 18.0f);
		p.velocity = glm::vec3(
			cosf(phi) * cosf(theta),
			cosf(phi) * sinf(theta),
			sinf(phi)
		) * speed;

		p.maxLife = randFloat(0.4f, 1.2f);
		p.life = p.maxLife;
		p.size = randFloat(0.5f, 2.5f);
		p.colour = glm::vec3(1.0f, randFloat(0.3f, 0.7f), 0.1f); // orange-ish

		spawned++;
	}
}

void CParticleSystem::Spawn(glm::vec3 origin, int count, glm::vec3 colour, float minSize, float maxSize, float minSpeed, float maxSpeed, float minLife, float maxLife)
{
	int spawned = 0;
	for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
		if (m_particles[i].active) continue;

		Particle &p = m_particles[i];
		p.active = true;
		p.position = origin;

		float theta = randFloat(0.0f, 2.0f * 3.14159f);
		float phi = randFloat(-3.14159f * 0.5f, 3.14159f * 0.5f);
		float speed = randFloat(minSpeed, maxSpeed);
		p.velocity = glm::vec3(
			cosf(phi) * cosf(theta),
			cosf(phi) * sinf(theta),
			sinf(phi)
		) * speed;

		p.maxLife = randFloat(minLife, maxLife);
		p.life = p.maxLife;
		p.size = randFloat(minSize, maxSize);
		p.colour = colour;

		spawned++;
	}
}

void CParticleSystem::Update(float dt)
{
	for (int i = 0; i < MAX_PARTICLES; i++) {
		if (!m_particles[i].active) continue;

		Particle &p = m_particles[i];
		p.position += p.velocity * dt;
		p.velocity *= 0.96f; // drag
		p.life -= dt;

		if (p.life <= 0.0f)
			p.active = false;
	}
}

bool CParticleSystem::HasActiveParticles() const
{
	for (int i = 0; i < MAX_PARTICLES; i++)
		if (m_particles[i].active) return true;
	return false;
}

void CParticleSystem::Render(glm::mat4 viewMatrix, glm::mat4 projMatrix)
{
	// Extract camera right and up from view matrix for billboarding
	glm::vec3 camRight(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
	glm::vec3 camUp(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

	// Build vertex data for all active particles
	std::vector<float> vertices;
	vertices.reserve(MAX_PARTICLES * 6 * 9);

	int activeCount = 0;
	for (int i = 0; i < MAX_PARTICLES; i++) {
		if (!m_particles[i].active) continue;

		const Particle &p = m_particles[i];
		float alpha = p.life / p.maxLife;
		// Colour lerps from orange to red as it dies
		glm::vec3 col = glm::mix(glm::vec3(0.8f, 0.1f, 0.0f), p.colour, alpha);
		float s = p.size * alpha; // shrink as it dies

		glm::vec3 r = camRight * s;
		glm::vec3 u = camUp * s;

		// Quad corners (world space, billboarded)
		glm::vec3 bl = p.position - r - u;
		glm::vec3 br = p.position + r - u;
		glm::vec3 tr = p.position + r + u;
		glm::vec3 tl = p.position - r + u;

		// Two triangles: bl-br-tr, bl-tr-tl
		auto addVert = [&](glm::vec3 pos, float uvx, float uvy) {
			vertices.push_back(pos.x); vertices.push_back(pos.y); vertices.push_back(pos.z);
			vertices.push_back(uvx); vertices.push_back(uvy);
			vertices.push_back(col.r); vertices.push_back(col.g); vertices.push_back(col.b);
			vertices.push_back(alpha);
		};

		addVert(bl, 0, 0);
		addVert(br, 1, 0);
		addVert(tr, 1, 1);

		addVert(bl, 0, 0);
		addVert(tr, 1, 1);
		addVert(tl, 0, 1);

		activeCount++;
	}

	if (activeCount == 0) return;

	m_pShader->UseProgram();
	m_pShader->SetUniform("projMatrix", projMatrix);
	m_pShader->SetUniform("viewMatrix", viewMatrix);
	m_pShader->SetUniform("sampler0", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_gradientTex);
	glBindSampler(0, 0);

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	glDrawArrays(GL_TRIANGLES, 0, activeCount * 6);

	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	glBindVertexArray(0);
}

void CParticleSystem::Release()
{
	if (m_vao) glDeleteVertexArrays(1, &m_vao);
	if (m_vbo) glDeleteBuffers(1, &m_vbo);
	if (m_gradientTex) glDeleteTextures(1, &m_gradientTex);
	m_vao = m_vbo = m_gradientTex = 0;
}
