#pragma once

#include<iostream>

#include<GLFW/glfw3.h>

class WindowManagerGLFW
{
public:
	WindowManagerGLFW();

	~WindowManagerGLFW();

	unsigned int screen_width;
	unsigned int screen_height;

private:
	
	GLFWwindow* m_window;
};