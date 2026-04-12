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
class CTexture;
class CShip;
class CAudio;
class CCatmullRom;
class CAnimatedMesh;
class CBridge;  // unused — keeping for now
class CParticleSystem;
class CFrameBufferObject;
class CTacticalGame;

class Game {
private:
	// Three main methods used in the game.  Initialise runs once, while Update and Render run repeatedly in the game loop.
	void Initialise();
	void Update();
	void Render();

	// Pointers to game objects.  They will get allocated in Game::Initialise()
	CSkybox *m_pSkybox;
	CSkybox *m_pSpaceSkybox;  // deep space skybox for exterior shots
	CCamera *m_pCamera;
	vector <CShaderProgram *> *m_pShaderPrograms;
	CPlane *m_pPlanarTerrain;
	CFreeTypeFont *m_pFtFont;
	COpenAssetImportMesh *m_pBarrelMesh;
	COpenAssetImportMesh *m_pHorseMesh;
	CSphere *m_pSphere;
	CShip *m_pShip;  // Full procedural ship (hull + nacelles + sails)
	CCatmullRom *m_pCatmullRom;
	CAnimatedMesh *m_pJean;
	CAnimatedMesh *m_pMieli;
	CAnimatedMesh *m_pChen;
	CBridge *m_pBridge;       // procedural bridge (unused)
	COpenAssetImportMesh *m_pChairMesh;
	COpenAssetImportMesh *m_pMonitorMesh;
	COpenAssetImportMesh *m_pTableMesh;
	COpenAssetImportMesh *m_pCruiserMesh;
	COpenAssetImportMesh *m_pWarmindMesh;
	COpenAssetImportMesh *m_pMissileMesh;
	bool m_cutsceneActive;
	float m_shipCharge;   // 0.0–1.0, stored energy (drives hull neon glow + thrust)
	float m_sailUnfurl;   // 0.0–1.0, how far sails are extended
	int m_shipMode;       // 1 = idle/cruise (sails unfurl, charging), 2 = combat (sails furl, thrust)
	CHighResolutionTimer *m_pHighResolutionTimer;
	CAudio *m_pAudio;

	// Some other member variables
	double m_dt;
	int m_framesPerSecond;
	bool m_appActive;
	float m_currentDistance;
	float m_cameraRoll;


public:
	Game();
	~Game();
	static Game& GetInstance();
	LRESULT ProcessEvents(HWND window, UINT message, WPARAM w_param, LPARAM l_param);
	void SetHinstance(HINSTANCE hinstance);
	WPARAM Execute();

private:
	static const int FPS = 60;
	void DisplayFrameRate();
	void GameLoop();
	GameWindow m_gameWindow;
	HINSTANCE m_hInstance;
	int m_frameCount;
	double m_elapsedTime;

	// Visual novel dialogue
	void RenderDialogue(const string& speaker, const string& text);
	GLuint m_dialogueVAO, m_dialogueVBO, m_whiteTex;
	CTexture *m_portraitJean, *m_portraitMieli, *m_portraitPellegrini, *m_portraitPerhonen;
	CTexture *m_pFloorTex, *m_pWallTex, *m_pViewportTex;
	struct DialogueLine { string speaker; string text; };
	vector<DialogueLine> m_dialogueScript;
	vector<DialogueLine> m_phase4Script;
	vector<DialogueLine> m_chenScript;
	int m_dialogueLine;
	int m_phase4Line;
	int m_chenLine;
	int m_cutscenePhase;    // -1=title, 0-4=cutscene, 5=chen monologue, 6=escape
	float m_cutsceneTimer;  // time elapsed in current phase
	glm::vec3 m_jeanPos, m_mieliPos;  // animated positions
	float m_sobornostApproach; // 0.0–1.0 approach progress
	bool m_shipArrived[4];     // cruiser 0,1,2 + warmind
	float m_screenFlash;       // 0–1, fullscreen white flash intensity
	float m_screenShake;       // 0–1, camera shake intensity
	glm::vec2 m_shakeAngle;        // current shake rotation (pitch, yaw) in radians
	glm::vec2 m_shakeAngleTarget;  // target shake rotation to lerp toward

	// Phase 5: escape gameplay
	float m_escapeTimer;
	float m_lateralOffset;
	float m_shipSpeed;
	bool m_gameOver, m_escaped;
	CParticleSystem *m_pParticleSystem;
	CTacticalGame *m_pTacticalGame;
	CCatmullRom *m_pEscapeSpline;  // separate long spline for escape phase
	void RenderEscapeHUD();

	// Rendering helpers
	void SetMatrices(CShaderProgram *prog, glm::mat4 modelView, glm::mat4 shadowBias, glm::mat4 spotShadowBias);

	// Shadow mapping
	void RenderShadowMap();
	GLuint m_shadowMapFBO, m_shadowMapTex;
	GLuint m_spotShadowFBO, m_spotShadowTex;
	glm::mat4 m_lightViewMatrix, m_lightProjMatrix;
	glm::mat4 m_spotViewMatrix, m_spotProjMatrix;
	static const int SHADOW_MAP_SIZE = 2048;

	// Bloom post-processing for tactical phase
	CFrameBufferObject *m_pBloomSceneFBO;
	CFrameBufferObject *m_pBloomPingFBO;
	CFrameBufferObject *m_pBloomPongFBO;
	GLuint m_bloomQuadVAO;

	// Viewport render-to-texture (bridge front wall live view)
	CFrameBufferObject *m_pViewportFBO;
	void RenderViewportFBO();

	// HUD sprite sheet overlay (phases 3–4)
	CTexture *m_pHudSpriteSheet;
	int m_hudFrameIndex;
	float m_hudFrameTimer;
	float m_hudBrightness;  // 0–1, fades in during phase 3
	static const int HUD_COLS = 12;
	static const int HUD_ROWS = 10;
	static const int HUD_TOTAL_FRAMES = 120;
	static constexpr float HUD_FPS = 15.0f;
	void RenderHudOverlay(float brightness);

	// Title screen
	bool m_titleScreen;
	CTexture *m_pTitleTex;

	// Audio tracking — last dialogue line index that triggered audio per phase
	int m_lastAudioLine0;
	int m_lastAudioLine4;
	int m_lastAudioLine5;

	// Periodic bridge ambience
	float m_bridgeSoundTimer;

	// Escape phase combat
	float m_hitCooldown;
	float m_hitTimer;

	// Free camera toggle (F2)
	bool m_freeCamOverride;
};
