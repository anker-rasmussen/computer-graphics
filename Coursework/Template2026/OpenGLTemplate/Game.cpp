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
	m_pParticleSystem = NULL;
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
	m_cutscenePhase = 0;
	m_cutsceneTimer = 0.0f;
	m_jeanPos = glm::vec3(-1.0f, 0.0f, -2.2f);
	m_mieliPos = glm::vec3(1.0f, 0.0f, -2.2f);
	m_dialogueScript = {
		{"Pellegrini", "The Engineer's gogols have found us. One of his chens are closing fast."},
		{"Jean",       "A chen? Out here? Someone has been careless with our trajectory."},
		{"Pellegrini", "Do not look at me, thief. I have kept my sisters blind for weeks."},
		{"Mieli",      "It doesn't matter how they found us. Perhonen, can we outrun them?"},
		{"Perhonen",   "Not a chen that size. Not without a head start we do not have."},
		{"Jean",       "Then we don't outrun them. We outthink them. That is what I do."},
		{"Pellegrini", "For once, the thief and I agree. Perhonen, show them what the q-dots see."},
	};
	m_phase4Script = {
		{"Perhonen",   "Three guberniya escorts in formation. And behind them..."},
		{"Mieli",      "That is a Founder warmind. The Engineer's own."},
		{"Jean",       "Look at the size of it. He really does want us dead."},
		{"Perhonen",   "Correction: he wants you alive. The kill signatures are for me."},
		{"Mieli",      "Then we run. Now."},
	};
	m_phase4Line = 0;
	m_chenScript = {
		{"Chen", "The conclave is ready to act."},
		{"Chen", "Shadows that most cannot see sing songs that most cannot hear."},
		{"Chen", "We bring war to our enemies."},
		{"Chen", "Battle has commenced."},
	};
	m_chenLine = 0;
	m_sobornostApproach = 0.0f;
	for (int i = 0; i < 4; i++) m_shipArrived[i] = false;
	m_screenFlash = 0.0f;
	m_screenShake = 0.0f;
	m_shipCharge = 0.0f;
	m_sailUnfurl = 0.0f;
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
	delete m_pHudSpriteSheet;
	if (m_pParticleSystem) { m_pParticleSystem->Release(); delete m_pParticleSystem; }
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

	// Create the Catmull-Rom circular camera path and track
	m_pCatmullRom = new CCatmullRom;
	m_pCatmullRom->CreateCentreline();
	m_pCatmullRom->CreateOffsetCurves();
	m_pCatmullRom->CreateTrack("resources/textures/", "asteroids.jpg");

	// Particle system for explosions
	m_pParticleSystem = new CParticleSystem;
	m_pParticleSystem->Create((*m_pShaderPrograms)[7]);

	// Initialise audio and play background music
	m_pAudio->Initialise();
	m_pAudio->LoadEventSound("resources/audio/Boing.wav");					// Royalty free sound from freesound.org
	m_pAudio->LoadMusicStream("resources/audio/DST-Garote.mp3");	// Royalty free music from http://www.nosoapradio.us/
	m_pAudio->PlayMusicStream();
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

