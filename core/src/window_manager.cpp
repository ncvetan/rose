#include <window_manager.hpp>
#include <logger.hpp>

namespace rose
{
	Window::Window() {};

	Window::~Window() {};

	WindowGLFW::WindowGLFW(std::string name, int height, int width) : name(name), height_(height), width_(width), window_(nullptr), is_glfw_initialized(false)
	{
		Initialize();
	};

	WindowGLFW::~WindowGLFW()
	{
		Destroy();
	};

	void WindowGLFW::Initialize()
	{
		// Only initialize glfw once
		if (!is_glfw_initialized)
		{
			auto init_success = glfwInit();

			// TODO: Have this conditional not compile for release builds
			if (init_success == GLFW_FALSE)
			{
				LOG_ERROR("GLFW failed to initialize");
				return;
			}

			is_glfw_initialized = true;
			LOG_INFO("GLFW has been initialized");
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		window_ = glfwCreateWindow(width_, height_, name.c_str(), NULL, NULL);

		if (window_ == nullptr)
		{
			LOG_ERROR("Failed to create GLFW window");
			glfwTerminate();
		}

		glfwMakeContextCurrent(window_);
	};

	void WindowGLFW::Update()
	{
		// Swap buffers and poll IO events
		glfwSwapBuffers(window_);
		glfwPollEvents();
	};

	void WindowGLFW::Destroy()
	{
		glfwDestroyWindow(window_);
		glfwTerminate();
		window_ = nullptr;
	};


	double WindowGLFW::GetTime()
	{
		return glfwGetTime();
	};

	void WindowGLFW::EnableVSync(bool enable)
	{
		if (enable) glfwSwapInterval(1);
		else glfwSwapInterval(0);
	};

}