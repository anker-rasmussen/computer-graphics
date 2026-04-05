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

// Constructor
Game::Game()
	: m_gameWindow(GameWindow::GetInstance())
{
	m_pSkybox = NULL;
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
	m_pBridge = NULL;
	m_pChairMesh = NULL;
	m_pMonitorMesh = NULL;
	m_pTableMesh = NULL;
	m_cutsceneActive = true; // start in cutscene for testing
	m_dialogueVAO = 0;
	m_dialogueVBO = 0;
	m_whiteTex = 0;
	m_portraitJean = NULL;
	m_portraitMieli = NULL;
	m_portraitPellegrini = NULL;
	m_pFloorTex = NULL;
	m_pWallTex = NULL;
	m_dialogueLine = 0;
	m_dialogueScript = {
		{"Pellegrini", "There is an incursion approaching. You should prepare yourself."},
		{"Jean",       "An incursion? Here? How delightful."},
		{"Pellegrini", "This is not a game, thief. The Archons have found us."},
		{"Mieli",      "Perhonen is ready. We can outrun them if we move now."},
		{"Jean",       "Outrun them? Where is the fun in that?"},
		{"Pellegrini", "The wall will show you what approaches. Then you decide."},
	};
	m_shipCharge = 0.0f;
	m_sailUnfurl = 0.0f;
	m_shipMode = 1;
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

	// Create the skybox
	// Skybox downloaded from http://www.akimbo.in/forum/viewtopic.php?f=10&t=9
	m_pSkybox->Create(2500.0f);

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

	// Bridge textures
	m_pFloorTex = new CTexture;
	m_pFloorTex->Load("resources/textures/tiles.jpg");
	m_pWallTex = new CTexture;
	m_pWallTex->Load("resources/textures/wall.jpg");

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
	m_pMieli->Load("resources/models/Mieli/mieli.fbx");
	m_pMieli->LoadAnimation("resources/models/Mieli/Sitting Talking.fbx", "sit");
	m_pMieli->LoadAnimation("resources/models/Mieli/idle.fbx", "idle");
	m_pMieli->LoadAnimation("resources/models/Mieli/walking.fbx", "walk");
	m_pMieli->LoadAnimation("resources/models/Mieli/running.fbx", "run");
	m_pMieli->SetTexture("resources/models/Mieli/mieli_atlas.png");
	m_pMieli->SetAnimation("sit");

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

	// Create the Catmull-Rom circular camera path and track
	m_pCatmullRom = new CCatmullRom;
	m_pCatmullRom->CreateCentreline();
	m_pCatmullRom->CreateOffsetCurves();
	m_pCatmullRom->CreateTrack("resources/textures/", "asteroids.jpg");

	// Initialise audio and play background music
	m_pAudio->Initialise();
	m_pAudio->LoadEventSound("resources/audio/Boing.wav");					// Royalty free sound from freesound.org
	m_pAudio->LoadMusicStream("resources/audio/DST-Garote.mp3");	// Royalty free music from http://www.nosoapradio.us/
	m_pAudio->PlayMusicStream();
}

