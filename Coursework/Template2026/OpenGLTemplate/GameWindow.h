#pragma once

#include "Common.h"
#include <GLFW/glfw3.h>

class GameWindow {
public:
	static GameWindow& GetInstance();
	GameWindow();

	enum {
		SCREEN_WIDTH = 800,
		SCREEN_HEIGHT = 600,
	};

	bool Init();
	void Deinit();

	void SetDimensions(int width, int height) { m_width = width; m_height = height; }
	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }

	bool Fullscreen() const { return m_fullscreen; }
	GLFWwindow* GetWindow() const { return m_window; }

private:

	GameWindow(const GameWindow&);
	void operator=(const GameWindow&);

	bool m_fullscreen;
	GLFWwindow* m_window;
	int m_width;
	int m_height;
	string m_appName;
};
