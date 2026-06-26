/*
OpenGL Template for INM376 / IN3005
City University London, School of Mathematics, Computer Science and Engineering
Source code drawn from a number of sources and examples, including contributions from
 - Ben Humphrey (gametutorials.com), Michal Bubner (mbsoftworks.sk), Christophe Riccio (glm.g-truc.net)
 - Christy Quinn, Sam Kellett and others

 For educational use by Department of Computer Science, City University London UK.

 This template contains a skybox, simple terrain, camera, lighting, shaders, texturing

 Potential ways to modify the code:  Add new geometry types, shaders, change the terrain, load new meshes, change the lighting,
 different camera controls, different shaders, etc.

 Template version 5.0a 29/01/2017
 Dr Greg Slabaugh (gregory.slabaugh.1@city.ac.uk)

 version 6.0a 29/01/2019
 Dr Eddie Edwards (Philip.Edwards@city.ac.uk)

 version 6.1a 13/02/2022 - Sorted out Release mode and a few small compiler warnings
 Dr Eddie Edwards (Philip.Edwards@city.ac.uk)

*/


#include "Game.h"


// Setup includes
#include "HighResolutionTimer.h"
#include "GameWindow.h"

// Game includes
#include "Camera.h"
#include "Skybox.h"
#include "Plane.h"
#include "Shaders.h"
#include "FreeTypeFont.h"
#include "Ship.h"
#include "Sphere.h"
#include "MatrixStack.h"
#include "OpenAssetImportMesh.h"
#include "Audio.h"
#include "CatmullRom.h"
#include "AnimatedMesh.h"
#include "Bridge.h"
#include "Texture.h"
#include "ParticleSystem.h"
#include "TacticalGame.h"
#include "Asteroid.h"
#include "FrameBufferObject.h"

// Constructor
Game::Game()
	: m_gameWindow(GameWindow::GetInstance())
{
	m_pSkybox = NULL;
	m_pSpaceSkybox = NULL;
	m_pCamera = NULL;
	m_pShaderPrograms = NULL;
	m_pPlanarTerrain = NULL;
	m_pFtFont = NULL;
	m_pBarrelMesh = NULL;
	m_pHorseMesh = NULL;
	m_pSphere = NULL;
	m_pShip = NULL;
	m_pCatmullRom = NULL;
	m_pJean = NULL;
	m_pMieli = NULL;
	m_pChen = NULL;
	m_pBridge = NULL;
	m_pChairMesh = NULL;
	m_pMonitorMesh = NULL;
	m_pTableMesh = NULL;
	m_pCruiserMesh = NULL;
	m_pWarmindMesh = NULL;
	m_pMissileMesh = NULL;
	m_pParticleSystem = NULL;
	m_pTacticalGame = NULL;
	m_pEscapeSpline = NULL;
	m_pBloomSceneFBO = NULL;
	m_pBloomPingFBO = NULL;
	m_pBloomPongFBO = NULL;
	m_bloomQuadVAO = 0;
	m_pViewportFBO = NULL;
	m_circleVAO = 0;
	m_circleVBO = 0;
	m_circleSegments = 64;
	m_escapeTimer = 60.0f;
	m_lateralOffset = 0.0f;
	m_shipSpeed = 30.0f;
	m_gameOver = false;
	m_escaped = false;
	m_cutsceneActive = true; // start in cutscene for testing
	m_dialogueVAO = 0;
	m_dialogueVBO = 0;
	m_whiteTex = 0;
	m_portraitJean = NULL;
	m_portraitMieli = NULL;
	m_portraitPellegrini = NULL;
	m_portraitPerhonen = NULL;
	m_portraitChen = NULL;
	m_pFloorTex = NULL;
	m_pWallTex = NULL;
	m_pViewportTex = NULL;
	m_shadowMapFBO = 0;
	m_shadowMapTex = 0;
	m_spotShadowFBO = 0;
	m_spotShadowTex = 0;
	m_pHudSpriteSheet = NULL;
	m_hudFrameIndex = 0;
	m_hudFrameTimer = 0.0f;
	m_hudBrightness = 0.0f;
	m_dialogueLine = 0;
	m_cutscenePhase = -1;  // start on title screen
	m_titleScreen = true;
	m_pTitleTex = NULL;
	m_lastAudioLine0 = -1;
	m_lastAudioLine4 = -1;
	m_lastAudioLine5 = -1;
	m_bridgeSoundTimer = 10.0f;
	m_hitCooldown = 0.0f;
	m_hitTimer = 0.0f;
	m_cutsceneTimer = 0.0f;
	m_jeanPos = glm::vec3(-1.0f, 0.0f, -2.2f);
	m_mieliPos = glm::vec3(1.0f, 0.0f, -2.2f);
	m_dialogueScript = {
		{"Pellegrini", "We are tracking an unidentified object. One of the Engineer's chens, closing fast."},  // voice: tracking
		{"Jean",       "A chen? Out here? Someone has been careless with our trajectory."},
		{"Pellegrini", "Do not look at me, thief. I have kept my sisters blind for weeks."},
		{"Mieli",      "It doesn't matter how they found us. Perhonen, can we outrun them?"},
		{"Perhonen",   "Not a chen that size. Not without a head start we do not have."},
		{"Jean",       "Then we don't outrun them. We outthink them. That is what I do."},
		{"Pellegrini", "For once, the thief and I agree. Perhonen, show them what the q-dots see."},
	};
	m_phase4Script = {
		{"Perhonen",   "Three guberniya escorts in formation. And behind them..."},
		{"Mieli",      "That is the chen's capital ship."},
		{"Jean",       "Look at the size of it. It really does want us dead."},
		{"Perhonen",   "Correction: it wants you alive. The kill signatures are for me."},
		{"Pellegrini", "At last, to war."},  // voice: towar
		{"Mieli",      "Then we run. Now."},
	};
	m_phase4Line = 0;
	m_chenScript = {
		{"Pellegrini", "Incoming transmission. Unknown origin."},           // voice: incoming
		{"Perhonen",   "That is not a transmission. Something is projecting onto the bridge."},
		{"Chen",       "The conclave is ready to act."},                    // voice: conclave
		{"Jean",       "It's the chen. It's projecting directly onto our bridge."},
		{"Chen",       "Shadows that most cannot see sing songs that most cannot hear."},  // voice: shadows
		{"Mieli",      "Perhonen, block it. Cut the projection."},
		{"Perhonen",   "I can't. It has bypassed every firewall I have."},
		{"Chen",       "We bring war to our enemies."},                     // voice: bringwar
		{"Jean",       "It means us. It means Perhonen."},
		{"Chen",       "Battle has commenced."},                            // voice: commenced
		{"Pellegrini", "The projection has ended. Make of that what you will."},
	};
	m_chenLine = 0;
	m_sobornostApproach = 0.0f;
	for (int i = 0; i < 4; i++) m_shipArrived[i] = false;
	m_screenFlash = 0.0f;
	m_screenShake = 0.0f;
	m_shakeAngle = glm::vec2(0.0f);
	m_shakeAngleTarget = glm::vec2(0.0f);
	m_shipCharge = 0.0f;
	m_sailUnfurl = 1.0f;
	m_shipMode = 2; // start in combat mode (sails furled)
	m_pHighResolutionTimer = NULL;
	m_pAudio = NULL;

	m_dt = 0.0;
	m_framesPerSecond = 0;
	m_frameCount = 0;
	m_elapsedTime = 0.0f;
	m_currentDistance = 0.0f;
	m_cameraRoll = 0.0f;
}

// Destructor
Game::~Game()
{
	//game objects
	delete m_pCamera;
	delete m_pSkybox;
	delete m_pSpaceSkybox;
	delete m_pPlanarTerrain;
	delete m_pFtFont;
	delete m_pBarrelMesh;
	delete m_pHorseMesh;
	delete m_pSphere;
	if (m_pShip) { m_pShip->Release(); delete m_pShip; m_pShip = NULL; }
	if (m_pCatmullRom) { m_pCatmullRom->Release(); delete m_pCatmullRom; m_pCatmullRom = NULL; }
	delete m_pJean;
	delete m_pMieli;
	if (m_pBridge) { m_pBridge->Release(); delete m_pBridge; m_pBridge = NULL; }
	delete m_pChairMesh;
	delete m_pMonitorMesh;
	delete m_pTableMesh;
	delete m_pCruiserMesh;
	delete m_pWarmindMesh;
	delete m_pMissileMesh;
	delete m_pHudSpriteSheet;
	if (m_pParticleSystem) { m_pParticleSystem->Release(); delete m_pParticleSystem; }
	delete m_pTacticalGame;
	if (m_pEscapeSpline) { m_pEscapeSpline->Release(); delete m_pEscapeSpline; }
	delete m_pBloomSceneFBO;
	delete m_pBloomPingFBO;
	delete m_pBloomPongFBO;
	delete m_pViewportFBO;
	delete m_pAudio;

	if (m_pShaderPrograms != NULL) {
		for (unsigned int i = 0; i < m_pShaderPrograms->size(); i++)
			delete (*m_pShaderPrograms)[i];
	}
	delete m_pShaderPrograms;

	//setup objects
	delete m_pHighResolutionTimer;
}

