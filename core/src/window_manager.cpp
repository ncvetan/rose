#include <window_manager.hpp>

WindowManagerGLFW::WindowManagerGLFW() : m_window(nullptr), screen_height(1080), screen_width(1920)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	m_window = glfwCreateWindow(screen_width, screen_height, "Rose", NULL, NULL);

	if (m_window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << "\n";
		glfwTerminate();
	}
}

WindowManagerGLFW::~WindowManagerGLFW()
{
	glfwTerminate();
	glfwDestroyWindow(m_window);
	m_window = nullptr;
}