// Render method runs repeatedly in a loop
void Game::Render()
{

	// Clear the buffers and enable depth testing (z-buffering)
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Set up a matrix stack
	glutil::MatrixStack modelViewMatrixStack;
	modelViewMatrixStack.SetIdentity();

	// Use the main shader program
	CShaderProgram *pMainProgram = (*m_pShaderPrograms)[0];
	pMainProgram->UseProgram();
	pMainProgram->SetUniform("bUseTexture", true);
	pMainProgram->SetUniform("sampler0", 0);
	// Note: cubemap and non-cubemap textures should not be mixed in the same texture unit.  Setting unit 10 to be a cubemap texture.
	int cubeMapTextureUnit = 10;
	pMainProgram->SetUniform("CubeMapTex", cubeMapTextureUnit);


	// Set the projection matrix
	pMainProgram->SetUniform("matrices.projMatrix", m_pCamera->GetPerspectiveProjectionMatrix());

	// Call LookAt to create the view matrix and put this on the modelViewMatrix stack.
	// Store the view matrix and the normal matrix associated with the view matrix for later (they're useful for lighting -- since lighting is done in eye coordinates)
	modelViewMatrixStack.LookAt(m_pCamera->GetPosition(), m_pCamera->GetView(), m_pCamera->GetUpVector());
	glm::mat4 viewMatrix = modelViewMatrixStack.Top();
	glm::mat3 viewNormalMatrix = m_pCamera->ComputeNormalMatrix(viewMatrix);


	// Set light and materials in main shader program
	glm::vec4 lightPosition1 = glm::vec4(-100, 100, -100, 1); // Sun light *in world coordinates*
	pMainProgram->SetUniform("light1.position", viewMatrix*lightPosition1);
	pMainProgram->SetUniform("light1.La", glm::vec3(1.0f));
	pMainProgram->SetUniform("light1.Ld", glm::vec3(1.0f));
	pMainProgram->SetUniform("light1.Ls", glm::vec3(1.0f));

	// Console point lights — cyan glow from each monitoring station
	glm::vec3 bo(0.0f, 25.0f, 60.0f); // bridge origin
	glm::vec4 consoleLights[] = {
		glm::vec4(bo + glm::vec3(-2.5f, 1.2f, -1.5f), 1), // left rear
		glm::vec4(bo + glm::vec3( 2.5f, 1.2f, -1.5f), 1), // right rear
		glm::vec4(bo + glm::vec3(-2.5f, 1.2f,  1.5f), 1), // left front
		glm::vec4(bo + glm::vec3( 2.5f, 1.2f,  1.5f), 1), // right front
	};
	glm::vec3 consoleLa(0.02f, 0.03f, 0.05f);
	glm::vec3 consoleLd(0.3f, 0.5f, 0.7f);   // cool cyan-blue
	glm::vec3 consoleLs(0.2f, 0.3f, 0.4f);

	pMainProgram->SetUniform("numPointLights", 4);
	for (int i = 0; i < 4; i++) {
		string prefix = "pointLights[" + to_string(i) + "]";
		pMainProgram->SetUniform(prefix + ".position", viewMatrix * consoleLights[i]);
		pMainProgram->SetUniform(prefix + ".La", consoleLa);
		pMainProgram->SetUniform(prefix + ".Ld", consoleLd);
		pMainProgram->SetUniform(prefix + ".Ls", consoleLs);
	}

	pMainProgram->SetUniform("numLights", 5);

	pMainProgram->SetUniform("material1.Ma", glm::vec3(1.0f));
	pMainProgram->SetUniform("material1.Md", glm::vec3(0.0f));
	pMainProgram->SetUniform("material1.Ms", glm::vec3(0.0f));
	pMainProgram->SetUniform("material1.shininess", 15.0f);


	// Render the skybox and terrain with full ambient reflectance
	modelViewMatrixStack.Push();
		pMainProgram->SetUniform("renderSkybox", true);
		// Translate the modelview matrix to the camera eye point so skybox stays centred around camera
		glm::vec3 vEye = m_pCamera->GetPosition();
		modelViewMatrixStack.Translate(vEye);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pSkybox->Render(cubeMapTextureUnit);
		pMainProgram->SetUniform("renderSkybox", false);
	modelViewMatrixStack.Pop();

	// Render the planar terrain
	modelViewMatrixStack.Push();
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
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
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pHorseMesh->Render();
	modelViewMatrixStack.Pop();



	// Render the barrel
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(100.0f, 0.0f, 0.0f));
		modelViewMatrixStack.Scale(5.0f);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pBarrelMesh->Render();
	modelViewMatrixStack.Pop();

	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(110.0f, 0.0f, 0.0f));
		modelViewMatrixStack.Scale(5.0f);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pBarrelMesh->Render();
	modelViewMatrixStack.Pop();

	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(120.0f, 0.0f, 0.0f));
		modelViewMatrixStack.Scale(5.0f);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pBarrelMesh->Render();
	modelViewMatrixStack.Pop();

	// Render the sphere
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(0.0f, 2.0f, 150.0f));
		modelViewMatrixStack.Scale(2.0f);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix", m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		// To turn off texture mapping and use the sphere colour only (currently white material), uncomment the next line
		//pMainProgram->SetUniform("bUseTexture", false);
		m_pSphere->Render();
	modelViewMatrixStack.Pop();

	// --- Cutscene: bridge interior + characters ---
	if (m_cutsceneActive) {

	// Bridge room origin at (0, 25, 50) — inside the ship hull
	glm::vec3 bridgeOrigin(0.0f, 25.0f, 60.0f);

	// Bridge room
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(bridgeOrigin);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix",
			m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));

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

		// Smartmatter mirror wall — untextured, high specular
		pMainProgram->SetUniform("bUseTexture", false);
		pMainProgram->SetUniform("material1.Ma", glm::vec3(0.05f, 0.05f, 0.08f));
		pMainProgram->SetUniform("material1.Md", glm::vec3(0.1f, 0.1f, 0.15f));
		pMainProgram->SetUniform("material1.Ms", glm::vec3(1.0f, 1.0f, 1.2f));
		pMainProgram->SetUniform("material1.shininess", 200.0f);
		m_pBridge->RenderMirrorWall();
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
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix",
			m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pChairMesh->Render();
	modelViewMatrixStack.Pop();

	// Chair 2 — Jean's
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(bridgeOrigin + glm::vec3(-1.0f, 0.0f, -2.2f));
		modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 3.2f);
		modelViewMatrixStack.Scale(0.5f);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix",
			m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pChairMesh->Render();
	modelViewMatrixStack.Pop();

	// Monitoring stations — render solid then additive for screen glow
	auto renderMonitor = [&](glm::vec3 offset, float rotY) {
		modelViewMatrixStack.Push();
			modelViewMatrixStack.Translate(bridgeOrigin + offset);
			if (rotY != 0.0f)
				modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), rotY);
			modelViewMatrixStack.Scale(0.5f);
			pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
			pMainProgram->SetUniform("matrices.normalMatrix",
				m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
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
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix",
			m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
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
		pSkinProg->SetUniform("light1.La", glm::vec3(1.0f));
		pSkinProg->SetUniform("light1.Ld", glm::vec3(1.0f));
		pSkinProg->SetUniform("light1.Ls", glm::vec3(1.0f));
		for (int i = 0; i < 4; i++) {
			string prefix = "light" + to_string(i + 2);
			pSkinProg->SetUniform(prefix + ".position", viewMatrix * consoleLights[i]);
			pSkinProg->SetUniform(prefix + ".La", consoleLa);
			pSkinProg->SetUniform(prefix + ".Ld", consoleLd);
			pSkinProg->SetUniform(prefix + ".Ls", consoleLs);
		}
		pSkinProg->SetUniform("numLights", 5);
		pSkinProg->SetUniform("material1.Ma", glm::vec3(0.5f));
		pSkinProg->SetUniform("material1.Md", glm::vec3(0.5f));
		pSkinProg->SetUniform("material1.Ms", glm::vec3(1.0f));
		pSkinProg->SetUniform("material1.shininess", 15.0f);

		// Upload bone matrices
		pSkinProg->SetUniform("boneMatrices", m_pJean->GetBoneMatrices(), m_pJean->GetNumBones());

		modelViewMatrixStack.Push();
			// Jean on chair 2 (left side, facing right) — match chair at (-1.0, 0, -2.2)
			modelViewMatrixStack.Translate(bridgeOrigin + glm::vec3(-1.0f, 0.0f, -2.2f));
			modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 1.56f);
			modelViewMatrixStack.Scale(0.005f);
			pSkinProg->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
			pSkinProg->SetUniform("matrices.normalMatrix",
				m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
			m_pJean->Render();
		modelViewMatrixStack.Pop();

		// Render Mieli on chair 1
		pSkinProg->SetUniform("boneMatrices", m_pMieli->GetBoneMatrices(), m_pMieli->GetNumBones());

		modelViewMatrixStack.Push();
			// Mieli on chair 1 (right side, facing left) — match chair at (1.0, 0, -2.2)
			modelViewMatrixStack.Translate(bridgeOrigin + glm::vec3(1.0f, 0.0f, -2.2f));
			modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), -1.56f);
			modelViewMatrixStack.Scale(0.05f);
			pSkinProg->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
			pSkinProg->SetUniform("matrices.normalMatrix",
				m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
			m_pMieli->Render();
		modelViewMatrixStack.Pop();

		pMainProgram->UseProgram();
	}

	} // end cutscene block

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

		// Pass 3: Solar sails with sail shader — unfurl drives charge
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

	modelViewMatrixStack.Pop();


	// Switch back to main shader for subsequent rendering
	pMainProgram->UseProgram();

	// Render the track — two-sided since the camera views it from varying angles
	glDisable(GL_CULL_FACE);
	modelViewMatrixStack.Push();
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix",
			m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		pMainProgram->SetUniform("bUseTexture", true);
		m_pCatmullRom->RenderTrack();
	modelViewMatrixStack.Pop();
	glEnable(GL_CULL_FACE);

	// Draw the 2D graphics after the 3D graphics
	if (m_cutsceneActive && m_dialogueLine < (int)m_dialogueScript.size())
		RenderDialogue(m_dialogueScript[m_dialogueLine].speaker, m_dialogueScript[m_dialogueLine].text);
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
		// Over-the-shoulder: behind Jean's right shoulder, looking at Mieli
		glm::vec3 bo(0.0f, 25.0f, 60.0f);
		glm::vec3 jeanPos = bo + glm::vec3(-1.0f, 0.0f, -2.2f);
		glm::vec3 camPos = jeanPos + glm::vec3(-0.8f, 0.9f, -0.6f);
		glm::vec3 mieliPos = bo + glm::vec3(1.0f, 0.5f, -1.0f);
		m_pCamera->Set(camPos, mieliPos, glm::vec3(0.0f, 1.0f, 0.0f));
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

	// Update animated mesh (0.7x speed for natural conversation pace)
	m_pJean->Update(dt_s * 0.7f);
	m_pMieli->Update(dt_s * 0.7f);

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
	else
		nameColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// --- Layout ---
	int boxMarginX = width / 20;
	int boxWidth = width - 2 * boxMarginX;
	int boxHeight = height / 7;
	int boxY = boxMarginX / 2;  // slight margin from bottom edge
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

	int portraitSize = (int)((nameTagH + boxHeight / 3) * 1.15f);
	int borderW = 4;
	int portraitX = boxMarginX - borderW;
	int portraitY = boxY + boxHeight - portraitSize + nameTagH;

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
		if (game.m_cutsceneActive && game.m_dialogueLine < (int)game.m_dialogueScript.size()) {
			game.m_dialogueLine++;
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