// Initialisation:  This method only runs once at startup
void Game::Initialise()
{
	// Set the clear colour and depth
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f);

	/// Create objects
	m_pCamera = new CCamera;
	m_pSkybox = new CSkybox;
	m_pShaderPrograms = new vector <CShaderProgram *>;
	m_pPlanarTerrain = new CPlane;
	m_pFtFont = new CFreeTypeFont;
	m_pBarrelMesh = new COpenAssetImportMesh;
	m_pHorseMesh = new COpenAssetImportMesh;
	m_pSphere = new CSphere;
	m_pShip = new CShip;
	m_pAudio = new CAudio;

	int width = m_gameWindow.GetWidth();
	int height = m_gameWindow.GetHeight();

	// Set the orthographic and perspective projection matrices based on the image size
	m_pCamera->SetOrthographicProjectionMatrix(width, height);
	m_pCamera->SetPerspectiveProjectionMatrix(glm::radians(45.0f), (float) width / (float) height, 0.5f, 5000.0f);

	// Load shaders
	vector<CShader> shShaders;
	vector<string> sShaderFileNames;
	sShaderFileNames.push_back("mainShader.vert");
	sShaderFileNames.push_back("mainShader.frag");
	sShaderFileNames.push_back("textShader.vert");
	sShaderFileNames.push_back("textShader.frag");
	sShaderFileNames.push_back("sailShader.vert");
	sShaderFileNames.push_back("sailShader.frag");
	sShaderFileNames.push_back("hullShader.vert");
	sShaderFileNames.push_back("hullShader.frag");
	sShaderFileNames.push_back("skinningShader.vert");
	sShaderFileNames.push_back("skinningShader.frag");
	sShaderFileNames.push_back("shadowMap.vert");
	sShaderFileNames.push_back("shadowMap.frag");
	sShaderFileNames.push_back("shadowMapSkinned.vert");
	sShaderFileNames.push_back("particleShader.vert");  // 13
	sShaderFileNames.push_back("particleShader.frag");  // 14
	sShaderFileNames.push_back("bloomShader.vert");     // 15
	sShaderFileNames.push_back("bloomShader.frag");     // 16
	sShaderFileNames.push_back("hologramShader.vert");  // 17
	sShaderFileNames.push_back("hologramShader.frag");  // 18

	for (int i = 0; i < (int) sShaderFileNames.size(); i++) {
		string sExt = sShaderFileNames[i].substr((int) sShaderFileNames[i].size()-4, 4);
		int iShaderType;
		if (sExt == "vert") iShaderType = GL_VERTEX_SHADER;
		else if (sExt == "frag") iShaderType = GL_FRAGMENT_SHADER;
		else if (sExt == "geom") iShaderType = GL_GEOMETRY_SHADER;
		else if (sExt == "tcnl") iShaderType = GL_TESS_CONTROL_SHADER;
		else iShaderType = GL_TESS_EVALUATION_SHADER;
		CShader shader;
		shader.LoadShader("resources/shaders/"+sShaderFileNames[i], iShaderType);
		shShaders.push_back(shader);
	}

	// Create the main shader program
	CShaderProgram *pMainProgram = new CShaderProgram;
	pMainProgram->CreateProgram();
	pMainProgram->AddShaderToProgram(&shShaders[0]);
	pMainProgram->AddShaderToProgram(&shShaders[1]);
	pMainProgram->LinkProgram();
	m_pShaderPrograms->push_back(pMainProgram);

	// Create a shader program for fonts
	CShaderProgram *pFontProgram = new CShaderProgram;
	pFontProgram->CreateProgram();
	pFontProgram->AddShaderToProgram(&shShaders[2]);
	pFontProgram->AddShaderToProgram(&shShaders[3]);
	pFontProgram->LinkProgram();
	m_pShaderPrograms->push_back(pFontProgram);

	// Create a shader program for the solar sail
	CShaderProgram *pSailProgram = new CShaderProgram;
	pSailProgram->CreateProgram();
	pSailProgram->AddShaderToProgram(&shShaders[4]);
	pSailProgram->AddShaderToProgram(&shShaders[5]);
	pSailProgram->LinkProgram();
	m_pShaderPrograms->push_back(pSailProgram);

	// Create a shader program for the hull (neon glow)
	CShaderProgram *pHullProgram = new CShaderProgram;
	pHullProgram->CreateProgram();
	pHullProgram->AddShaderToProgram(&shShaders[6]);
	pHullProgram->AddShaderToProgram(&shShaders[7]);
	pHullProgram->LinkProgram();
	m_pShaderPrograms->push_back(pHullProgram);

	// Create a shader program for skinned meshes (index 4)
	CShaderProgram *pSkinningProgram = new CShaderProgram;
	pSkinningProgram->CreateProgram();
	pSkinningProgram->AddShaderToProgram(&shShaders[8]); // skinningShader.vert
	pSkinningProgram->AddShaderToProgram(&shShaders[9]); // skinningShader.frag
	pSkinningProgram->LinkProgram();
	m_pShaderPrograms->push_back(pSkinningProgram);

	// Shadow map shader for static geometry (index 5)
	CShaderProgram *pShadowProgram = new CShaderProgram;
	pShadowProgram->CreateProgram();
	pShadowProgram->AddShaderToProgram(&shShaders[10]); // shadowMap.vert
	pShadowProgram->AddShaderToProgram(&shShaders[11]); // shadowMap.frag
	pShadowProgram->LinkProgram();
	m_pShaderPrograms->push_back(pShadowProgram);

	// Shadow map shader for skinned geometry (index 6)
	CShaderProgram *pShadowSkinnedProgram = new CShaderProgram;
	pShadowSkinnedProgram->CreateProgram();
	pShadowSkinnedProgram->AddShaderToProgram(&shShaders[12]); // shadowMapSkinned.vert
	pShadowSkinnedProgram->AddShaderToProgram(&shShaders[11]); // shadowMap.frag (shared)
	pShadowSkinnedProgram->LinkProgram();
	m_pShaderPrograms->push_back(pShadowSkinnedProgram);

	// Particle shader [7]
	CShaderProgram *pParticleProgram = new CShaderProgram;
	pParticleProgram->CreateProgram();
	pParticleProgram->AddShaderToProgram(&shShaders[13]);
	pParticleProgram->AddShaderToProgram(&shShaders[14]);
	pParticleProgram->LinkProgram();
	m_pShaderPrograms->push_back(pParticleProgram);

	// Bloom shader [8]
	CShaderProgram *pBloomProgram = new CShaderProgram;
	pBloomProgram->CreateProgram();
	pBloomProgram->AddShaderToProgram(&shShaders[15]);
	pBloomProgram->AddShaderToProgram(&shShaders[16]);
	pBloomProgram->LinkProgram();
	m_pShaderPrograms->push_back(pBloomProgram);

	// Hologram shader [9] -- bridge viewport ship schematic
	CShaderProgram *pHologramProgram = new CShaderProgram;
	pHologramProgram->CreateProgram();
	pHologramProgram->AddShaderToProgram(&shShaders[17]);
	pHologramProgram->AddShaderToProgram(&shShaders[18]);
	pHologramProgram->LinkProgram();
	m_pShaderPrograms->push_back(pHologramProgram);

	// Bloom FBOs for tactical phase
	m_pBloomSceneFBO = new CFrameBufferObject;
	m_pBloomSceneFBO->Create(m_gameWindow.GetWidth(), m_gameWindow.GetHeight());
	m_pBloomPingFBO = new CFrameBufferObject;
	m_pBloomPingFBO->Create(m_gameWindow.GetWidth() / 2, m_gameWindow.GetHeight() / 2);
	m_pBloomPongFBO = new CFrameBufferObject;
	m_pBloomPongFBO->Create(m_gameWindow.GetWidth() / 2, m_gameWindow.GetHeight() / 2);

	// Fullscreen quad VAO for bloom
	{
		float quadVerts[] = {
			0,0, 0,0,  1,0, 1,0,  0,1, 0,1,
			1,0, 1,0,  1,1, 1,1,  0,1, 0,1
		};
		glGenVertexArrays(1, &m_bloomQuadVAO);
		glBindVertexArray(m_bloomQuadVAO);
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	}

	// Viewport render-to-texture FBO (bridge front wall live view)
	// Sized ~2.4:1 to roughly match the wall opening aspect; split L/R into 640x512 panels.
	m_pViewportFBO = new CFrameBufferObject;
	m_pViewportFBO->Create(1280, 512);

	// Unit-circle outline VAO (used for sensor range rings)
	// Vertex layout matches textShader: vec2 position, vec2 texCoord
	{
		std::vector<float> circleVerts;
		circleVerts.reserve(m_circleSegments * 4);
		for (int i = 0; i < m_circleSegments; i++) {
			float a = 2.0f * (float)M_PI * (float)i / (float)m_circleSegments;
			circleVerts.push_back(cosf(a));
			circleVerts.push_back(sinf(a));
			circleVerts.push_back(0.0f);
			circleVerts.push_back(0.0f);
		}
		glGenVertexArrays(1, &m_circleVAO);
		glBindVertexArray(m_circleVAO);
		glGenBuffers(1, &m_circleVBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_circleVBO);
		glBufferData(GL_ARRAY_BUFFER, circleVerts.size() * sizeof(float),
		             circleVerts.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
		                      (void*)(2 * sizeof(float)));
		glBindVertexArray(0);
	}

	// Create the skybox
	// Skybox downloaded from http://www.akimbo.in/forum/viewtopic.php?f=10&t=9
	m_pSkybox->Create(2500.0f);

	m_pSpaceSkybox = new CSkybox;
	m_pSpaceSkybox->Create(2500.0f, "resources/skyboxes/sky-pano-milkyway/flipped/", "milkyway");

	// Create the planar terrain
	m_pPlanarTerrain->Create("resources/textures/", "grassfloor01.jpg", 2000.0f, 2000.0f, 50.0f); // Texture downloaded from http://www.psionicgames.com/?page_id=26 on 24 Jan 2013

	if (!m_pFtFont->LoadFont("/usr/share/fonts/liberation/LiberationSans-Regular.ttf", 32))
		fprintf(stderr, "ERROR: Failed to load font!\n");
	else
		fprintf(stderr, "Font loaded OK\n");
	m_pFtFont->SetShaderProgram(pFontProgram);

	// Dialogue box quad (unit quad, scaled at render time)
	float boxVerts[] = {
		0.0f, 0.0f,  0.0f, 0.0f,
		1.0f, 0.0f,  1.0f, 0.0f,
		0.0f, 1.0f,  0.0f, 1.0f,
		1.0f, 1.0f,  1.0f, 1.0f,
	};
	glGenVertexArrays(1, &m_dialogueVAO);
	glBindVertexArray(m_dialogueVAO);
	glGenBuffers(1, &m_dialogueVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_dialogueVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(boxVerts), boxVerts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	// 1x1 white texture for the box background
	GLubyte white = 255;
	glGenTextures(1, &m_whiteTex);
	glBindTexture(GL_TEXTURE_2D, m_whiteTex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &white);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	// Character portraits
	m_portraitJean = new CTexture;
	m_portraitMieli = new CTexture;
	m_portraitPellegrini = new CTexture;
	m_portraitPerhonen = new CTexture;
	m_portraitChen = new CTexture;
	auto loadPortrait = [](CTexture* tex, const char* path) {
		if (!tex->Load(path, false)) {
			fprintf(stderr, "Warning: %s not found\n", path);
			return;
		}
		tex->SetSamplerObjectParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		tex->SetSamplerObjectParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	};
	// Jean portrait: "The Stranger" by Tom Edwards (deviantart.com/tomedwardsconcepts/art/The-Stranger-334851059)
	loadPortrait(m_portraitJean, "resources/textures/portrait_Jean.jpg");
	loadPortrait(m_portraitMieli, "resources/textures/portrait_mieli.jpg");
	// Pellegrini portrait: commissioned art by cyberae0n (tumblr.com/cyberae0n/633245267827851264)
	loadPortrait(m_portraitPellegrini, "resources/textures/portrait_pellegrini.jpg");
	// Perhonen portrait: butterfly metaphor (weshape.tech)
	loadPortrait(m_portraitPerhonen, "resources/textures/portrait_perhonen.jpg");
	loadPortrait(m_portraitChen, "resources/textures/portrait_chen.jpg");

	// HUD sprite sheet (vecteezy.com — futuristic HUD screen overlay)
	m_pHudSpriteSheet = new CTexture;
	m_pHudSpriteSheet->Load("resources/textures/hud_spritesheet.jpg");
	m_pHudSpriteSheet->SetSamplerObjectParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_pHudSpriteSheet->SetSamplerObjectParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_pHudSpriteSheet->SetSamplerObjectParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_pHudSpriteSheet->SetSamplerObjectParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Bridge textures
	m_pFloorTex = new CTexture;
	m_pFloorTex->Load("resources/textures/tiles.jpg");
	m_pWallTex = new CTexture;
	m_pWallTex->Load("resources/textures/wall.jpg");
	// Viewport texture: space view from bridge front wall
	// Source: https://www.wrir.org/wp-content/uploads/2022/03/Expanse.jpg
	m_pViewportTex = new CTexture;
	m_pViewportTex->Load("resources/textures/viewport.png");

	// Shadow map FBO
	glGenFramebuffers(1, &m_shadowMapFBO);
	glGenTextures(1, &m_shadowMapTex);
	glBindTexture(GL_TEXTURE_2D, m_shadowMapTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMapTex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Spot shadow map (overhead bridge light)
	glGenFramebuffers(1, &m_spotShadowFBO);
	glGenTextures(1, &m_spotShadowTex);
	glBindTexture(GL_TEXTURE_2D, m_spotShadowTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glBindFramebuffer(GL_FRAMEBUFFER, m_spotShadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_spotShadowTex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Load some meshes in OBJ format
	m_pBarrelMesh->Load("resources/models/Barrel/barrel02.obj");  // Downloaded from http://www.psionicgames.com/?page_id=24 on 24 Jan 2013
	m_pHorseMesh->Load("resources/models/Horse/horse2.obj");  // Downloaded from http://opengameart.org/content/horse-lowpoly on 24 Jan 2013

	// Create a sphere
	m_pSphere->Create("resources/textures/", "dirtpile01.jpg", 25, 25);  // Texture downloaded from http://www.psionicgames.com/?page_id=26 on 24 Jan 2013

	// Hull uses neon circuit texture, sails use iridescent texture
	m_pShip->Create("resources/textures/", "neon.png",
	                "resources/textures/", "iridescent.png");
	glEnable(GL_CULL_FACE);

	// Load Jean (animated character)
	m_pJean = new CAnimatedMesh;
	m_pJean->Load("resources/models/Jean/jeansit.fbx");
	m_pJean->LoadAnimation("resources/models/Jean/jeansit.fbx", "sit");
	m_pJean->LoadAnimation("resources/models/Jean/idle.fbx", "idle");
	m_pJean->LoadAnimation("resources/models/Jean/walking.fbx", "walk");
	m_pJean->LoadAnimation("resources/models/Jean/running.fbx", "run");
	m_pJean->LoadAnimation("resources/models/Jean/jump.fbx", "jump");
	m_pJean->SetTexture("resources/models/Jean/jean_tex0.png");
	m_pJean->SetAnimation("sit");

	// Load Mieli (animated character)
	m_pMieli = new CAnimatedMesh;
	m_pMieli->Load("resources/models/Mieli/mieli_multimaterial.fbx", true);
	m_pMieli->LoadAnimation("resources/models/Mieli/idle.fbx", "idle");
	m_pMieli->LoadAnimation("resources/models/Mieli/walking.fbx", "walking");
	m_pMieli->LoadAnimation("resources/models/Mieli/running.fbx", "running");
	m_pMieli->LoadAnimation("resources/models/Mieli/jump.fbx", "jump");
	m_pMieli->LoadAnimation("resources/models/Mieli/sitting talking.fbx", "sitting_talking");
	m_pMieli->LoadAnimation("resources/models/Mieli/left strafe.fbx", "left_strafe");
	m_pMieli->LoadAnimation("resources/models/Mieli/left strafe walk.fbx", "left_strafe_walk");
	m_pMieli->LoadAnimation("resources/models/Mieli/left turn.fbx", "left_turn");
	m_pMieli->LoadAnimation("resources/models/Mieli/right strafe.fbx", "right_strafe");
	m_pMieli->LoadAnimation("resources/models/Mieli/right strafe walk.fbx", "right_strafe_walk");
	m_pMieli->LoadAnimation("resources/models/Mieli/right turn.fbx", "right_turn");
	// Per-material textures loaded via InitMaterials fallback (texture/<name>_diff*.jpg)
	m_pMieli->SetAnimation("sitting_talking");

	m_pChen = new CAnimatedMesh;
	m_pChen->Load("resources/models/chen/Salute.fbx");
	m_pChen->LoadAnimation("resources/models/chen/Salute.fbx", "salute");
	m_pChen->SetAnimation("salute");
	m_pChen->SetLooping(false); // hold last frame after salute

	// Create procedural bridge room (6m wide, 3m tall, 8m deep)
	m_pBridge = new CBridge;
	m_pBridge->Create(6.0f, 3.0f, 8.0f);

	// Load furniture
	m_pChairMesh = new COpenAssetImportMesh;
	m_pChairMesh->Load("resources/models/Chair/source/Chair.obj");
	m_pMonitorMesh = new COpenAssetImportMesh;
	m_pMonitorMesh->Load("resources/models/MonitoringStation/source/Stol2.obj");
	m_pTableMesh = new COpenAssetImportMesh;
	m_pTableMesh->Load("resources/models/TableHall.obj");

	m_pCruiserMesh = new COpenAssetImportMesh;
	m_pCruiserMesh->Load("resources/models/cruiser.glb");
	m_pWarmindMesh = new COpenAssetImportMesh;
	m_pWarmindMesh->Load("resources/models/warmind.glb");
	// missile.glb wasn't loading reliably; fall back to laser-line rendering
	// (TacticalGame::RenderProjectiles checks m_pMissileMesh != NULL)
	m_pMissileMesh = NULL;

	// Create the Catmull-Rom circular camera path and track
	m_pCatmullRom = new CCatmullRom;
	m_pCatmullRom->CreateCentreline();
	m_pCatmullRom->CreateOffsetCurves();
	m_pCatmullRom->CreateTrack("resources/textures/", "asteroids.jpg");

	// Particle system for explosions
	m_pParticleSystem = new CParticleSystem;
	m_pParticleSystem->Create((*m_pShaderPrograms)[7]);

	// Create escape spline (long non-looping path) and tactical game
	m_pEscapeSpline = new CCatmullRom;
	m_pEscapeSpline->CreateCentrelineEscape();
	m_pEscapeSpline->CreateOffsetCurves();
	m_pEscapeSpline->CreateTrack("resources/textures/", "asteroids.jpg");

	m_pTacticalGame = new CTacticalGame;
	m_pTacticalGame->Init(m_pEscapeSpline, m_pShip, m_pCruiserMesh, m_pWarmindMesh,
	                      m_pMissileMesh, m_pParticleSystem, m_pAudio,
	                      m_portraitPerhonen, m_portraitMieli, m_portraitJean,
	                      m_portraitChen, m_portraitPellegrini, m_dialogueVAO);

	// Title screen texture
	m_pTitleTex = new CTexture;
	m_pTitleTex->Load("resources/textures/title.png", false);
	m_pTitleTex->SetSamplerObjectParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_pTitleTex->SetSamplerObjectParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Initialise audio and load all sounds
	m_pAudio->Initialise();

	// Bridge ambience (one-shot, played periodically)
	m_pAudio->LoadSound("bridge1", "resources/audio/bridge1.wav");
	m_pAudio->LoadSound("bridge2", "resources/audio/bridge2.wav");
	m_pAudio->LoadSound("bridge3", "resources/audio/bridge3.wav");
	m_pAudio->LoadSound("shipflight", "resources/audio/shipflight.wav", true);

	// SFX
	m_pAudio->LoadSound("sensor", "resources/audio/sensor.wav");
	m_pAudio->LoadSound("alert", "resources/audio/alert.wav");
	m_pAudio->LoadSound("warpin", "resources/audio/warpin.wav");
	m_pAudio->LoadSound("fire1", "resources/audio/fire1.wav");
	m_pAudio->LoadSound("fire2", "resources/audio/fire2.wav");
	m_pAudio->LoadSound("hit", "resources/audio/hit.wav");
	m_pAudio->LoadSound("hologram1", "resources/audio/hologram1.wav");
	m_pAudio->LoadSound("hologram2", "resources/audio/hologram2.wav");
	m_pAudio->LoadSound("hologram3", "resources/audio/hologram3.wav");
	m_pAudio->LoadSound("weapondwind", "resources/audio/weapondwind.wav");
	m_pAudio->LoadSound("loss", "resources/audio/loss.wav");

	// Pellegrini voice lines
	m_pAudio->LoadSound("tracking", "resources/audio/trackingunidentifiedobject.wav");
	m_pAudio->LoadSound("incoming", "resources/audio/incoming_transmission.wav");
	m_pAudio->LoadSound("strangegame", "resources/audio/strangegame.wav");
	m_pAudio->LoadSound("letslip", "resources/audio/let_slip_the_dogs.wav");
	m_pAudio->LoadSound("towar", "resources/audio/atlast_to_war.wav");
	m_pAudio->LoadSound("battlejoined", "resources/audio/battle_is_joined.wav");

	// Chen voice lines
	m_pAudio->LoadSound("conclave", "resources/audio/conclave_ready.wav");
	m_pAudio->LoadSound("shadows", "resources/audio/shadows_sing_songs.wav");
	m_pAudio->LoadSound("bringwar", "resources/audio/bring_war_to_enemies.wav");
	m_pAudio->LoadSound("commenced", "resources/audio/battle_has_commenced.wav");

	// Background music — loops softly throughout
	m_pAudio->LoadSound("music", "resources/audio/gameaudio.mp3", true);
	m_pAudio->PlaySound("music", 0.25f);

	// Play first bridge sound for title screen
	m_pAudio->PlaySound("bridge1", 0.4f);
	m_bridgeSoundTimer = 10.0f;  // next bridge sound in ~10s
}

void Game::SetMatrices(CShaderProgram *prog, glm::mat4 modelView, glm::mat4 shadowBias, glm::mat4 spotShadowBias)
{
	prog->SetUniform("matrices.modelViewMatrix", modelView);
	prog->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelView));
	prog->SetUniform("lightSpaceMatrix", shadowBias * modelView);
	prog->SetUniform("spotLightSpaceMatrix", spotShadowBias * modelView);
}

void Game::RenderShadowMap()
{
	glm::vec3 bridgeCenter(0.0f, 25.0f, 60.0f);
	glm::vec3 lightPos(-100.0f, 100.0f, -100.0f);
	m_lightViewMatrix = glm::lookAt(lightPos, bridgeCenter, glm::vec3(0.0f, 1.0f, 0.0f));
	m_lightProjMatrix = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 50.0f, 300.0f);
	glm::mat4 lightVP = m_lightProjMatrix * m_lightViewMatrix;

	glm::vec3 bridgeOrigin(0.0f, 25.0f, 60.0f);

	// Precompute model matrices (shared between sun and spot passes)
	glm::mat4 bridgeModel = glm::translate(glm::mat4(1.0f), bridgeOrigin);
	// Chair 1 — Mieli's (matches main render)
	glm::mat4 chairModel1 = glm::translate(glm::mat4(1.0f), bridgeOrigin + glm::vec3(1.0f, 0.0f, -2.2f));
	chairModel1 = glm::rotate(chairModel1, -0.2f, glm::vec3(0.0f, 1.0f, 0.0f));
	chairModel1 = glm::scale(chairModel1, glm::vec3(0.5f));
	// Chair 2 — Jean's
	glm::mat4 chairModel2 = glm::translate(glm::mat4(1.0f), bridgeOrigin + glm::vec3(-1.0f, 0.0f, -2.2f));
	chairModel2 = glm::rotate(chairModel2, 3.2f, glm::vec3(0.0f, 1.0f, 0.0f));
	chairModel2 = glm::scale(chairModel2, glm::vec3(0.5f));
	glm::mat4 tableModel = glm::scale(glm::translate(glm::mat4(1.0f), bridgeOrigin + glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.5f));
	float jeanRotY = (m_cutscenePhase == 5) ? 3.14f : (m_cutscenePhase >= 1) ? 0.0f : 1.56f;
	glm::mat4 jeanModel = glm::translate(glm::mat4(1.0f), bridgeOrigin + m_jeanPos);
	jeanModel = glm::rotate(jeanModel, jeanRotY, glm::vec3(0.0f, 1.0f, 0.0f));
	jeanModel = glm::scale(jeanModel, glm::vec3(0.005f));
	float mieliRotY = (m_cutscenePhase == 5) ? 3.14f : (m_cutscenePhase >= 1) ? 0.0f : -1.56f;
	glm::mat4 mieliModel = glm::translate(glm::mat4(1.0f), bridgeOrigin + m_mieliPos);
	mieliModel = glm::rotate(mieliModel, mieliRotY, glm::vec3(0.0f, 1.0f, 0.0f));
	mieliModel = glm::scale(mieliModel, glm::vec3(0.05f));

	auto renderMonitorShadow = [&](CShaderProgram *prog, glm::mat4 vp, glm::vec3 offset, float rotY) {
		glm::mat4 m = glm::translate(glm::mat4(1.0f), bridgeOrigin + offset);
		if (rotY != 0.0f) m = glm::rotate(m, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
		m = glm::scale(m, glm::vec3(0.5f));
		prog->SetUniform("lightMVP", vp * m);
		m_pMonitorMesh->Render();
	};

	auto renderSceneShadow = [&](CShaderProgram *pStatic, CShaderProgram *pSkinned, glm::mat4 vp, bool includeBridge) {
		pStatic->UseProgram();
		if (includeBridge) {
			pStatic->SetUniform("lightMVP", vp * bridgeModel);
			m_pBridge->RenderFloor();
			m_pBridge->RenderWalls();
		}
		pStatic->SetUniform("lightMVP", vp * chairModel1);
		m_pChairMesh->Render();
		pStatic->SetUniform("lightMVP", vp * chairModel2);
		m_pChairMesh->Render();
		renderMonitorShadow(pStatic, vp, glm::vec3(-2.5f, 0.0f, -1.5f), 0.0f);
		renderMonitorShadow(pStatic, vp, glm::vec3(-2.5f, 0.0f,  1.5f), 0.0f);
		renderMonitorShadow(pStatic, vp, glm::vec3( 2.5f, 0.0f, -1.5f), 3.14159f);
		renderMonitorShadow(pStatic, vp, glm::vec3( 2.5f, 0.0f,  1.5f), 3.14159f);
		pStatic->SetUniform("lightMVP", vp * tableModel);
		m_pTableMesh->Render();

		pSkinned->UseProgram();
		pSkinned->SetUniform("lightMVP", vp * jeanModel);
		pSkinned->SetUniform("boneMatrices", m_pJean->GetBoneMatrices(), (int)m_pJean->GetNumBones());
		m_pJean->Render();
		pSkinned->SetUniform("lightMVP", vp * mieliModel);
		pSkinned->SetUniform("lightMVP", vp * mieliModel);
		pSkinned->SetUniform("boneMatrices", m_pMieli->GetBoneMatrices(), (int)m_pMieli->GetNumBones());
		m_pMieli->Render();
	};

	CShaderProgram *pShadowProg = (*m_pShaderPrograms)[5];
	CShaderProgram *pShadowSkinProg = (*m_pShaderPrograms)[6];

	// --- Sun shadow pass ---
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
	glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	glClear(GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);
	renderSceneShadow(pShadowProg, pShadowSkinProg, lightVP, true);
	// Terrain (single-sided, skip face culling)
	pShadowProg->UseProgram();
	glDisable(GL_CULL_FACE);
	pShadowProg->SetUniform("lightMVP", lightVP);
	m_pPlanarTerrain->Render();
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// --- Viewport spotlight shadow pass (light from front wall center toward rear) ---
	glm::vec3 spotPos = bridgeCenter + glm::vec3(0.0f, 1.0f, 5.0f);
	glm::vec3 spotTarget = bridgeCenter;
	m_spotViewMatrix = glm::lookAt(spotPos, spotTarget, glm::vec3(0.0f, 1.0f, 0.0f));
	m_spotProjMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 1.0f, 20.0f);
	glm::mat4 spotVP = m_spotProjMatrix * m_spotViewMatrix;

	glBindFramebuffer(GL_FRAMEBUFFER, m_spotShadowFBO);
	glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	glClear(GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);
	renderSceneShadow(pShadowProg, pShadowSkinProg, spotVP, false);
	glCullFace(GL_BACK);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_gameWindow.GetWidth(), m_gameWindow.GetHeight());
}