void Game::Render()
{
	// --- Shadow map pass ---
	RenderShadowMap();

	// Clear the buffers and enable depth testing (z-buffering)
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Phase 3: cut to black with HUD fading in
	if (m_cutsceneActive && m_cutscenePhase == 3) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		RenderHudOverlay(m_hudBrightness);
		DisplayFrameRate();
		glfwSwapBuffers(m_gameWindow.GetWindow());
		return;
	}

	// Phase 6: escape gameplay rendering
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

		// Dummy shadow bias (no shadow map in escape phase)
		glm::mat4 identBias(1.0f);

		// Skybox
		ms.Push();
			pMainProgram->SetUniform("renderSkybox", true);
			ms.Translate(m_pCamera->GetPosition());
			SetMatrices(pMainProgram, ms.Top(), identBias, identBias);
			m_pSpaceSkybox->Render(cubeMapUnit);
			pMainProgram->SetUniform("renderSkybox", false);
		ms.Pop();

		// Track
		pMainProgram->SetUniform("material1.Ma", glm::vec3(0.5f));
		pMainProgram->SetUniform("material1.Md", glm::vec3(0.5f));
		pMainProgram->SetUniform("material1.Ms", glm::vec3(0.3f));
		pMainProgram->SetUniform("material1.shininess", 15.0f);
		glDisable(GL_CULL_FACE);
		ms.Push();
			SetMatrices(pMainProgram, ms.Top(), identBias, identBias);
			m_pCatmullRom->RenderTrack();
		ms.Pop();
		glEnable(GL_CULL_FACE);

		// Ship at current track position
		glm::vec3 trackPos, T, N, B;
		m_pCatmullRom->SampleTNB(m_currentDistance, trackPos, T, N, B);
		float halfWidth = 5.0f;
		glm::vec3 shipPos = trackPos + N * (m_lateralOffset * halfWidth);

		// Build orientation from TNB
		glm::mat4 orient(1.0f);
		orient[0] = glm::vec4(N, 0.0f);
		orient[1] = glm::vec4(B, 0.0f);
		orient[2] = glm::vec4(T, 0.0f); // ship nose is +Z in model space
		orient[3] = glm::vec4(shipPos, 1.0f);

		ms.Push();
			glm::mat4 shipMV = ms.Top() * orient * glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
			glm::mat3 shipNM = m_pCamera->ComputeNormalMatrix(shipMV);

			// Hull
			CShaderProgram *pHullProgram = (*m_pShaderPrograms)[3];
			pHullProgram->UseProgram();
			pHullProgram->SetUniform("matrices.projMatrix", m_pCamera->GetPerspectiveProjectionMatrix());
			pHullProgram->SetUniform("sampler0", 0);
			pHullProgram->SetUniform("charge", m_shipCharge);
			pHullProgram->SetUniform("light1.position", viewMatrix * lightPos);
			pHullProgram->SetUniform("light1.La", glm::vec3(0.6f));
			pHullProgram->SetUniform("light1.Ld", glm::vec3(0.8f));
			pHullProgram->SetUniform("light1.Ls", glm::vec3(0.5f));
			pHullProgram->SetUniform("material1.Ma", glm::vec3(0.15f, 0.15f, 0.2f));
			pHullProgram->SetUniform("material1.Md", glm::vec3(0.6f, 0.6f, 0.7f));
			pHullProgram->SetUniform("material1.Ms", glm::vec3(1.0f, 1.0f, 1.2f));
			pHullProgram->SetUniform("material1.shininess", 80.0f);
			pHullProgram->SetUniform("bUseTexture", true);
			pHullProgram->SetUniform("matrices.modelViewMatrix", shipMV);
			pHullProgram->SetUniform("matrices.normalMatrix", shipNM);
			m_pShip->RenderHull();

			// Thrust
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glDepthMask(GL_FALSE);
			glDisable(GL_CULL_FACE);

			float exhaustZ = -7.35f;
			float thrustScale = 0.1f + 0.9f * m_shipCharge;
			glm::mat4 thrustMV = shipMV
				* glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, exhaustZ))
				* glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, thrustScale))
				* glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -exhaustZ));
			pHullProgram->SetUniform("bUseTexture", false);
			pHullProgram->SetUniform("material1.Ma", glm::vec3(0.0f));
			float intensity = 0.15f + 0.85f * m_shipCharge;
			pHullProgram->SetUniform("material1.Md", intensity * glm::vec3(0.5f, 0.8f, 1.0f));
			pHullProgram->SetUniform("material1.Ms", glm::vec3(0.0f));
			pHullProgram->SetUniform("matrices.modelViewMatrix", thrustMV);
			pHullProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(thrustMV));
			m_pShip->RenderThrust();

			glEnable(GL_CULL_FACE);
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);

			// Sails (if unfurled)
			if (m_sailUnfurl > 0.01f) {
				CShaderProgram *pSailProgram = (*m_pShaderPrograms)[2];
				pSailProgram->UseProgram();
				pSailProgram->SetUniform("matrices.projMatrix", m_pCamera->GetPerspectiveProjectionMatrix());
				pSailProgram->SetUniform("sampler0", 0);
				pSailProgram->SetUniform("unfurl", m_sailUnfurl);
				pSailProgram->SetUniform("light1.position", viewMatrix * lightPos);
				pSailProgram->SetUniform("light1.La", glm::vec3(0.6f));
				pSailProgram->SetUniform("light1.Ld", glm::vec3(0.8f));
				pSailProgram->SetUniform("light1.Ls", glm::vec3(0.5f));
				pSailProgram->SetUniform("material1.Ma", glm::vec3(0.3f));
				pSailProgram->SetUniform("material1.Md", glm::vec3(1.0f));
				pSailProgram->SetUniform("material1.Ms", glm::vec3(1.5f));
				pSailProgram->SetUniform("material1.shininess", 200.0f);
				pSailProgram->SetUniform("bUseTexture", true);
				pSailProgram->SetUniform("matrices.modelViewMatrix", shipMV);
				pSailProgram->SetUniform("matrices.normalMatrix", shipNM);
				m_pShip->RenderSails();
			}
		ms.Pop();

		// Particles
		m_pParticleSystem->Render(viewMatrix, *m_pCamera->GetPerspectiveProjectionMatrix());

		// HUD
		RenderEscapeHUD();

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
				modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 3.66f);
				modelViewMatrixStack.Scale(1.8f);
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
			// Red flash for warmind, warm white for escorts
			glm::vec4 flashCol = (m_shipArrived[3] && m_screenFlash > 0.5f)
				? glm::vec4(0.8f, 0.1f, 0.05f, m_screenFlash)
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

	// Front wall — viewport texture (space view)
	if (m_cutsceneActive) {
		pMainProgram->UseProgram();
		modelViewMatrixStack.Push();
			glm::vec3 bridgeOriginVP(0.0f, 25.0f, 60.0f);
			modelViewMatrixStack.Translate(bridgeOriginVP);
			SetMatrices(pMainProgram, modelViewMatrixStack.Top(), shadowBias, spotShadowBias);
			pMainProgram->SetUniform("bUseTexture", true);
			pMainProgram->SetUniform("material1.Ma", glm::vec3(2.0f));
			pMainProgram->SetUniform("material1.Md", glm::vec3(0.0f));
			pMainProgram->SetUniform("material1.Ms", glm::vec3(0.0f));
			pMainProgram->SetUniform("material1.shininess", 1.0f);
			m_pViewportTex->Bind(0);
			m_pBridge->RenderMirrorWall();
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
			// Screen shake — random offset that decays
			if (m_screenShake > 0.01f) {
				float shakeAmt = m_screenShake * 2.0f;
				camPos.x += shakeAmt * ((float)(rand() % 200 - 100) / 100.0f);
				camPos.y += shakeAmt * ((float)(rand() % 200 - 100) / 100.0f);
			}
			glm::vec3 lookAt(20.0f, 25.0f, 150.0f);
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

	// Cutscene phase logic
	if (m_cutsceneActive) {
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

		// Phase 3: black screen — HUD fades in
		if (m_cutscenePhase == 3) {
			m_cutsceneTimer += dt_s;
			m_hudBrightness = glm::clamp(m_cutsceneTimer / 1.0f, 0.0f, 1.0f);

			// Advance HUD animation frames during fade-in
			m_hudFrameTimer += dt_s;
			if (m_hudFrameTimer >= 1.0f / HUD_FPS) {
				m_hudFrameTimer -= 1.0f / HUD_FPS;
				m_hudFrameIndex = (m_hudFrameIndex + 1) % HUD_TOTAL_FRAMES;
			}

			if (m_cutsceneTimer >= 1.0f) {
				m_cutscenePhase = 4;
				m_cutsceneTimer = 0.0f;
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
					m_pParticleSystem->Spawn(pos, 50);
					m_screenFlash = 0.3f;
					m_screenShake = 0.3f;
				}
			}
			// Warmind arrival — massive blood-red explosion
			if (!m_shipArrived[3] && m_cutsceneTimer >= arrivalTimes[3]) {
				m_shipArrived[3] = true;
				glm::vec3 wStart(900.0f, 27.0f, 1800.0f);
				glm::vec3 wEnd(600.0f, 26.0f, 1200.0f);
				glm::vec3 wPos = glm::mix(wStart, wEnd, ap);
				glm::vec3 bloodRed(0.8f, 0.05f, 0.02f);
				glm::vec3 darkRed(0.5f, 0.0f, 0.0f);
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
				m_screenFlash = 1.0f;
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

		// Phase 45: Chen hologram monologue on bridge
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
			}
		}
	}

	// Phase 6: escape gameplay
	if (m_cutscenePhase == 6 && !m_gameOver) {
		// Countdown
		m_escapeTimer -= dt_s;
		if (m_escapeTimer <= 0.0f) {
			m_escapeTimer = 0.0f;
			m_gameOver = true;
			m_escaped = false;
		}

		// Auto-advance along track
		m_currentDistance += m_shipSpeed * dt_s;
		float totalLen = m_pCatmullRom->GetTotalLength();
		if (m_currentDistance >= totalLen) {
			m_gameOver = true;
			m_escaped = true;
		}

		// Lateral movement: A/D
		GLFWwindow *win = m_gameWindow.GetWindow();
		float lateralSpeed = 2.0f;
		if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
			m_lateralOffset -= lateralSpeed * dt_s;
		if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
			m_lateralOffset += lateralSpeed * dt_s;
		m_lateralOffset = glm::clamp(m_lateralOffset, -1.0f, 1.0f);

		// Compute ship position using TNB frame
		glm::vec3 trackPos, T, N, B;
		m_pCatmullRom->SampleTNB(m_currentDistance, trackPos, T, N, B);
		float halfWidth = 5.0f;
		glm::vec3 shipPos = trackPos + N * (m_lateralOffset * halfWidth);

		// Third-person camera
		glm::vec3 camPos = shipPos - T * 15.0f + B * 5.0f;
		glm::vec3 camLookAt = shipPos + T * 20.0f;
		m_pCamera->Set(camPos, camLookAt, B);

		// Update particles
		m_pParticleSystem->Update(dt_s);

		// Combat mode: sails retract, thrust active
		m_sailUnfurl = glm::max(m_sailUnfurl - 0.6f * dt_s, 0.0f);
		m_shipCharge = glm::max(m_shipCharge - 0.05f * dt_s, 0.3f);
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
		}
	}
}

void Game::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	(void)window;
	(void)mods;
	Game& game = Game::GetInstance();
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
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
