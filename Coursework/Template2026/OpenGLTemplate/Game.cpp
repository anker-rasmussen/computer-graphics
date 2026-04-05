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
	m_pBridgeMesh = NULL;
	m_pChairMesh = NULL;
	m_cutsceneActive = true; // start in cutscene for testing
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
	delete m_pBridgeMesh;
	delete m_pChairMesh;
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
	m_pCamera->SetPerspectiveProjectionMatrix(45.0f, (float) width / (float) height, 0.5f, 5000.0f);

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

	m_pFtFont->LoadSystemFont("arial.ttf", 32);
	m_pFtFont->SetShaderProgram(pFontProgram);

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
	m_pMieli->LoadAnimation("resources/models/Mieli/sit.fbx", "sit");
	m_pMieli->LoadAnimation("resources/models/Mieli/idle.fbx", "idle");
	m_pMieli->LoadAnimation("resources/models/Mieli/walking.fbx", "walk");
	m_pMieli->LoadAnimation("resources/models/Mieli/running.fbx", "run");
	m_pMieli->SetTexture("resources/models/Mieli/mieli_tex0.jpg");
	m_pMieli->SetAnimation("sit");

	// Load bridge diorama
	m_pBridgeMesh = new COpenAssetImportMesh;
	m_pBridgeMesh->Load("resources/models/bridge.glb");

	// Load chair
	m_pChairMesh = new COpenAssetImportMesh;
	m_pChairMesh->Load("resources/models/Chair/source/Chair.obj");

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
	glm::vec4 lightPosition1 = glm::vec4(-100, 100, -100, 1); // Position of light source *in world coordinates*
	pMainProgram->SetUniform("light1.position", viewMatrix*lightPosition1); // Position of light source *in eye coordinates*
	pMainProgram->SetUniform("light1.La", glm::vec3(1.0f));		// Ambient colour of light
	pMainProgram->SetUniform("light1.Ld", glm::vec3(1.0f));		// Diffuse colour of light
	pMainProgram->SetUniform("light1.Ls", glm::vec3(1.0f));		// Specular colour of light
	pMainProgram->SetUniform("material1.Ma", glm::vec3(1.0f));	// Ambient material reflectance
	pMainProgram->SetUniform("material1.Md", glm::vec3(0.0f));	// Diffuse material reflectance
	pMainProgram->SetUniform("material1.Ms", glm::vec3(0.0f));	// Specular material reflectance
	pMainProgram->SetUniform("material1.shininess", 15.0f);		// Shininess material property


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

	// Bridge diorama — inside the ship hull
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(0.0f, 25.0f, 50.0f)); // match ship position
		modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 180.0f);
		modelViewMatrixStack.Scale(3.0f);
		pMainProgram->SetUniform("bUseTexture", true);
		pMainProgram->SetUniform("material1.Ma", glm::vec3(0.5f));
		pMainProgram->SetUniform("material1.Md", glm::vec3(0.5f));
		pMainProgram->SetUniform("material1.Ms", glm::vec3(1.0f));
		pMainProgram->SetUniform("material1.shininess", 15.0f);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix",
			m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pBridgeMesh->Render();

		// Ceiling — flat quad across the top of the bridge
		pMainProgram->SetUniform("bUseTexture", false);
		pMainProgram->SetUniform("material1.Ma", glm::vec3(0.15f, 0.15f, 0.2f));
		pMainProgram->SetUniform("material1.Md", glm::vec3(0.3f, 0.3f, 0.35f));
		pMainProgram->SetUniform("material1.Ms", glm::vec3(0.5f));
		pMainProgram->SetUniform("material1.shininess", 30.0f);
		// Render ceiling as the terrain plane repositioned (reuse modelview)
		// Actually we'll just use glBegin for a simple quad
		glBegin(GL_QUADS);
			glNormal3f(0.0f, -1.0f, 0.0f);
			glVertex3f(-3.0f, 2.5f, -3.0f);
			glVertex3f( 3.0f, 2.5f, -3.0f);
			glVertex3f( 3.0f, 2.5f,  3.0f);
			glVertex3f(-3.0f, 2.5f,  3.0f);
		glEnd();
		pMainProgram->SetUniform("bUseTexture", true);
		pMainProgram->SetUniform("material1.Ma", glm::vec3(0.5f));
		pMainProgram->SetUniform("material1.Md", glm::vec3(0.5f));
		pMainProgram->SetUniform("material1.Ms", glm::vec3(1.0f));
		pMainProgram->SetUniform("material1.shininess", 15.0f);
	modelViewMatrixStack.Pop();

	// Two chairs facing each other inside the bridge
	// Chair 1 - mieli
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(0.4f, 24.15f, 50.0f));
		modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 00.0f);
		modelViewMatrixStack.Scale(0.5f);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix",
			m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pChairMesh->Render();
	modelViewMatrixStack.Pop();

	// Chair 2 - jean
	modelViewMatrixStack.Push();
		modelViewMatrixStack.Translate(glm::vec3(-0.6f, 24.15f, 50.0f));
		modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 35.0f);
		modelViewMatrixStack.Scale(0.5f);
		pMainProgram->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
		pMainProgram->SetUniform("matrices.normalMatrix",
			m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
		m_pChairMesh->Render();
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
		pSkinProg->SetUniform("material1.Ma", glm::vec3(0.5f));
		pSkinProg->SetUniform("material1.Md", glm::vec3(0.5f));
		pSkinProg->SetUniform("material1.Ms", glm::vec3(1.0f));
		pSkinProg->SetUniform("material1.shininess", 15.0f);

		// Upload bone matrices
		pSkinProg->SetUniform("boneMatrices", m_pJean->GetBoneMatrices(), m_pJean->GetNumBones());

		modelViewMatrixStack.Push();
			// Position Jean on chair 2 — match chair at (-0.5, 24.2, 50.0)
			modelViewMatrixStack.Translate(glm::vec3(-0.5f, 24.1f, 50.0f));
			modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 1.5f);
			modelViewMatrixStack.Scale(0.0049f);
			pSkinProg->SetUniform("matrices.modelViewMatrix", modelViewMatrixStack.Top());
			pSkinProg->SetUniform("matrices.normalMatrix",
				m_pCamera->ComputeNormalMatrix(modelViewMatrixStack.Top()));
			m_pJean->Render();
		modelViewMatrixStack.Pop();

		// Render Mieli on chair 1
		pSkinProg->SetUniform("boneMatrices", m_pMieli->GetBoneMatrices(), m_pMieli->GetNumBones());

		modelViewMatrixStack.Push();
			// Match chair 1 at (0.4, 24.15, 50.0), rotation 0
			modelViewMatrixStack.Translate(glm::vec3(0.4f, 24.15f, 50.0f));
			modelViewMatrixStack.Rotate(glm::vec3(0.0f, 1.0f, 0.0f), 180.0f);
			modelViewMatrixStack.Scale(0.5f);
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
			float exhaustZ = -4.25f;
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
	DisplayFrameRate();

	// Swap buffers to show the rendered image
	glfwSwapBuffers(m_gameWindow.GetWindow());

}

// Update method runs repeatedly with the Render method
void Game::Update()
{

	GLFWwindow *win = m_gameWindow.GetWindow();

	// Free camera — WASD + mouse
	m_pCamera->Update(m_dt);

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

	// Update animated mesh
	m_pJean->Update(dt_s);
	m_pMieli->Update(dt_s);

	m_pAudio->Update();
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
		// Use the font shader program and render the text
		fontProgram->UseProgram();
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		fontProgram->SetUniform("matrices.modelViewMatrix", glm::mat4(1));
		fontProgram->SetUniform("matrices.projMatrix", m_pCamera->GetOrthographicProjectionMatrix());
		fontProgram->SetUniform("vColour", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		m_pFtFont->Render(20, height - 20, 20, "FPS: %d", m_framesPerSecond);
		glDisable(GL_BLEND);
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

void Game::Execute()
{
	m_pHighResolutionTimer = new CHighResolutionTimer;

	if (!m_gameWindow.Init()) {
		return;
	}

	GLFWwindow* window = m_gameWindow.GetWindow();
	glfwSetKeyCallback(window, KeyCallback);

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