void Game::RenderViewportFBO()
{
	if (!m_cutsceneActive) return;

	m_pViewportFBO->Bind();

	// FBO is 1280x512 with two 640x512 panels side by side (matches wall aspect).
	const int fboW = 1280;
	const int fboH = 512;
	const int panelW = fboW / 2;
	const float panelAspect = (float)panelW / (float)fboH;
	// Continuous monotonic time (m_elapsedTime resets every 1s for FPS counting).
	float t = (float)glfwGetTime();

	if (m_cutscenePhase == 3) {
		// Phase 3 transition: the smartmatter wall switches from a status
		// schematic to a diffuse-glow preview of the inbound Sobornost fleet.
		const float phase3Duration = 3.0f;
		float t01 = glm::clamp(m_cutsceneTimer / phase3Duration, 0.0f, 1.0f);
		float fleetAspect = (float)fboW / (float)fboH;

		glClearColor(0.005f, 0.012f, 0.020f, 1.0f);
		glViewport(0, 0, fboW, fboH);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		RenderDiffuseFleetPanel(t01, fleetAspect, t);
	} else {
		// Default: hologram + sensor split.
		glClearColor(0.012f, 0.025f, 0.035f, 1.0f);
		glViewport(0, 0, fboW, fboH);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// --- Left panel: rotating Perhonen hologram schematic ---
		glViewport(0, 0, panelW, fboH);
		RenderHologramPanel(panelAspect, t);

		// --- Right panel: tactical sensor sweep ---
		glViewport(panelW, 0, panelW, fboH);
		RenderSensorPanel(t);
	}

	// Restore default framebuffer + viewport
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_gameWindow.GetWidth(), m_gameWindow.GetHeight());
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void Game::RenderHologramPanel(float aspect, float time)
{
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive: holographic glow
	glDisable(GL_CULL_FACE);

	// Slow continuous orbit around the schematic.
	float orbitYaw = time * 0.45f;
	float camDist = 7.5f;
	glm::vec3 eye(sinf(orbitYaw) * camDist, 1.6f, cosf(orbitYaw) * camDist);
	glm::mat4 viewMatrix = glm::lookAt(eye, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0, 1, 0));
	glm::mat4 projMatrix = glm::perspective(glm::radians(35.0f), aspect, 0.1f, 60.0f);

	// Ship is built at world scale ~16 long, scale it down to fit the panel.
	glm::mat4 shipModel = glm::scale(glm::mat4(1.0f), glm::vec3(0.32f));
	shipModel = glm::rotate(shipModel, glm::radians(-12.0f), glm::vec3(1, 0, 0));
	glm::mat4 shipMV = viewMatrix * shipModel;
	glm::mat3 shipNM = glm::transpose(glm::inverse(glm::mat3(shipMV)));

	CShaderProgram *pHoloProgram = (*m_pShaderPrograms)[9];
	pHoloProgram->UseProgram();
	pHoloProgram->SetUniform("matrices.projMatrix", projMatrix);
	pHoloProgram->SetUniform("matrices.modelViewMatrix", shipMV);
	pHoloProgram->SetUniform("matrices.normalMatrix", shipNM);
	pHoloProgram->SetUniform("time", time);
	pHoloProgram->SetUniform("holoColour", glm::vec3(0.35f, 0.95f, 1.0f));
	pHoloProgram->SetUniform("wipeAxisScale", 0.06f);

	m_pShip->RenderHull();

	if (m_sailUnfurl > 0.01f) {
		pHoloProgram->SetUniform("holoColour", glm::vec3(0.55f, 0.85f, 1.0f));
		m_pShip->RenderSails();
	}

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	// HUD labels (FreeType) — design coords 640x512 match panel pixels.
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	CShaderProgram *fontProgram = (*m_pShaderPrograms)[1];
	fontProgram->UseProgram();
	glm::mat4 ortho = glm::ortho(0.0f, 640.0f, 0.0f, 512.0f);
	fontProgram->SetUniform("matrices.projMatrix", ortho);
	fontProgram->SetUniform("matrices.modelViewMatrix", glm::mat4(1.0f));
	fontProgram->SetUniform("vColour", glm::vec4(0.6f, 0.95f, 1.0f, 0.95f));
	m_pFtFont->Render(16, 478, 24, "PERHONEN  //  STATUS");

	int chargePct = (int)(m_shipCharge * 100.0f + 0.5f);
	int unfurlPct = (int)(m_sailUnfurl * 100.0f + 0.5f);
	const char* modeStr = (m_shipMode == 2) ? "COMBAT" : "CRUISE";
	char buf[64];
	m_pFtFont->Render(16, 22, 18, "HULL  100%%");
	snprintf(buf, sizeof(buf), "CHRG %3d%%", chargePct);
	m_pFtFont->Render(150, 22, 18, buf);
	snprintf(buf, sizeof(buf), "SAIL %3d%%", unfurlPct);
	m_pFtFont->Render(290, 22, 18, buf);
	fontProgram->SetUniform("vColour", glm::vec4(1.0f, 0.85f, 0.4f, 0.95f));
	m_pFtFont->Render(440, 22, 18, modeStr);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void Game::RenderDiffuseFleetPanel(float t01, float aspect, float time)
{
	// Camera positioned just inside the bridge front wall, looking outward.
	glm::vec3 viewPos(0.0f, 26.0f, 64.0f);
	glm::vec3 viewTarget(0.0f, 26.0f, 200.0f);
	glm::mat4 viewMatrix = glm::lookAt(viewPos, viewTarget, glm::vec3(0, 1, 0));
	glm::mat4 projMatrix = glm::perspective(glm::radians(45.0f), aspect, 0.5f, 5000.0f);

	CShaderProgram *pMain = (*m_pShaderPrograms)[0];
	pMain->UseProgram();
	pMain->SetUniform("matrices.projMatrix", projMatrix);
	pMain->SetUniform("sampler0", 0);
	pMain->SetUniform("numPointLights", 0);
	pMain->SetUniform("CubeMapTex", 10);
	pMain->SetUniform("bMirror", false);
	pMain->SetUniform("alpha", 0.0f);

	glm::mat4 ident(1.0f);

	// --- Skybox background ---
	pMain->SetUniform("renderSkybox", true);
	glm::mat4 skyMV = viewMatrix * glm::translate(glm::mat4(1.0f), viewPos);
	SetMatrices(pMain, skyMV, ident, ident);
	m_pSpaceSkybox->Render(10);
	pMain->SetUniform("renderSkybox", false);

	(void)viewMatrix;  // ships are deliberately not rendered — diffuse view only.

	// --- Diffuse haze overlay: additive warm haze + scanlines + a
	// soft red warning tint that builds with t01. ---
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	CShaderProgram *fontProgram = (*m_pShaderPrograms)[1];
	fontProgram->UseProgram();
	glm::mat4 ortho = glm::ortho(0.0f, 1280.0f, 0.0f, 512.0f);
	fontProgram->SetUniform("matrices.projMatrix", ortho);
	fontProgram->SetUniform("sampler0", 0);
	fontProgram->SetUniform("bFullColour", false);
	glActiveTexture(GL_TEXTURE0);
	glBindSampler(0, 0);
	glBindTexture(GL_TEXTURE_2D, m_whiteTex);
	glBindVertexArray(m_dialogueVAO);

	// Soft warm haze across the whole panel — the "diffuse" atmospheric look.
	{
		glm::mat4 m = glm::scale(glm::mat4(1.0f), glm::vec3(1280.0f, 512.0f, 1.0f));
		fontProgram->SetUniform("matrices.modelViewMatrix", m);
		float warmth = 0.06f + 0.05f * sinf(time * 0.6f);
		fontProgram->SetUniform("vColour",
			glm::vec4(warmth + 0.04f * t01, warmth, warmth * 0.7f, 0.9f));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	// Scanline ribbon — thin horizontal additive bands every ~24 pixels.
	for (int y = 0; y < 512; y += 24) {
		glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, (float)y, 0.0f))
		            * glm::scale(glm::mat4(1.0f), glm::vec3(1280.0f, 1.0f, 1.0f));
		fontProgram->SetUniform("matrices.modelViewMatrix", m);
		fontProgram->SetUniform("vColour", glm::vec4(0.20f, 0.30f, 0.45f, 0.18f));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	// Red threat tint that ramps with t01 — sells the "they're already here" beat.
	{
		float redA = 0.08f + 0.18f * t01;
		glm::mat4 m = glm::scale(glm::mat4(1.0f), glm::vec3(1280.0f, 512.0f, 1.0f));
		fontProgram->SetUniform("matrices.modelViewMatrix", m);
		fontProgram->SetUniform("vColour", glm::vec4(0.80f, 0.10f, 0.05f, redA));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	// --- Warning text: "INCOMING / SOBORNOST FLEET" ---
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	fontProgram->SetUniform("matrices.modelViewMatrix", glm::mat4(1.0f));
	float textPulse = 0.6f + 0.4f * sinf(time * 6.5f);
	fontProgram->SetUniform("vColour",
		glm::vec4(1.0f, 0.18f, 0.12f, 0.55f + 0.45f * textPulse));
	m_pFtFont->Render(20, 470, 28, "WARNING");
	fontProgram->SetUniform("vColour", glm::vec4(1.0f, 0.65f, 0.40f, 0.85f));
	m_pFtFont->Render(20, 30, 22, "INBOUND  SOBORNOST FLEET");

	glBindVertexArray(0);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void Game::RenderSensorPanel(float time)
{
	// 2D pass via the text/quad shader. Design coords 640x512 match panel pixels.
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	CShaderProgram *fontProgram = (*m_pShaderPrograms)[1];
	fontProgram->UseProgram();
	glm::mat4 ortho = glm::ortho(0.0f, 640.0f, 0.0f, 512.0f);
	fontProgram->SetUniform("matrices.projMatrix", ortho);
	fontProgram->SetUniform("sampler0", 0);
	fontProgram->SetUniform("bFullColour", false);

	glActiveTexture(GL_TEXTURE0);
	glBindSampler(0, 0);
	glBindTexture(GL_TEXTURE_2D, m_whiteTex);

	auto drawQuad = [&](float x, float y, float w, float h, float rotRad, glm::vec4 col) {
		fontProgram->SetUniform("vColour", col);
		glm::mat4 m(1.0f);
		m = glm::translate(m, glm::vec3(x, y, 0.0f));
		if (rotRad != 0.0f) m = glm::rotate(m, rotRad, glm::vec3(0, 0, 1));
		m = glm::translate(m, glm::vec3(-w * 0.5f, -h * 0.5f, 0.0f));
		m = glm::scale(m, glm::vec3(w, h, 1.0f));
		fontProgram->SetUniform("matrices.modelViewMatrix", m);
		glBindVertexArray(m_dialogueVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	};

	// Background tint
	drawQuad(320.0f, 256.0f, 640.0f, 512.0f, 0.0f, glm::vec4(0.02f, 0.06f, 0.04f, 1.0f));

	// Sensor centre and max radius — radar fills upper portion, HUD strip below.
	const float cx = 320.0f;
	const float cy = 290.0f;
	const float rMax = 175.0f;
	const int numRings = 4;

	// Range rings — line loops with the unit-circle VAO
	{
		glBindVertexArray(m_circleVAO);
		glLineWidth(1.2f);
		for (int i = 1; i <= numRings; i++) {
			float r = rMax * (float)i / (float)numRings;
			glm::mat4 m(1.0f);
			m = glm::translate(m, glm::vec3(cx, cy, 0.0f));
			m = glm::scale(m, glm::vec3(r, r, 1.0f));
			fontProgram->SetUniform("matrices.modelViewMatrix", m);
			fontProgram->SetUniform("vColour", glm::vec4(0.18f, 0.65f, 0.40f, 0.55f));
			glDrawArrays(GL_LINE_LOOP, 0, m_circleSegments);
		}
		// Cardinal cross — thin quads
		drawQuad(cx, cy, rMax * 2.0f, 1.0f, 0.0f, glm::vec4(0.18f, 0.55f, 0.35f, 0.45f));
		drawQuad(cx, cy, 1.0f, rMax * 2.0f, 0.0f, glm::vec4(0.18f, 0.55f, 0.35f, 0.45f));
	}

	// Rotating sweep with a fading trail
	{
		float sweepAng = time * 1.2f;
		const int trailSegs = 14;
		for (int i = 0; i < trailSegs; i++) {
			float a = sweepAng - (float)i * 0.05f;
			float alpha = (1.0f - (float)i / (float)trailSegs) * 0.55f;
			float w = 2.4f - (float)i * 0.1f;
			if (w < 0.6f) w = 0.6f;
			float ex = cx + cosf(a) * rMax * 0.5f;
			float ey = cy + sinf(a) * rMax * 0.5f;
			drawQuad(ex, ey, rMax, w, a,
			         glm::vec4(0.35f, 1.0f, 0.55f, alpha));
		}
		float ex = cx + cosf(sweepAng) * rMax * 0.5f;
		float ey = cy + sinf(sweepAng) * rMax * 0.5f;
		drawQuad(ex, ey, rMax, 3.5f, sweepAng,
		         glm::vec4(0.7f, 1.0f, 0.8f, 0.95f));
	}

	// Centre Perhonen marker
	drawQuad(cx, cy, 14.0f, 14.0f, glm::radians(45.0f),
	         glm::vec4(0.5f, 1.0f, 0.6f, 1.0f));
	drawQuad(cx, cy, 6.0f, 6.0f, glm::radians(45.0f),
	         glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	// Enemy contacts — Sobornost cruisers + Founders warmind, mirrored from cutscene state.
	struct Contact { float bearingDeg; float rangeT; bool warmind; };
	Contact contacts[4] = {
		{  35.0f, 0.55f, false },
		{  62.0f, 0.78f, false },
		{  18.0f, 0.45f, false },
		{ 145.0f, 0.95f, true  },
	};
	float ap = m_sobornostApproach;
	int contactCount = 0;
	for (int i = 0; i < 4; i++) {
		if (!m_shipArrived[i] && ap < 0.05f) continue;
		contactCount++;
		float ang = glm::radians(contacts[i].bearingDeg);
		float rangeFrac = contacts[i].rangeT * (1.0f - 0.6f * ap);
		float r = rMax * rangeFrac;
		float px = cx + cosf(ang) * r;
		float py = cy + sinf(ang) * r;

		bool hot = m_shipArrived[i];
		glm::vec4 col = hot
			? glm::vec4(1.0f, 0.18f, 0.18f, 0.95f)
			: glm::vec4(1.0f, 0.65f, 0.30f, 0.65f);
		float pulse = 0.85f + 0.15f * sinf(time * 6.0f + (float)i);
		float size = (contacts[i].warmind ? 16.0f : 10.0f) * pulse;

		drawQuad(px, py, size * 2.2f, size * 2.2f, 0.0f,
		         glm::vec4(col.r, col.g, col.b, col.a * 0.18f));
		drawQuad(px, py, size, size, glm::radians(45.0f), col);

		if (contacts[i].warmind && hot) {
			glBindVertexArray(m_circleVAO);
			glm::mat4 m(1.0f);
			m = glm::translate(m, glm::vec3(px, py, 0.0f));
			m = glm::scale(m, glm::vec3(size * 1.6f, size * 1.6f, 1.0f));
			fontProgram->SetUniform("matrices.modelViewMatrix", m);
			fontProgram->SetUniform("vColour", glm::vec4(1.0f, 0.2f, 0.2f, 0.9f));
			glLineWidth(2.0f);
			glDrawArrays(GL_LINE_LOOP, 0, m_circleSegments);
		}
	}

	// HUD — header at top, status strip at bottom
	glm::vec4 labelCol = (contactCount > 0 && (m_shipArrived[3] || ap > 0.6f))
		? glm::vec4(1.0f, 0.3f, 0.3f, 0.95f)
		: glm::vec4(0.55f, 1.0f, 0.65f, 0.95f);
	fontProgram->SetUniform("matrices.modelViewMatrix", glm::mat4(1.0f));
	fontProgram->SetUniform("vColour", labelCol);
	m_pFtFont->Render(16, 478, 24, "TACTICAL  //  LONG-RANGE SCAN");

	char buf[64];
	snprintf(buf, sizeof(buf), "CONTACTS %d", contactCount);
	fontProgram->SetUniform("vColour", labelCol);
	m_pFtFont->Render(16, 88, 18, buf);

	if (m_shipArrived[3]) {
		float warn = 0.5f + 0.5f * sinf(time * 8.0f);
		fontProgram->SetUniform("vColour", glm::vec4(1.0f, 0.15f, 0.15f, 0.6f + 0.4f * warn));
		m_pFtFont->Render(170, 88, 18, "ALERT  FOUNDERS");
	} else if (ap > 0.05f) {
		fontProgram->SetUniform("vColour", glm::vec4(1.0f, 0.85f, 0.4f, 0.85f));
		m_pFtFont->Render(170, 88, 18, "INBOUND  SOBORNOST");
	}

	fontProgram->SetUniform("vColour", glm::vec4(0.55f, 1.0f, 0.65f, 0.8f));
	float primaryAng = m_shipArrived[3] ? contacts[3].bearingDeg : contacts[0].bearingDeg;
	float primaryRng = (m_shipArrived[3] ? contacts[3].rangeT : contacts[0].rangeT) * (1.0f - 0.6f * ap);
	snprintf(buf, sizeof(buf), "BRG %3d  RNG %.2f", (int)(primaryAng + 0.5f), primaryRng);
	m_pFtFont->Render(16, 56, 16, buf);
	snprintf(buf, sizeof(buf), "SCAN %.0f%%", glm::clamp(ap * 100.0f, 0.0f, 100.0f));
	m_pFtFont->Render(16, 28, 16, buf);

	glBindVertexArray(0);
	glLineWidth(1.0f);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void Game::Render()
{
	// --- Shadow map pass ---
	RenderShadowMap();

	// --- Viewport render-to-texture pass ---
	RenderViewportFBO();

	// Clear the buffers and enable depth testing (z-buffering)
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Title screen
	if (m_titleScreen) {
		int w = m_gameWindow.GetWidth();
		int h = m_gameWindow.GetHeight();
		CShaderProgram *fontProgram = (*m_pShaderPrograms)[1];
		m_pCamera->SetOrthographicProjectionMatrix(w, h);
		fontProgram->UseProgram();
		glDisable(GL_DEPTH_TEST);
		fontProgram->SetUniform("matrices.projMatrix", m_pCamera->GetOrthographicProjectionMatrix());

		// Fullscreen title image
		fontProgram->SetUniform("bFullColour", true);
		fontProgram->SetUniform("vColour", glm::vec4(1.0f));
		glBindVertexArray(m_dialogueVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindSampler(0, 0);
		m_pTitleTex->Bind(0);
		fontProgram->SetUniform("sampler0", 0);
		glm::mat4 mv = glm::scale(glm::mat4(1.0f), glm::vec3((float)w, (float)h, 1.0f));
		fontProgram->SetUniform("matrices.modelViewMatrix", mv);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// Title text
		fontProgram->SetUniform("bFullColour", false);
		fontProgram->SetUniform("vColour", glm::vec4(0.9f, 0.75f, 0.3f, 1.0f));
		int titleSize = h / 12;
		string title = "DOOMSDAY CATALYST";
		int titleW = m_pFtFont->GetTextWidth(title, titleSize);
		m_pFtFont->Print(title, (w - titleW) / 2, h * 3 / 4, titleSize);

		// Subtitle / prompt
		float pulse = 0.5f + 0.5f * sinf((float)glfwGetTime() * 2.0f);
		fontProgram->SetUniform("vColour", glm::vec4(0.7f, 0.7f, 0.8f, pulse));
		int promptSize = h / 30;
		string prompt = "Click to begin";
		int promptW = m_pFtFont->GetTextWidth(prompt, promptSize);
		m_pFtFont->Print(prompt, (w - promptW) / 2, h / 6, promptSize);

		glEnable(GL_DEPTH_TEST);
		DisplayFrameRate();
		glfwSwapBuffers(m_gameWindow.GetWindow());
		return;
	}

	// Phase 3: bridge stays visible. The FBO viewport switches to a
	// diffuse "incoming fleet" preview (handled inside RenderViewportFBO),
	// and the HUD overlay fades in over the bridge view (added at the end
	// of the main render, near the swap call).

	// Phase 6: tactical escape gameplay rendering
	if (m_cutscenePhase == 6) {
		glutil::MatrixStack ms;
		ms.SetIdentity();

		CShaderProgram *pMainProgram = (*m_pShaderPrograms)[0];
		pMainProgram->UseProgram();
		pMainProgram->SetUniform("bUseTexture", true);
		pMainProgram->SetUniform("sampler0", 0);
		int cubeMapUnit = 10;
		pMainProgram->SetUniform("CubeMapTex", cubeMapUnit);
		pMainProgram->SetUniform("bMirror", false);
		pMainProgram->SetUniform("alpha", 0.0f);
		pMainProgram->SetUniform("numPointLights", 0);

		pMainProgram->SetUniform("matrices.projMatrix", m_pCamera->GetPerspectiveProjectionMatrix());
		ms.LookAt(m_pCamera->GetPosition(), m_pCamera->GetView(), m_pCamera->GetUpVector());
		glm::mat4 viewMatrix = ms.Top();

		glm::vec4 lightPos(-100, 100, -100, 1);
		pMainProgram->SetUniform("light1.position", viewMatrix * lightPos);
		pMainProgram->SetUniform("light1.La", glm::vec3(0.6f));
		pMainProgram->SetUniform("light1.Ld", glm::vec3(0.8f));
		pMainProgram->SetUniform("light1.Ls", glm::vec3(0.5f));

		glm::mat4 identBias(1.0f);

		// Skybox
		ms.Push();
			pMainProgram->SetUniform("renderSkybox", true);
			ms.Translate(m_pCamera->GetPosition());
			SetMatrices(pMainProgram, ms.Top(), identBias, identBias);
			m_pSpaceSkybox->Render(cubeMapUnit);
			pMainProgram->SetUniform("renderSkybox", false);
		ms.Pop();

		// Visible sun — bright sphere in the same direction as light1 (-100, 100, -100).
		// Anchored to the camera so it reads as infinitely distant.
		// Lit only by ambient: high Ma, zero Md/Ms, no texture.
		// Plus an additive halo for a god-light bloom.
		{
			glm::vec3 sunDir = glm::normalize(glm::vec3(-100.0f, 100.0f, -100.0f));
			glm::vec3 sunPos = m_pCamera->GetPosition() + sunDir * 600.0f;

			pMainProgram->SetUniform("bUseTexture", false);
			pMainProgram->SetUniform("material1.Md", glm::vec3(0.0f));
			pMainProgram->SetUniform("material1.Ms", glm::vec3(0.0f));
			glDepthMask(GL_FALSE);

			// Halo (additive, large + dim)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			ms.Push();
				ms.Translate(sunPos);
				ms.Scale(140.0f);
				pMainProgram->SetUniform("material1.Ma", glm::vec3(0.45f, 0.38f, 0.20f));
				SetMatrices(pMainProgram, ms.Top(), identBias, identBias);
				m_pSphere->Render();
			ms.Pop();
			glDisable(GL_BLEND);

			// Sun core (opaque, bright)
			ms.Push();
				ms.Translate(sunPos);
				ms.Scale(40.0f);
				pMainProgram->SetUniform("material1.Ma", glm::vec3(3.0f, 2.7f, 1.9f));
				SetMatrices(pMainProgram, ms.Top(), identBias, identBias);
				m_pSphere->Render();
			ms.Pop();

			glDepthMask(GL_TRUE);
			pMainProgram->SetUniform("bUseTexture", true);
		}

		// Delegate all gameplay rendering to TacticalGame
		m_pTacticalGame->Render(m_pCamera, m_pShaderPrograms, m_pFtFont,
		                        m_gameWindow.GetWidth(), m_gameWindow.GetHeight());

		DisplayFrameRate();
		glfwSwapBuffers(m_gameWindow.GetWindow());
		return;
	}

	// Set up a matrix stack
	glutil::MatrixStack modelViewMatrixStack;
	modelViewMatrixStack.SetIdentity();

	// Use the main shader program
	CShaderProgram *pMainProgram = (*m_pShaderPrograms)[0];
	pMainProgram->UseProgram();
	pMainProgram->SetUniform("bUseTexture", true);
	pMainProgram->SetUniform("sampler0", 0);
	int cubeMapTextureUnit = 10;
	pMainProgram->SetUniform("CubeMapTex", cubeMapTextureUnit);

	// Bind shadow maps
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, m_shadowMapTex);
	pMainProgram->SetUniform("shadowMap", 5);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, m_spotShadowTex);
	pMainProgram->SetUniform("spotShadowMap", 6);
	glActiveTexture(GL_TEXTURE0);

	// Set the projection matrix
	pMainProgram->SetUniform("matrices.projMatrix", m_pCamera->GetPerspectiveProjectionMatrix());

	// View matrix
	modelViewMatrixStack.LookAt(m_pCamera->GetPosition(), m_pCamera->GetView(), m_pCamera->GetUpVector());
	glm::mat4 viewMatrix = modelViewMatrixStack.Top();
	glm::mat3 viewNormalMatrix = m_pCamera->ComputeNormalMatrix(viewMatrix);

	// Precompute shadow bias matrices: lightVP * inverse(viewMatrix)
	glm::mat4 lightVP = m_lightProjMatrix * m_lightViewMatrix;
	glm::mat4 viewInverse = glm::inverse(viewMatrix);
	glm::mat4 shadowBias = lightVP * viewInverse;
	glm::mat4 spotVP = m_spotProjMatrix * m_spotViewMatrix;
	glm::mat4 spotShadowBias = spotVP * viewInverse;

	// Set light and materials in main shader program
	glm::vec4 lightPosition1 = glm::vec4(-100, 100, -100, 1);
	pMainProgram->SetUniform("light1.position", viewMatrix*lightPosition1);
	pMainProgram->SetUniform("light1.La", glm::vec3(1.0f));
	pMainProgram->SetUniform("light1.Ld", glm::vec3(1.0f));
	pMainProgram->SetUniform("light1.Ls", glm::vec3(1.0f));

	// Console point lights
	glm::vec3 bo(0.0f, 25.0f, 60.0f);
	glm::vec4 consoleLights[] = {
		glm::vec4(bo + glm::vec3(-2.5f, 1.2f, -1.5f), 1),
		glm::vec4(bo + glm::vec3( 2.5f, 1.2f, -1.5f), 1),
		glm::vec4(bo + glm::vec3(-2.5f, 1.2f,  1.5f), 1),
		glm::vec4(bo + glm::vec3( 2.5f, 1.2f,  1.5f), 1),
	};
	glm::vec3 consoleLa(0.05f, 0.08f, 0.12f);
	glm::vec3 consoleLd(0.8f, 1.2f, 1.6f);
	glm::vec3 consoleLs(0.4f, 0.6f, 0.8f);

	pMainProgram->SetUniform("numPointLights", 4);
	for (int i = 0; i < 4; i++) {
		string prefix = "pointLights[" + to_string(i) + "]";
		pMainProgram->SetUniform(prefix + ".position", viewMatrix * consoleLights[i]);
		pMainProgram->SetUniform(prefix + ".La", consoleLa);
		pMainProgram->SetUniform(prefix + ".Ld", consoleLd);
		pMainProgram->SetUniform(prefix + ".Ls", consoleLs);
	}

	pMainProgram->SetUniform("material1.Ma", glm::vec3(1.0f));
	pMainProgram->SetUniform("material1.Md", glm::vec3(0.0f));
	pMainProgram->SetUniform("material1.Ms", glm::vec3(0.0f));
	pMainProgram->SetUniform("material1.shininess", 15.0f);


	// Render the skybox and terrain with full ambient reflectance
	modelViewMatrixStack.Push();
		pMainProgram->SetUniform("renderSkybox", true);
		glm::vec3 vEye = m_pCamera->GetPosition();
		modelViewMatrixStack.Translate(vEye);
		SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
		// Use deep space skybox for exterior Sobornost shot
		if (m_cutsceneActive && m_cutscenePhase == 4)
			m_pSpaceSkybox->Render(cubeMapTextureUnit);
		else
			m_pSkybox->Render(cubeMapTextureUnit);
		pMainProgram->SetUniform("renderSkybox", false);
	modelViewMatrixStack.Pop();

	// Phase 4: exterior Sobornost fleet shot — skip everything except skybox + ships
	if (m_cutsceneActive && m_cutscenePhase == 4) {
		float ap = m_sobornostApproach;
		float currentTime = m_cutsceneTimer;

		// Bright lighting to make dark hull materials visible against space
		pMainProgram->SetUniform("light1.La", glm::vec3(0.8f));
		pMainProgram->SetUniform("light1.Ld", glm::vec3(1.0f));
		pMainProgram->SetUniform("light1.Ls", glm::vec3(0.6f));

		// Override materials — boost so near-black hull colours are visible
		pMainProgram->SetUniform("bUseTexture", true);
		pMainProgram->SetUniform("material1.Ma", glm::vec3(0.5f));
		pMainProgram->SetUniform("material1.Md", glm::vec3(0.8f));
		pMainProgram->SetUniform("material1.Ms", glm::vec3(0.5f));
		pMainProgram->SetUniform("material1.shininess", 50.0f);

		// Gogol escort carriers — side-on, staggered formation, drifting in from the right
		struct SoborShip { glm::vec3 startPos; glm::vec3 endPos; float scale; float wobbleFreq; };
		SoborShip escorts[] = {
			{ glm::vec3( 40.0f, 26.0f, 100.0f), glm::vec3(-2.0f, 25.5f, 100.0f), 0.3f, 1.3f },
			{ glm::vec3( 55.0f, 24.0f, 115.0f), glm::vec3( 8.0f, 24.5f, 112.0f), 0.225f, 1.7f },
			{ glm::vec3( 48.0f, 27.5f, 105.0f), glm::vec3( 3.0f, 27.0f, 105.0f), 0.27f, 1.1f },
		};

		// Compute escort positions and set up blood-red point lights
		glm::vec3 escortPositions[3];
		int numArrivedLights = 0;
		for (int i = 0; i < 3; i++) {
			escortPositions[i] = glm::mix(escorts[i].startPos, escorts[i].endPos, ap);
			escortPositions[i].y += 0.2f * sinf(currentTime * escorts[i].wobbleFreq + (float)i * 2.0f);
			if (m_shipArrived[i]) numArrivedLights++;
		}

		// Blood-red point lights on arrived escorts
		pMainProgram->SetUniform("numPointLights", numArrivedLights);
		int lightIdx = 0;
		for (int i = 0; i < 3; i++) {
			if (!m_shipArrived[i]) continue;
			string prefix = "pointLights[" + to_string(lightIdx) + "]";
			pMainProgram->SetUniform(prefix + ".position", viewMatrix * glm::vec4(escortPositions[i], 1.0f));
			pMainProgram->SetUniform(prefix + ".La", glm::vec3(0.3f, 0.02f, 0.02f));
			pMainProgram->SetUniform(prefix + ".Ld", glm::vec3(1.0f, 0.08f, 0.05f));
			pMainProgram->SetUniform(prefix + ".Ls", glm::vec3(0.8f, 0.05f, 0.03f));
			lightIdx++;
		}

		for (int i = 0; i < 3; i++) {
			if (!m_shipArrived[i]) continue;

			modelViewMatrixStack.Push();
				modelViewMatrixStack.Translate(escortPositions[i]);
				modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), -1.5708f);
				modelViewMatrixStack.Scale(escorts[i].scale);
				SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
				m_pCruiserMesh->Render();
			modelViewMatrixStack.Pop();
		}

		// Founders warship — only after its hyperjump
		if (m_shipArrived[3]) {
			glm::vec3 warshipStart(900.0f, 27.0f, 1800.0f);
			glm::vec3 warshipEnd(600.0f, 26.0f, 1200.0f);
			glm::vec3 warshipPos = glm::mix(warshipStart, warshipEnd, ap);

			modelViewMatrixStack.Push();
				modelViewMatrixStack.Translate(warshipPos);
				modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 0.52f);
				modelViewMatrixStack.Scale(2.0f);
				SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
				m_pWarmindMesh->Render();
			modelViewMatrixStack.Pop();
		}

		// Render explosion/tear particles
		m_pParticleSystem->Render(viewMatrix, *m_pCamera->GetPerspectiveProjectionMatrix());

		// Screen flash overlay (white additive quad)
		if (m_screenFlash > 0.01f) {
			CShaderProgram *fontProgram = (*m_pShaderPrograms)[1];
			int w = m_gameWindow.GetWidth();
			int h = m_gameWindow.GetHeight();
			m_pCamera->SetOrthographicProjectionMatrix(w, h);
			fontProgram->UseProgram();
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive
			fontProgram->SetUniform("matrices.projMatrix", m_pCamera->GetOrthographicProjectionMatrix());
			fontProgram->SetUniform("matrices.modelViewMatrix", glm::mat4(1));
			// Red flash for warmind (latches once it has arrived), warm white for escorts.
			glm::vec4 flashCol = m_shipArrived[3]
				? glm::vec4(0.85f, 0.15f, 0.08f, m_screenFlash)
				: glm::vec4(1.0f, 0.9f, 0.8f, m_screenFlash);
			fontProgram->SetUniform("vColour", flashCol);
			fontProgram->SetUniform("bFullColour", false);
			glBindVertexArray(m_dialogueVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindSampler(0, 0);
			glBindTexture(GL_TEXTURE_2D, m_whiteTex);
			fontProgram->SetUniform("sampler0", 0);
			glm::mat4 mv = glm::scale(glm::mat4(1.0f), glm::vec3((float)w, (float)h, 1.0f));
			fontProgram->SetUniform("matrices.modelViewMatrix", mv);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			pMainProgram->UseProgram();
		}

		// HUD overlay on top of fleet scene
		RenderHudOverlay(1.0f);

		// Phase 4 dialogue overlay
		if (m_phase4Line < (int)m_phase4Script.size())
			RenderDialogue(m_phase4Script[m_phase4Line].speaker, m_phase4Script[m_phase4Line].text);

		DisplayFrameRate();
		glfwSwapBuffers(m_gameWindow.GetWindow());
		return;
	}

	// Render the planar terrain
	modelViewMatrixStack.Push();
		SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
		m_pPlanarTerrain->Render();
	modelViewMatrixStack.Pop();


	// Turn on diffuse + specular materials
	pMainProgram->SetUniform("material1.Ma", glm::vec3(0.5f));	// Ambient material reflectance
	pMainProgram->SetUniform("material1.Md", glm::vec3(0.5f));	// Diffuse material reflectance
	pMainProgram->SetUniform("material1.Ms", glm::vec3(1.0f));	// Specular material reflectance


	// Render the horse
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(0.0f, 0.0f, 0.0f));
		modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 180.0f);
		modelViewMatrixStack.Scale(2.5f);
		SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
		m_pHorseMesh->Render();
	modelViewMatrixStack.Pop();



	// Render the barrel
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(100.0f, 0.0f, 0.0f));
		modelViewMatrixStack.Scale(5.0f);
		SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
		m_pBarrelMesh->Render();
	modelViewMatrixStack.Pop();

	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(110.0f, 0.0f, 0.0f));
		modelViewMatrixStack.Scale(5.0f);
		SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
		m_pBarrelMesh->Render();
	modelViewMatrixStack.Pop();

	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(120.0f, 0.0f, 0.0f));
		modelViewMatrixStack.Scale(5.0f);
		SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
		m_pBarrelMesh->Render();
	modelViewMatrixStack.Pop();

	// Render the sphere
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(0.0f, 2.0f, 150.0f));
		modelViewMatrixStack.Scale(2.0f);
		SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
		// To turn off texture mapping and use the sphere colour only (currently white material), uncomment the next line
		//pMainProgram->SetUniform("bUseTexture", false);
		m_pSphere->Render();
	modelViewMatrixStack.Pop();

	// --- Cutscene: bridge interior + characters ---
	if (m_cutsceneActive) {

	// Bridge room origin at (0, 25, 60) — inside the ship hull
	glm::vec3 bridgeOrigin(0.0f, 25.0f, 60.0f);

	// Kill sun light inside the bridge — only point lights + viewport spotlight matter
	pMainProgram->SetUniform("light1.La", glm::vec3(0.0f));
	pMainProgram->SetUniform("light1.Ld", glm::vec3(0.0f));
	pMainProgram->SetUniform("light1.Ls", glm::vec3(0.0f));

	// Bridge room
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(bridgeOrigin);
		SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);

		// Floor — hex tile texture
		pMainProgram->SetUniform("bUseTexture", true);
		pMainProgram->SetUniform("material1.Ma", glm::vec3(0.1f));
		pMainProgram->SetUniform("material1.Md", glm::vec3(0.6f));
		pMainProgram->SetUniform("material1.Ms", glm::vec3(0.4f));
		pMainProgram->SetUniform("material1.shininess", 40.0f);
		m_pFloorTex->Bind(0);
		m_pBridge->RenderFloor();

		// Walls + ceiling — sci-fi panel texture
		pMainProgram->SetUniform("material1.Ma", glm::vec3(0.08f, 0.09f, 0.12f));
		pMainProgram->SetUniform("material1.Md", glm::vec3(0.3f, 0.33f, 0.4f));
		pMainProgram->SetUniform("material1.Ms", glm::vec3(0.5f));
		pMainProgram->SetUniform("material1.shininess", 60.0f);
		m_pWallTex->Bind(0);
		m_pBridge->RenderWalls();

		// Viewport wall rendered later with alpha blending
	modelViewMatrixStack.Pop();

	// Reset material for furniture
	pMainProgram->SetUniform("bUseTexture", true);
	pMainProgram->SetUniform("material1.Ma", glm::vec3(0.5f));
	pMainProgram->SetUniform("material1.Md", glm::vec3(0.5f));
	pMainProgram->SetUniform("material1.Ms", glm::vec3(1.0f));
	pMainProgram->SetUniform("material1.shininess", 15.0f);

	// Chair 1 — Mieli's
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(bridgeOrigin + glm::vec3(1.0f, 0.0f, -2.2f));
		modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), -0.2f);
		modelViewMatrixStack.Scale(0.5f);
		SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
		m_pChairMesh->Render();
	modelViewMatrixStack.Pop();

	// Chair 2 — Jean's
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(bridgeOrigin + glm::vec3(-1.0f, 0.0f, -2.2f));
		modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 3.2f);
		modelViewMatrixStack.Scale(0.5f);
		SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
		m_pChairMesh->Render();
	modelViewMatrixStack.Pop();

	// Monitoring stations — render solid then additive for screen glow
	auto renderMonitor = [&](glm::vec3 offset, float rotY) {
		modelViewMatrixStack.Push();
			modelViewMatrixStack.Translate(bridgeOrigin + offset);
			if (rotY != 0.0f)
				modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), rotY);
			modelViewMatrixStack.Scale(0.5f);
			SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
			m_pMonitorMesh->Render();
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			glDepthMask(GL_FALSE);
			m_pMonitorMesh->Render();
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
		modelViewMatrixStack.Pop();
	};
	// Left wall consoles
	renderMonitor(glm::vec3(-2.5f, 0.0f, -1.5f), 0.0f);
	renderMonitor(glm::vec3(-2.5f, 0.0f, 1.5f), 0.0f);
	// Right wall consoles
	renderMonitor(glm::vec3(2.5f, 0.0f, -1.5f), 3.14159f);
	renderMonitor(glm::vec3(2.5f, 0.0f, 1.5f), 3.14159f);

	// Table between the chairs
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(bridgeOrigin + glm::vec3(0.0f, 0.0f, 1.0f));
		modelViewMatrixStack.Scale(0.5f);
		SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
		m_pTableMesh->Render();
	modelViewMatrixStack.Pop();

	// --- Render Jean (animated character) ---
	{
		CShaderProgram *pSkinProg = (*m_pShaderPrograms)[4];
		pSkinProg->UseProgram();
		pSkinProg->SetUniform("matrices.projMatrix", m_pCamera->GetPerspectiveProjectionMatrix());
		pSkinProg->SetUniform("sampler0", 0);
		pSkinProg->SetUniform("bUseTexture", true);

		pSkinProg->SetUniform("light1.position", viewMatrix * lightPosition1);
		pSkinProg->SetUniform("light1.La", glm::vec3(0.0f));
		pSkinProg->SetUniform("light1.Ld", glm::vec3(0.0f));
		pSkinProg->SetUniform("light1.Ls", glm::vec3(0.0f));
		for (int i = 0; i < 4; i++) {
			string prefix = "light" + to_string(i + 2);
			pSkinProg->SetUniform(prefix + ".position", viewMatrix * consoleLights[i]);
			pSkinProg->SetUniform(prefix + ".La", consoleLa);
			pSkinProg->SetUniform(prefix + ".Ld", consoleLd);
			pSkinProg->SetUniform(prefix + ".Ls", consoleLs);
		}
		pSkinProg->SetUniform("numLights", 5);
		pSkinProg->SetUniform("shadowMap", 5);
		pSkinProg->SetUniform("spotShadowMap", 6);
		pSkinProg->SetUniform("material1.Ma", glm::vec3(0.5f));
		pSkinProg->SetUniform("material1.Md", glm::vec3(0.5f));
		pSkinProg->SetUniform("material1.Ms", glm::vec3(1.0f));
		pSkinProg->SetUniform("material1.shininess", 15.0f);

		// Upload bone matrices
		pSkinProg->SetUniform("boneMatrices", m_pJean->GetBoneMatrices(), m_pJean->GetNumBones());

		// Jean — position and facing based on cutscene phase
		// Phase 0: face each other. Phase 1-4: face viewport. Phase 5: face Chen (inward)
		float jeanRotY = (m_cutscenePhase == 5) ? 3.14f : (m_cutscenePhase >= 1) ? 0.0f : 1.56f;
		modelViewMatrixStack.Push();
			modelViewMatrixStack.Translate(bridgeOrigin + m_jeanPos);
			modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), jeanRotY);
			modelViewMatrixStack.Scale(0.005f);
			SetMatrices(pSkinProg, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
			m_pJean->Render();
		modelViewMatrixStack.Pop();

		// Mieli
		float mieliRotY = (m_cutscenePhase == 5) ? 3.14f : (m_cutscenePhase >= 1) ? 0.0f : -1.56f;
		pSkinProg->SetUniform("boneMatrices", m_pMieli->GetBoneMatrices(), m_pMieli->GetNumBones());
		modelViewMatrixStack.Push();
			modelViewMatrixStack.Translate(bridgeOrigin + m_mieliPos);
			modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), mieliRotY);
			modelViewMatrixStack.Scale(0.05f);
			SetMatrices(pSkinProg, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
			m_pMieli->Render();
		modelViewMatrixStack.Pop();

		// Chen — holographic projection, only during phase 5 monologue
		if (m_cutscenePhase == 5) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive for hologram glow
		glDepthMask(GL_FALSE);

		// Tint the hologram cyan-blue
		pSkinProg->SetUniform("material1.Ma", glm::vec3(0.05f, 0.15f, 0.3f));
		pSkinProg->SetUniform("material1.Md", glm::vec3(0.1f, 0.4f, 0.8f));
		pSkinProg->SetUniform("material1.Ms", glm::vec3(0.2f, 0.5f, 1.0f));

		pSkinProg->SetUniform("boneMatrices", m_pChen->GetBoneMatrices(), m_pChen->GetNumBones());
		modelViewMatrixStack.Push();
			modelViewMatrixStack.Translate(bridgeOrigin + glm::vec3(0.0f, 0.0f, 0.0f));
			modelViewMatrixStack.Scale(0.005f);
			SetMatrices(pSkinProg, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
			m_pChen->Render();
		modelViewMatrixStack.Pop();

		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		// Restore material for next objects
		pSkinProg->SetUniform("material1.Ma", glm::vec3(0.5f));
		pSkinProg->SetUniform("material1.Md", glm::vec3(0.5f));
		pSkinProg->SetUniform("material1.Ms", glm::vec3(1.0f));
		} // end Chen phase 5 only

		pMainProgram->UseProgram();
	}

	// Front wall — viewport.png frame + live FBO in the window opening
	if (m_cutsceneActive) {
		pMainProgram->UseProgram();
		modelViewMatrixStack.Push();
			glm::vec3 bridgeOriginVP(0.0f, 25.0f, 60.0f);
			modelViewMatrixStack.Translate(bridgeOriginVP);
			SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
			pMainProgram->SetUniform("bUseTexture", true);
			pMainProgram->SetUniform("material1.Ma", glm::vec3(1.0f));
			pMainProgram->SetUniform("material1.Md", glm::vec3(0.0f));
			pMainProgram->SetUniform("material1.Ms", glm::vec3(0.0f));
			pMainProgram->SetUniform("material1.shininess", 1.0f);

			// Temporarily restore sun light so viewport textures render at full brightness
			pMainProgram->SetUniform("light1.La", glm::vec3(1.0f));

			// Full wall with viewport.png (frame artwork)
			m_pViewportTex->Bind(0);
			m_pBridge->RenderMirrorWall();

			// Live view rendered on top of the window opening
			m_pViewportFBO->BindTexture(0);
			m_pBridge->RenderViewport();

			// Kill sun light again for remaining bridge objects
			pMainProgram->SetUniform("light1.La", glm::vec3(0.0f));
		modelViewMatrixStack.Pop();
	}

	} // end cutscene block

	// Restore sun light for exterior
	pMainProgram->SetUniform("light1.La", glm::vec3(1.0f));
	pMainProgram->SetUniform("light1.Ld", glm::vec3(1.0f));
	pMainProgram->SetUniform("light1.Ls", glm::vec3(1.0f));

	// --- Render ship in three passes: hull, thrust, sails ---

	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(0.0f, 25.0f, 50.0f));
		modelViewMatrixStack.Scale(3.0f);

		glm::mat4 shipMV = modelViewMatrixStack.Top();
		glm::mat3 shipNM = m_pCamera->ComputeNormalMatrix(shipMV);

		// Pass 1: Hull + nacelles with neon glow shader
		CShaderProgram *pHullProgram = (*m_pShaderPrograms)[3];
		pHullProgram->UseProgram();
		pHullProgram->SetUniform("matrices.projMatrix", m_pCamera->GetPerspectiveProjectionMatrix());
		pHullProgram->SetUniform("sampler0", 0);
		pHullProgram->SetUniform("charge", m_shipCharge);

		pHullProgram->SetUniform("light1.position", viewMatrix * lightPosition1);
		pHullProgram->SetUniform("light1.La", glm::vec3(1.0f));
		pHullProgram->SetUniform("light1.Ld", glm::vec3(1.0f));
		pHullProgram->SetUniform("light1.Ls", glm::vec3(1.0f));

		pHullProgram->SetUniform("material1.Ma", glm::vec3(0.15f, 0.15f, 0.2f));
		pHullProgram->SetUniform("material1.Md", glm::vec3(0.6f, 0.6f, 0.7f));
		pHullProgram->SetUniform("material1.Ms", glm::vec3(1.0f, 1.0f, 1.2f));
		pHullProgram->SetUniform("material1.shininess", 80.0f);
		pHullProgram->SetUniform("bUseTexture", true);

		pHullProgram->SetUniform("matrices.modelViewMatrix", shipMV);
		pHullProgram->SetUniform("matrices.normalMatrix", shipNM);
		m_pShip->RenderHull();

		// Pass 2: Ion thrust — always on, depth scales with charge
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive
			glDepthMask(GL_FALSE);
			glDisable(GL_CULL_FACE);

			// Scale thrust length: pin cone base at exhaust plane, stretch tip by charge
			// Exhaust Z in model space: centerZ - nacelleLength/2 = -2.5 - 1.75 = -4.25
			float exhaustZ = -7.35f;
			float thrustScale = 0.1f + 0.9f * m_shipCharge; // 10% idle, 100% at full charge
			glm::mat4 thrustMV = shipMV
				* glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, exhaustZ))
				* glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, thrustScale))
				* glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -exhaustZ));

			pHullProgram->SetUniform("bUseTexture", false);
			pHullProgram->SetUniform("material1.Ma", glm::vec3(0.0f));
			// Brightness scales with charge: dim idle flicker → bright full burn
			float intensity = 0.15f + 0.85f * m_shipCharge;
			pHullProgram->SetUniform("material1.Md", intensity * glm::vec3(0.5f, 0.8f, 1.0f));
			pHullProgram->SetUniform("material1.Ms", glm::vec3(0.0f));

			pHullProgram->SetUniform("matrices.modelViewMatrix", thrustMV);
			pHullProgram->SetUniform("matrices.normalMatrix",
				m_pCamera->ComputeNormalMatrix(thrustMV));
			m_pShip->RenderThrust();

			glEnable(GL_CULL_FACE);
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
		}

		// Pass 3: Solar sails — only render when visibly unfurled
		if (m_sailUnfurl > 0.01f) {
			CShaderProgram *pSailProgram = (*m_pShaderPrograms)[2];
			pSailProgram->UseProgram();
			pSailProgram->SetUniform("matrices.projMatrix", m_pCamera->GetPerspectiveProjectionMatrix());
			pSailProgram->SetUniform("sampler0", 0);
			pSailProgram->SetUniform("unfurl", m_sailUnfurl);

			pSailProgram->SetUniform("light1.position", viewMatrix * lightPosition1);
			pSailProgram->SetUniform("light1.La", glm::vec3(1.0f));
			pSailProgram->SetUniform("light1.Ld", glm::vec3(1.0f));
			pSailProgram->SetUniform("light1.Ls", glm::vec3(1.0f));

			pSailProgram->SetUniform("material1.Ma", glm::vec3(0.3f, 0.3f, 0.35f));
			pSailProgram->SetUniform("material1.Md", glm::vec3(1.0f, 1.0f, 1.0f));
			pSailProgram->SetUniform("material1.Ms", glm::vec3(1.5f, 1.5f, 1.5f));
			pSailProgram->SetUniform("material1.shininess", 200.0f);
			pSailProgram->SetUniform("bUseTexture", true);

			pSailProgram->SetUniform("matrices.modelViewMatrix", shipMV);
			pSailProgram->SetUniform("matrices.normalMatrix", shipNM);
			m_pShip->RenderSails();
		}

	modelViewMatrixStack.Pop();


	// Switch back to main shader for subsequent rendering
	pMainProgram->UseProgram();

	// Render the track — two-sided since the camera views it from varying angles
	glDisable(GL_CULL_FACE);
	modelViewMatrixStack.Push();
		SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
		pMainProgram->SetUniform("bUseTexture", true);
		m_pCatmullRom->RenderTrack();
	modelViewMatrixStack.Pop();
	glEnable(GL_CULL_FACE);

	// Draw the 2D graphics after the 3D graphics
	if (m_cutsceneActive && m_cutscenePhase == 0 && m_dialogueLine < (int)m_dialogueScript.size())
		RenderDialogue(m_dialogueScript[m_dialogueLine].speaker, m_dialogueScript[m_dialogueLine].text);
	if (m_cutsceneActive && m_cutscenePhase == 5 && m_chenLine < (int)m_chenScript.size())
		RenderDialogue(m_chenScript[m_chenLine].speaker, m_chenScript[m_chenLine].text);
	DisplayFrameRate();

	// Swap buffers to show the rendered image
	glfwSwapBuffers(m_gameWindow.GetWindow());

}

