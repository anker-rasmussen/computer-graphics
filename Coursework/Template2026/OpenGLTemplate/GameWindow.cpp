#include "GameWindow.h"

#include <cstdio>

GameWindow& GameWindow::GetInstance()
{
	static GameWindow instance;

	return instance;
}

GameWindow::GameWindow() : m_fullscreen(false), m_window(nullptr), m_width(SCREEN_WIDTH), m_height(SCREEN_HEIGHT)
{
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	GameWindow::GetInstance().SetDimensions(width, height);
	glViewport(0, 0, width, height);
}

bool GameWindow::Init()
{
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	m_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL Template", nullptr, nullptr);
	if (!m_window) {
		fprintf(stderr, "Failed to create GLFW window\n");
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(m_window);
	glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return false;
	}

	// Capture mouse
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Get actual framebuffer size
	glfwGetFramebufferSize(m_window, &m_width, &m_height);

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
