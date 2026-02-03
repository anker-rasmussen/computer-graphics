#pragma once

#include "Common.h"
#include <GLFW/glfw3.h>

class GameWindow {
public:
	static GameWindow& GetInstance();

	enum {
		SCREEN_WIDTH = 800,
		SCREEN_HEIGHT = 600,
	};

	bool Init();
	void Deinit();

	bool ShouldClose() const;
	void SwapBuffers();
	void PollEvents();

	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }
	void SetDimensions(int width, int height) { m_width = width; m_height = height; }

	bool Fullscreen() const { return m_bFullscreen; }

	GLFWwindow* GetWindow() const { return m_window; }

private:
	GameWindow();
	~GameWindow();
	GameWindow(const GameWindow&);
	void operator=(const GameWindow&);

	bool InitGLFW();
	bool InitGLEW();

	bool m_bFullscreen;
	GLFWwindow* m_window;
	int m_width, m_height;
	string m_sAppName;
};
