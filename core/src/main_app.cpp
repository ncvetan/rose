#include <main_app.hpp>
#include <logger.hpp>

namespace rose
{
	RoseApp::RoseApp(Platform platform) : window(nullptr), platform_(platform), is_running(false)
	{
		switch (platform)
		{
		case RoseApp::Platform::None:
			LOG_ERROR("No platform has been specified");
			break;
		case RoseApp::Platform::OpenGL:
			LOG_INFO("Successfully created OpenGL app");
			window = std::make_unique<rose::WindowGLFW>();
			break;
		default:
			break;
		}
	};

	void RoseApp::Run()
	{
		is_running = true;
		while (is_running)
		{
			glClearColor(0.95f, 0.23f, 0.42f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			window->Update();
		}
	};

	RoseApp::Platform RoseApp::get_platform() const
	{
		return platform_;
	};
}
