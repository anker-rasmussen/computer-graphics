#pragma once

#include "Common.h"
#include "GameWindow.h"

// Classes used in game
class CShader;
class CShaderProgram;
class CHighResolutionTimer;

class Game {
private:
	void Initialise();
	void Update();
	void Render();

	float m_spacing;
	
	
	void DrawTriangle(glm::vec3 t);
	void DrawTriangleStack(glm::vec3 s);
	CShaderProgram *m_pShaderProgram;
	CHighResolutionTimer *m_pTimer;
	glm::mat4 *m_pModelMatrix;
	glm::mat4 *m_pViewMatrix;
	glm::mat4 *m_pProjectionMatrix;
	GLuint m_uiVAO;	// A vertex array object (to wrap VBOs)

public:
	Game();
	~Game();
	static Game& GetInstance();
	int Execute();

private:
	void ProcessInput();
	void GameLoop();
	GameWindow gameWindow;

};