// Update method runs repeatedly with the Render method
void Game::Update()
{

	GLFWwindow *win = m_gameWindow.GetWindow();

	// F2 toggles free camera for testing
	static bool f2WasPressed = false;
	bool f2Pressed = glfwGetKey(win, GLFW_KEY_F2) == GLFW_PRESS;
	static bool freeCamOverride = false;
	if (f2Pressed && !f2WasPressed)
		freeCamOverride = !freeCamOverride;
	f2WasPressed = f2Pressed;

	if (m_cutsceneActive && !freeCamOverride) {
		glm::vec3 bo(0.0f, 25.0f, 60.0f);
		if (m_cutscenePhase == 0) {
			// Dialogue: over-the-shoulder behind Jean, looking at Mieli
			glm::vec3 camPos = bo + m_jeanPos + glm::vec3(-0.8f, 0.9f, -0.6f);
			glm::vec3 lookAt = bo + m_mieliPos + glm::vec3(0.0f, 0.5f, 0.0f);
			m_pCamera->Set(camPos, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));
		} else if (m_cutscenePhase == 4) {
			// Exterior shot: camera far back, looking at Sobornost fleet side-on
			float dollyT = m_cutsceneTimer / 30.0f; // slow continuous dolly
			glm::vec3 camStart(-40.0f, 28.0f, 60.0f);
			glm::vec3 camEnd(-45.0f, 27.0f, 65.0f);
			glm::vec3 camPos = glm::mix(camStart, camEnd, dollyT);
			glm::vec3 lookAt(20.0f, 25.0f, 150.0f);
			// Screen shake — rotation-based so skybox shakes too
			if (m_screenShake > 0.01f) {
				float maxAngle = glm::radians(m_screenShake * 6.0f); // up to 6 degrees
				m_shakeAngleTarget = glm::vec2(
					maxAngle * ((float)(rand() % 200 - 100) / 100.0f),
					maxAngle * ((float)(rand() % 200 - 100) / 100.0f)
				);
				m_shakeAngle = glm::mix(m_shakeAngle, m_shakeAngleTarget, 0.3f);
				glm::vec3 forward = glm::normalize(lookAt - camPos);
				glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
				glm::vec3 up = glm::cross(right, forward);
				glm::mat4 rot = glm::rotate(glm::mat4(1.0f), m_shakeAngle.x, right);
				rot = glm::rotate(rot, m_shakeAngle.y, up);
				lookAt = camPos + glm::vec3(rot * glm::vec4(forward, 0.0f)) * glm::length(lookAt - camPos);
			} else {
				m_shakeAngle = glm::vec2(0.0f);
			}
			m_pCamera->Set(camPos, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));
		} else if (m_cutscenePhase == 5) {
			// Chen monologue: camera from viewport side, looking back into bridge
			glm::vec3 camPos = bo + glm::vec3(-3.0f, 1.0f, 3.5f);   // viewport side
			glm::vec3 lookAt = bo + glm::vec3(2.0f, 0.0f, 0.0f);   // looking at Chen center
			m_pCamera->Set(camPos, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));
		} else {
			// Walk/viewport: follow from behind, looking toward the viewport
			glm::vec3 midpoint = (m_jeanPos + m_mieliPos) * 0.5f;
			glm::vec3 camPos = bo + midpoint + glm::vec3(0.0f, 1.2f, -3.0f);
			glm::vec3 lookAt = bo + midpoint + glm::vec3(0.0f, 0.5f, 2.0f);
			m_pCamera->Set(camPos, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));
		}
	} else {
		// Free camera — WASD + mouse
		m_pCamera->Update(m_dt);
	}

	// Mode toggle: TAB switches between mode 1 (idle/cruise) and mode 2 (combat)
	// Use a simple edge-detect to avoid rapid toggling
	static bool tabWasPressed = false;
	bool tabPressed = glfwGetKey(win, GLFW_KEY_TAB) == GLFW_PRESS;
	if (tabPressed && !tabWasPressed)
		m_shipMode = (m_shipMode == 1) ? 2 : 1;
	tabWasPressed = tabPressed;

	float dt_s = (float)(m_dt / 1000.0);

	if (m_shipMode == 1) {
		// Idle/cruise: sails unfurl, ship charges from solar energy
		m_sailUnfurl = glm::min(m_sailUnfurl + 0.4f * dt_s, 1.0f);
		// Charge proportional to how unfurled the sails are
		m_shipCharge = glm::min(m_shipCharge + m_sailUnfurl * 0.3f * dt_s, 1.0f);
	} else {
		// Combat: sails retract, charge drains into thrust/combat systems
		m_sailUnfurl = glm::max(m_sailUnfurl - 0.6f * dt_s, 0.0f);
		m_shipCharge = glm::max(m_shipCharge - 0.15f * dt_s, 0.0f);
	}

	// Periodic bridge ambience — plays during bridge scenes (title, phases 0-2, 5)
	bool onBridge = m_titleScreen || (m_cutsceneActive && (m_cutscenePhase <= 2 || m_cutscenePhase == 5));
	if (onBridge) {
		m_bridgeSoundTimer -= dt_s;
		if (m_bridgeSoundTimer <= 0.0f) {
			string bridgeSounds[] = {"bridge1", "bridge2", "bridge3"};
			m_pAudio->PlaySound(bridgeSounds[rand() % 3], 0.4f);
			m_bridgeSoundTimer = 10.0f;
		}
	}

	// Cutscene phase logic
	if (m_cutsceneActive) {

		// --- Audio triggers for phase 0 dialogue ---
		if (m_cutscenePhase == 0 && m_dialogueLine != m_lastAudioLine0) {
			m_lastAudioLine0 = m_dialogueLine;
			if (m_dialogueLine == 0)
				m_pAudio->PlaySound("tracking", 0.8f);  // Pellegrini: "We are tracking..."
		}

		// --- Audio triggers for phase 4 dialogue ---
		if (m_cutscenePhase == 4 && m_phase4Line != m_lastAudioLine4) {
			m_lastAudioLine4 = m_phase4Line;
			if (m_phase4Line < (int)m_phase4Script.size() &&
				m_phase4Script[m_phase4Line].speaker == "Pellegrini")
				m_pAudio->PlaySound("towar", 0.8f);  // "At last, to war"
		}

		// --- Audio triggers for phase 5 (Chen monologue) ---
		if (m_cutscenePhase == 5 && m_chenLine != m_lastAudioLine5) {
			m_lastAudioLine5 = m_chenLine;
			if (m_chenLine < (int)m_chenScript.size()) {
				const string& speaker = m_chenScript[m_chenLine].speaker;
				const string& text = m_chenScript[m_chenLine].text;
				if (speaker == "Pellegrini" && text.find("Incoming") != string::npos) {
					m_pAudio->PlaySound("incoming", 0.8f);
				} else if (speaker == "Chen") {
					// Play matching voice clip + hologram SFX
					int holoIdx = rand() % 3;
					string holoName = "hologram" + to_string(holoIdx + 1);
					m_pAudio->PlaySound(holoName, 0.5f);

					if (text.find("conclave") != string::npos)
						m_pAudio->PlaySound("conclave", 0.9f);
					else if (text.find("Shadows") != string::npos)
						m_pAudio->PlaySound("shadows", 0.9f);
					else if (text.find("bring war") != string::npos)
						m_pAudio->PlaySound("bringwar", 0.9f);
					else if (text.find("commenced") != string::npos)
						m_pAudio->PlaySound("commenced", 0.9f);
				}
			}
		}

		if (m_cutscenePhase == 0 && m_dialogueLine >= (int)m_dialogueScript.size()) {
			// Dialogue finished — start walk phase
			m_cutscenePhase = 1;
			m_cutsceneTimer = 0.0f;
			m_pJean->SetAnimation("walk");
			m_pJean->SetInPlace(true);
			m_pMieli->SetAnimation("walking");
			m_pMieli->SetInPlace(true);
		}

		if (m_cutscenePhase == 1) {
			m_cutsceneTimer += dt_s;
			float walkDuration = 4.0f;
			float t = glm::clamp(m_cutsceneTimer / walkDuration, 0.0f, 1.0f);

			glm::vec3 jeanStart(-1.0f, 0.0f, -2.2f);
			glm::vec3 jeanEnd(-0.5f, 0.0f, 3.0f);
			glm::vec3 mieliStart(1.0f, 0.0f, -2.2f);
			glm::vec3 mieliEnd(0.5f, 0.0f, 3.0f);

			m_jeanPos = glm::mix(jeanStart, jeanEnd, t);
			m_mieliPos = glm::mix(mieliStart, mieliEnd, t);

			if (t >= 1.0f) {
				m_cutscenePhase = 2;
				m_cutsceneTimer = 0.0f;
				m_pJean->SetAnimation("idle");
				m_pJean->SetInPlace(false);
				m_pMieli->SetAnimation("idle");
				m_pMieli->SetInPlace(false);
			}
		}

		// Phase 2: brief pause at viewport, then cut to black
		if (m_cutscenePhase == 2) {
			m_cutsceneTimer += dt_s;
			if (m_cutsceneTimer >= 1.5f) {
				m_cutscenePhase = 3;
				m_cutsceneTimer = 0.0f;
			}
		}

		// Phase 2→3 transition: stop bridge ambience, play alert
		if (m_cutscenePhase == 3 && m_cutsceneTimer == 0.0f) {
			m_pAudio->PlaySound("alert", 0.7f);
		}

		// Phase 3: bridge + diffuse-fleet FBO — HUD fades in over the bridge view
		if (m_cutscenePhase == 3) {
			m_cutsceneTimer += dt_s;
			const float phase3Duration = 3.0f;
			m_hudBrightness = glm::clamp(m_cutsceneTimer / 1.5f, 0.0f, 1.0f);

			// Advance HUD animation frames during fade-in
			m_hudFrameTimer += dt_s;
			if (m_hudFrameTimer >= 1.0f / HUD_FPS) {
				m_hudFrameTimer -= 1.0f / HUD_FPS;
				m_hudFrameIndex = (m_hudFrameIndex + 1) % HUD_TOTAL_FRAMES;
			}

			if (m_cutsceneTimer >= phase3Duration) {
				m_cutscenePhase = 4;
				m_cutsceneTimer = 0.0f;
				m_pAudio->PlaySound("shipflight", 0.4f);
			}
		}

		// Phase 4: exterior shot — Sobornost fleet keeps moving
		if (m_cutscenePhase == 4) {
			m_cutsceneTimer += dt_s;
			m_sobornostApproach = m_cutsceneTimer / 10.0f;

			// Staggered hyperjump arrivals — compute positions matching Render
			float arrivalTimes[] = { 1.0f, 2.5f, 4.0f, 6.0f };
			float ap = m_sobornostApproach;

			// Same escort data as Render
			glm::vec3 escortStarts[] = {
				glm::vec3( 40.0f, 26.0f, 100.0f),
				glm::vec3( 55.0f, 24.0f, 115.0f),
				glm::vec3( 48.0f, 27.5f, 105.0f),
			};
			glm::vec3 escortEnds[] = {
				glm::vec3(-2.0f, 25.5f, 100.0f),
				glm::vec3( 8.0f, 24.5f, 112.0f),
				glm::vec3( 3.0f, 27.0f, 105.0f),
			};

			for (int i = 0; i < 3; i++) {
				if (!m_shipArrived[i] && m_cutsceneTimer >= arrivalTimes[i]) {
					m_shipArrived[i] = true;
					glm::vec3 pos = glm::mix(escortStarts[i], escortEnds[i], ap);
					glm::vec3 warpBlue(0.3f, 0.5f, 1.0f);
					glm::vec3 warpWhite(0.8f, 0.85f, 1.0f);
					// Cruisers spawn close to camera (z~100), so keep particles small
					// (sizes scaled down ~3x from the warmind, which is ~12x further away).
					m_pParticleSystem->Spawn(pos, 40, warpWhite, 4.0f, 10.0f, 30.0f, 80.0f, 0.6f, 1.4f);
					m_pParticleSystem->Spawn(pos, 30, warpBlue,  3.0f,  8.0f, 50.0f, 120.0f, 0.4f, 1.0f);
					m_screenFlash = 0.2f;
					m_screenShake = 0.5f;
					m_pAudio->PlaySound("warpin", 0.6f);
				}
			}
			// Warmind arrival — massive blood-red explosion
			if (!m_shipArrived[3] && m_cutsceneTimer >= arrivalTimes[3]) {
				m_shipArrived[3] = true;
				m_pAudio->PlaySound("warpin", 1.0f);
				glm::vec3 wStart(900.0f, 27.0f, 1800.0f);
				glm::vec3 wEnd(600.0f, 26.0f, 1200.0f);
				glm::vec3 wPos = glm::mix(wStart, wEnd, ap);
				glm::vec3 hotWhite(1.0f, 0.9f, 0.7f);
				glm::vec3 hotOrange(1.0f, 0.4f, 0.1f);
				glm::vec3 bloodRed(0.8f, 0.05f, 0.02f);
				glm::vec3 darkRed(0.5f, 0.0f, 0.0f);
				// Hot bloom core — bright white/orange center like cruiser warp flashes
				m_pParticleSystem->Spawn(wPos, 80, hotWhite, 40.0f, 100.0f, 20.0f, 60.0f, 1.0f, 2.5f);
				m_pParticleSystem->Spawn(wPos, 60, hotOrange, 25.0f, 70.0f, 30.0f, 80.0f, 1.5f, 3.0f);
				// Core burst — massive particles to match distant warship scale
				m_pParticleSystem->Spawn(wPos, 180, bloodRed, 30.0f, 80.0f, 40.0f, 120.0f, 2.5f, 5.0f);
				// Outer shrapnel ring
				m_pParticleSystem->Spawn(wPos, 120, darkRed, 15.0f, 50.0f, 80.0f, 200.0f, 1.5f, 3.5f);
				// Spread points — wide offsets to surround the ship
				m_pParticleSystem->Spawn(wPos + glm::vec3( 150, 0, 0),  50, bloodRed, 20.0f, 60.0f, 50.0f, 150.0f, 2.0f, 4.0f);
				m_pParticleSystem->Spawn(wPos + glm::vec3(-150, 0, 0),  50, bloodRed, 20.0f, 60.0f, 50.0f, 150.0f, 2.0f, 4.0f);
				m_pParticleSystem->Spawn(wPos + glm::vec3(0,  80, 0),   50, darkRed,  20.0f, 50.0f, 60.0f, 160.0f, 1.5f, 3.5f);
				m_pParticleSystem->Spawn(wPos + glm::vec3(0, -60, 0),   50, darkRed,  20.0f, 50.0f, 60.0f, 160.0f, 1.5f, 3.5f);
				m_pParticleSystem->Spawn(wPos + glm::vec3(0, 0,  120),  40, bloodRed, 20.0f, 50.0f, 50.0f, 140.0f, 2.0f, 4.5f);
				m_pParticleSystem->Spawn(wPos + glm::vec3(0, 0, -120),  40, bloodRed, 20.0f, 50.0f, 50.0f, 140.0f, 2.0f, 4.5f);
				m_screenFlash = 0.85f;
				m_screenShake = 1.0f;
			}

			// Fade screen flash and shake — slower for warmind
			float flashDecay = m_shipArrived[3] ? 0.5f : 1.5f;
			float shakeDecay = m_shipArrived[3] ? 0.6f : 2.0f;
			if (m_screenFlash > 0.0f)
				m_screenFlash = glm::max(m_screenFlash - dt_s * flashDecay, 0.0f);
			if (m_screenShake > 0.0f)
				m_screenShake = glm::max(m_screenShake - dt_s * shakeDecay, 0.0f);

			// Update particles
			m_pParticleSystem->Update(dt_s);

			// Keep advancing HUD frames (looping)
			m_hudFrameTimer += dt_s;
			if (m_hudFrameTimer >= 1.0f / HUD_FPS) {
				m_hudFrameTimer -= 1.0f / HUD_FPS;
				m_hudFrameIndex = (m_hudFrameIndex + 1) % HUD_TOTAL_FRAMES;
			}

			// Transition to Chen monologue when phase 4 dialogue is exhausted
			if (m_phase4Line >= (int)m_phase4Script.size()) {
				m_cutscenePhase = 5;
				m_cutsceneTimer = 0.0f;
				m_chenLine = 0;
			}
		}

		// Phase 5: Chen hologram monologue on bridge
		if (m_cutscenePhase == 5) {
			m_cutsceneTimer += dt_s;

			// Transition to escape when Chen finishes
			if (m_chenLine >= (int)m_chenScript.size()) {
				m_cutscenePhase = 6;
				m_cutsceneActive = false;
				m_cutsceneTimer = 0.0f;
				m_escapeTimer = 60.0f;
				m_lateralOffset = 0.0f;
				m_shipSpeed = 30.0f;
				m_gameOver = false;
				m_escaped = false;
				m_shipMode = 2;
				m_shipCharge = 0.8f;
				m_hitCooldown = 0.0f;
				m_hitTimer = 3.0f;
				// Stop bridge ambience
				m_pAudio->StopSound("bridge1");
				// Start tactical escape game
				m_pTacticalGame->StartEscape();
			}
		}
	}

	// Phase 6: tactical escape gameplay
	if (m_cutscenePhase == 6) {
		m_pTacticalGame->Update(dt_s, m_gameWindow.GetWindow());
		if (!freeCamOverride)
			m_pTacticalGame->UpdateCamera(m_pCamera);
		else
			m_pCamera->Set(m_pCamera->GetPosition(), m_pCamera->GetView(), glm::vec3(0,1,0));
		m_gameOver = m_pTacticalGame->IsGameOver();
		m_escaped = m_pTacticalGame->HasEscaped();
	}

	// Update animated meshes
	if (m_cutscenePhase < 6) {
		m_pJean->Update(dt_s * 0.7f);
		m_pMieli->Update(dt_s * 0.7f);
		// Chen: play salute, hold last frame for 10s, then restart
		{
			float dur = m_pChen->GetAnimationDuration("salute");
			float t = m_pChen->GetCurrentTime();
			if (t >= dur) {
				// Hold at last frame; accumulate pause in timer
				static float chenPause = 0.0f;
				chenPause += dt_s;
				if (chenPause >= 10.0f) {
					m_pChen->SetCurrentTime(0.0f);
					chenPause = 0.0f;
				}
			} else {
				m_pChen->Update(dt_s * 0.7f);
			}
		}
	}

	m_pAudio->Update();
}



