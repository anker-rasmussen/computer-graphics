#include "GameWindow.h"

#include <iostream>

// GLFW callbacks
static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	GameWindow::GetInstance().SetDimensions(width, height);
	glViewport(0, 0, width, height);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

GameWindow& GameWindow::GetInstance()
{
	static GameWindow instance;
	return instance;
}

GameWindow::GameWindow() : m_bFullscreen(false), m_window(nullptr), m_width(SCREEN_WIDTH), m_height(SCREEN_HEIGHT)
{
}

GameWindow::~GameWindow()
{
}

bool GameWindow::InitGLFW()
{
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return false;
	}

	// Set OpenGL version to 4.0 core profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	return true;
}

bool GameWindow::InitGLEW()
{
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
		return false;
	}
	return true;
}

bool GameWindow::Init()
{
	if (!InitGLFW()) {
		return false;
	}

	m_sAppName = "INM376 / IN3005 Computer Graphics Using OpenGL: Lab 2";

	// Create window
	m_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, m_sAppName.c_str(), nullptr, nullptr);
	if (!m_window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(m_window);

	// Initialize GLEW after making context current
	if (!InitGLEW()) {
		glfwDestroyWindow(m_window);
		glfwTerminate();
		return false;
	}

	// Set callbacks
	glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
	glfwSetKeyCallback(m_window, key_callback);

	// Get actual framebuffer size
	glfwGetFramebufferSize(m_window, &m_width, &m_height);

	// Enable vsync
	glfwSwapInterval(1);

	// Hide cursor
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	return true;
}

void GameWindow::Deinit()
{
	if (m_window) {
		glfwDestroyWindow(m_window);
		m_window = nullptr;
	}
	glfwTerminate();
}

bool GameWindow::ShouldClose() const
{
	return glfwWindowShouldClose(m_window);
}

void GameWindow::SwapBuffers()
{
	glfwSwapBuffers(m_window);
}

void GameWindow::PollEvents()
{
	glfwPollEvents();
}
