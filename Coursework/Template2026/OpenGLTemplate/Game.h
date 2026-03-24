#pragma once

#include "Common.h"
#include "GameWindow.h"

// Classes used in game.  For a new class, declare it here and provide a pointer to an object of this class below.  Then, in Game.cpp,
// include the header.  In the Game constructor, set the pointer to NULL and in Game::Initialise, create a new object.  Don't forget to
// delete the object in the destructor.
class CCamera;
class CSkybox;
class CShader;
class CShaderProgram;
class CPlane;
class CFreeTypeFont;
class CHighResolutionTimer;
class CSphere;
class COpenAssetImportMesh;
class CShip;
class CAudio;
class CCatmullRom;

class Game {
private:
	// Three main methods used in the game.  Initialise runs once, while Update and Render run repeatedly in the game loop.
	void Initialise();
	void Update();
	void Render();

	// Pointers to game objects.  They will get allocated in Game::Initialise()
	CSkybox *m_pSkybox;
	CCamera *m_pCamera;
	vector <CShaderProgram *> *m_pShaderPrograms;
	CPlane *m_pPlanarTerrain;
	CFreeTypeFont *m_pFtFont;
	COpenAssetImportMesh *m_pBarrelMesh;
	COpenAssetImportMesh *m_pHorseMesh;
	CSphere *m_pSphere;
	CShip *m_pShip;  // Procedural solar sail (Coons patch)
	CCatmullRom *m_pCatmullRom;
	CHighResolutionTimer *m_pHighResolutionTimer;
	CAudio *m_pAudio;

	// Some other member variables
	double m_dt;
	int m_framesPerSecond;
	bool m_appActive;
	float m_currentDistance;
	float m_cameraRoll; // roll angle in degrees, controlled by left/right arrow keys


public:
	Game();
	~Game();
	static Game& GetInstance();
	void Execute();

private:
	static const int FPS = 60;
	void DisplayFrameRate();
	void GameLoop();
	GameWindow& m_gameWindow;
	int m_frameCount;
	double m_elapsedTime;

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};