void Game::RenderDialogue(const string& speaker, const string& text)
{
	CShaderProgram *fontProgram = (*m_pShaderPrograms)[1];
	int width = m_gameWindow.GetWidth();
	int height = m_gameWindow.GetHeight();
	m_pCamera->SetOrthographicProjectionMatrix(width, height);

	fontProgram->UseProgram();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	fontProgram->SetUniform("matrices.projMatrix", m_pCamera->GetOrthographicProjectionMatrix());

	// --- Character color ---
	glm::vec4 nameColour;
	if (speaker == "Jean")
		nameColour = glm::vec4(1.0f, 0.84f, 0.0f, 1.0f);    // gold
	else if (speaker == "Mieli")
		nameColour = glm::vec4(0.4f, 0.8f, 1.0f, 1.0f);      // ice blue
	else if (speaker == "Pellegrini")
		nameColour = glm::vec4(0.85f, 0.3f, 0.85f, 1.0f);    // magenta
	else if (speaker == "Perhonen")
		nameColour = glm::vec4(0.3f, 1.0f, 0.6f, 1.0f);      // soft green (ship AI)
	else if (speaker == "Chen")
		nameColour = glm::vec4(0.9f, 0.15f, 0.15f, 1.0f);    // blood red
	else
		nameColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// --- Layout ---
	int boxMarginX = width / 20;
	int boxWidth = width - 2 * boxMarginX;
	int boxHeight = height / 7;
	int boxY = boxMarginX / 4;  // near bottom edge
	int textX = boxMarginX + 30;
	int textSize = height / 40;
	int nameSize = textSize + 4;
	int nameTagW = m_pFtFont->GetTextWidth(speaker, nameSize) + 40;
	int nameTagH = nameSize + 16;
	int nameTagX = boxMarginX;
	int nameTagY = boxY + boxHeight;  // sits on top of the box

	// --- Bind quad resources ---
	glBindVertexArray(m_dialogueVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindSampler(0, 0);
	glBindTexture(GL_TEXTURE_2D, m_whiteTex);
	fontProgram->SetUniform("sampler0", 0);
	fontProgram->SetUniform("bFullColour", false);

	auto drawBox = [&](int x, int y, int w, int h, glm::vec4 col) {
		fontProgram->SetUniform("vColour", col);
		glm::mat4 mv = glm::translate(glm::mat4(1.0f), glm::vec3(float(x), float(y), 0.0f));
		mv = glm::scale(mv, glm::vec3(float(w), float(h), 1.0f));
		fontProgram->SetUniform("matrices.modelViewMatrix", mv);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	};

	// Main box — dark, semi-transparent
	drawBox(boxMarginX, boxY, boxWidth, boxHeight, glm::vec4(0.05f, 0.05f, 0.08f, 0.8f));

	// --- Portrait beside the name tag ---
	CTexture *portrait = NULL;
	if (speaker == "Jean") portrait = m_portraitJean;
	else if (speaker == "Mieli") portrait = m_portraitMieli;
	else if (speaker == "Pellegrini") portrait = m_portraitPellegrini;
	else if (speaker == "Perhonen") portrait = m_portraitPerhonen;
	else if (speaker == "Chen") portrait = m_portraitChen;

	int portraitSize = (int)((nameTagH + boxHeight / 3) * 1.8f);
	int borderW = 4;
	int portraitX = boxMarginX - borderW;
	int portraitY = boxY + boxHeight;

	if (portrait && portrait->GetWidth() > 0) {
		// Outer frame — speaker color
		drawBox(portraitX - borderW, portraitY - borderW,
			portraitSize + 2 * borderW, portraitSize + 2 * borderW, nameColour);
		// Inner dark mat
		drawBox(portraitX - 1, portraitY - 1, portraitSize + 2, portraitSize + 2,
			glm::vec4(0.05f, 0.05f, 0.08f, 1.0f));

		// Portrait image
		fontProgram->SetUniform("bFullColour", true);
		fontProgram->SetUniform("vColour", glm::vec4(1.0f));
		glBindVertexArray(m_dialogueVAO);
		portrait->Bind(0);  // sampler has GL_LINEAR set during init
		glm::mat4 mv = glm::translate(glm::mat4(1.0f), glm::vec3(float(portraitX), float(portraitY), 0.0f));
		mv = glm::scale(mv, glm::vec3(float(portraitSize), float(portraitSize), 1.0f));
		fontProgram->SetUniform("matrices.modelViewMatrix", mv);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// Name tag overlaid on bottom of portrait
		fontProgram->SetUniform("bFullColour", false);
		glBindSampler(0, 0);
		glBindTexture(GL_TEXTURE_2D, m_whiteTex);
		drawBox(portraitX, portraitY, portraitSize, nameTagH,
			glm::vec4(0.0f, 0.0f, 0.0f, 0.6f));
		// Accent strip at top of overlay
		drawBox(portraitX, portraitY + nameTagH - 3, portraitSize, 3, nameColour);

		// Name text centered on overlay
		fontProgram->SetUniform("vColour", glm::vec4(1.0f));
		int nameW = m_pFtFont->GetTextWidth(speaker, nameSize);
		m_pFtFont->Print(speaker, portraitX + (portraitSize - nameW) / 2, portraitY + 8, nameSize);
	} else {
		// No portrait — standalone name tag above box
		glBindTexture(GL_TEXTURE_2D, m_whiteTex);
		drawBox(nameTagX, nameTagY, nameTagW, nameTagH, glm::vec4(1.0f, 1.0f, 1.0f, 0.95f));
		drawBox(nameTagX, nameTagY, nameTagW, 3, nameColour);
		fontProgram->SetUniform("bFullColour", false);
		fontProgram->SetUniform("vColour", glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
		m_pFtFont->Print(speaker, nameTagX + 20, nameTagY + 8, nameSize);
	}

	// --- Dialogue text ---
	fontProgram->SetUniform("bFullColour", false);
	fontProgram->SetUniform("vColour", glm::vec4(0.9f, 0.9f, 0.92f, 1.0f));
	m_pFtFont->Print(text, textX, boxY + boxHeight / 2 - textSize, textSize);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void Game::RenderEscapeHUD()
{
	CShaderProgram *fontProgram = (*m_pShaderPrograms)[1];
	int width = m_gameWindow.GetWidth();
	int height = m_gameWindow.GetHeight();
	m_pCamera->SetOrthographicProjectionMatrix(width, height);

	fontProgram->UseProgram();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	fontProgram->SetUniform("matrices.projMatrix", m_pCamera->GetOrthographicProjectionMatrix());
	fontProgram->SetUniform("matrices.modelViewMatrix", glm::mat4(1));

	// Timer at top center
	glm::vec4 timerColour = (m_escapeTimer < 10.0f)
		? glm::vec4(1.0f, 0.2f, 0.2f, 1.0f)
		: glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	fontProgram->SetUniform("vColour", timerColour);

	int mins = (int)m_escapeTimer / 60;
	int secs = (int)m_escapeTimer % 60;
	char timerBuf[16];
	sprintf(timerBuf, "%d:%02d", mins, secs);
	m_pFtFont->Render(width / 2 - 40, height - 50, 36, timerBuf);

	// Game over message
	if (m_gameOver) {
		fontProgram->SetUniform("vColour", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
		if (m_escaped)
			m_pFtFont->Render(width / 2 - 80, height / 2, 48, "ESCAPED!");
		else
			m_pFtFont->Render(width / 2 - 100, height / 2, 48, "TIME'S UP");
	}

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void Game::RenderHudOverlay(float brightness)
{
	if (!m_pHudSpriteSheet || brightness <= 0.0f) return;

	CShaderProgram *fontProgram = (*m_pShaderPrograms)[1];
	int width = m_gameWindow.GetWidth();
	int height = m_gameWindow.GetHeight();

	fontProgram->UseProgram();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE); // additive blend — black background becomes transparent

	m_pCamera->SetOrthographicProjectionMatrix(width, height);
	fontProgram->SetUniform("matrices.projMatrix", m_pCamera->GetOrthographicProjectionMatrix());
	fontProgram->SetUniform("bFullColour", true);
	fontProgram->SetUniform("vColour", glm::vec4(brightness, brightness, brightness, 1.0f));

	// Compute UV coords for current frame in the sprite sheet
	// FreeImage loads bottom-to-top, so v=0 is the bottom of the image.
	// Row 0 of the sprite sheet (first frames) is at the TOP of the image = v near 1.0
	int col = m_hudFrameIndex % HUD_COLS;
	int row = m_hudFrameIndex / HUD_COLS;
	float uMin = (float)col / HUD_COLS;
	float uMax = (float)(col + 1) / HUD_COLS;
	float vMax = 1.0f - (float)row / HUD_ROWS;        // top edge of this row
	float vMin = 1.0f - (float)(row + 1) / HUD_ROWS;  // bottom edge of this row

	// Full-screen quad: screen bottom-left=(0,0), top-left=(0,h)
	float verts[] = {
		0.0f,         0.0f,          uMin, vMin,  // screen bottom → frame bottom
		(float)width, 0.0f,          uMax, vMin,
		0.0f,         (float)height, uMin, vMax,  // screen top    → frame top
		(float)width, (float)height, uMax, vMax,
	};

	glBindVertexArray(m_dialogueVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_dialogueVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

	glActiveTexture(GL_TEXTURE0);
	glBindSampler(0, 0);
	m_pHudSpriteSheet->Bind(0);
	fontProgram->SetUniform("sampler0", 0);

	fontProgram->SetUniform("matrices.modelViewMatrix", glm::mat4(1.0f));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Restore dialogue quad data
	float boxVerts[] = {
		0.0f, 0.0f,  0.0f, 0.0f,
		1.0f, 0.0f,  1.0f, 0.0f,
		0.0f, 1.0f,  0.0f, 1.0f,
		1.0f, 1.0f,  1.0f, 1.0f,
	};
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(boxVerts), boxVerts);

	fontProgram->SetUniform("bFullColour", false);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void Game::DisplayFrameRate()
{


	CShaderProgram *fontProgram = (*m_pShaderPrograms)[1];

	int height = m_gameWindow.GetHeight();

	// Increase the elapsed time and frame counter
	m_elapsedTime += m_dt;
	m_frameCount++;

	// Now we want to subtract the current time by the last time that was stored
	// to see if the time elapsed has been over a second, which means we found our FPS.
	if (m_elapsedTime > 1000)
    {
		m_elapsedTime = 0;
		m_framesPerSecond = m_frameCount;

		// Reset the frames per second
		m_frameCount = 0;
    }

	if (m_framesPerSecond > 0) {
		// Update ortho projection to match current framebuffer size
		int width = m_gameWindow.GetWidth();
		m_pCamera->SetOrthographicProjectionMatrix(width, height);

		fontProgram->UseProgram();
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		fontProgram->SetUniform("matrices.modelViewMatrix", glm::mat4(1));
		fontProgram->SetUniform("matrices.projMatrix", m_pCamera->GetOrthographicProjectionMatrix());
		fontProgram->SetUniform("vColour", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

		m_pFtFont->Render(20, height - 20, 20, "FPS: %d", m_framesPerSecond);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}
}

// The game loop runs repeatedly until game over
void Game::GameLoop()
{
	// Variable timer
	m_pHighResolutionTimer->Start();
	Update();
	Render();
	m_dt = m_pHighResolutionTimer->Elapsed();
}


void Game::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void)scancode;
	(void)mods;

	Game& game = Game::GetInstance();

	if (action == GLFW_PRESS) {
		// Tactical game input (phase 6)
		if (game.m_cutscenePhase == 6 && game.m_pTacticalGame) {
			game.m_pTacticalGame->OnKeyPress(key);
		}

		switch (key) {
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		case GLFW_KEY_1:
			game.m_pAudio->PlayEventSound();
			break;
		case GLFW_KEY_F1:
			game.m_pAudio->PlayEventSound();
			break;
		// Camera mode pins (only meaningful during the tactical phase)
		case GLFW_KEY_F3:
			if (game.m_pTacticalGame)
				game.m_pTacticalGame->SetCameraMode(CameraMode::TopDown);
			break;
		case GLFW_KEY_F4:
			if (game.m_pTacticalGame)
				game.m_pTacticalGame->SetCameraMode(CameraMode::ThirdPerson);
			break;
		case GLFW_KEY_F5:
			if (game.m_pTacticalGame)
				game.m_pTacticalGame->SetCameraMode(CameraMode::Animated);
			break;
		case GLFW_KEY_F6:
			if (game.m_pTacticalGame)
				game.m_pTacticalGame->SetCameraMode(CameraMode::Auto);
			break;
		}
	}
}

void Game::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	(void)window;
	(void)mods;
	Game& game = Game::GetInstance();
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		// Title screen — click to start cutscene
		if (game.m_titleScreen) {
			game.m_titleScreen = false;
			game.m_cutscenePhase = 0;
			game.m_cutsceneActive = true;
			game.m_pAudio->PlaySound("sensor", 0.6f);
			return;
		}

		if (game.m_cutsceneActive && game.m_cutscenePhase == 0 && game.m_dialogueLine < (int)game.m_dialogueScript.size()) {
			game.m_dialogueLine++;
		} else if (game.m_cutsceneActive && game.m_cutscenePhase == 4 && game.m_phase4Line < (int)game.m_phase4Script.size()) {
			game.m_phase4Line++;
		} else if (game.m_cutsceneActive && game.m_cutscenePhase == 5 && game.m_chenLine < (int)game.m_chenScript.size()) {
			game.m_chenLine++;
		}
	}
}

void Game::Execute()
{
	m_pHighResolutionTimer = new CHighResolutionTimer;

	if (!m_gameWindow.Init()) {
		return;
	}

	GLFWwindow* window = m_gameWindow.GetWindow();
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);

	Initialise();

	m_pHighResolutionTimer->Start();
	m_appActive = true;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		if (m_appActive) {
			GameLoop();
		}
	}

	m_gameWindow.Deinit();
}

Game& Game::GetInstance()
{
	static Game instance;

	return instance;
}

int main()
{
	Game &game = Game::GetInstance();
	game.Execute();

	return 0;
}
