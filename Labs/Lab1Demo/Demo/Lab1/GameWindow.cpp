#include "GameWindow.h"
#include "include/gl/glew.h"
#include <iostream>

GameWindow& GameWindow::GetInstance()
{
	static GameWindow instance;
	return instance;
}

GameWindow::GameWindow() : m_bFullscreen(false), m_window(nullptr),
                           m_width(SCREEN_WIDTH), m_height(SCREEN_HEIGHT)
{
}

// Initialize GLFW, create window, and initialize GLEW
bool GameWindow::Init()
{
	// Initialize GLFW
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return false;
	}

	// Set OpenGL version (4.0 core profile to match original)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// Double buffering
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
	glfwWindowHint(GLFW_DEPTH_BITS, 24);

	// Create window
	m_window = glfwCreateWindow(m_width, m_height, "Lab 1", nullptr, nullptr);
	if (!m_window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	// Make OpenGL context current
	glfwMakeContextCurrent(m_window);

	// Initialize GLEW
	if (!InitGLEW()) {
		glfwDestroyWindow(m_window);
		glfwTerminate();
		return false;
	}

	// Set vsync
	glfwSwapInterval(1);

	// Hide cursor (original behavior)
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	return true;
}

// Initialize GLEW
bool GameWindow::InitGLEW()
{
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
		return false;
	}

	// Clear the error flag set by glewInit (known issue with core contexts)
	glGetError();

	return true;
}

// Check if window should close
bool GameWindow::ShouldClose() const
{
	return glfwWindowShouldClose(m_window);
}

// Swap front and back buffers
void GameWindow::SwapBuffers()
{
	glfwSwapBuffers(m_window);
}

// Poll for and process events
void GameWindow::PollEvents()
{
	glfwPollEvents();

	// Handle ESC key to quit (matching original behavior)
	if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(m_window, GL_TRUE);
	}
}

// Cleanup
void GameWindow::Deinit()
{
	if (m_window) {
		glfwDestroyWindow(m_window);
		m_window = nullptr;
	}
	glfwTerminate();
}
