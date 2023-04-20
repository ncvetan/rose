#include <main_app.hpp>
#include <logger.hpp>

namespace rose
{
	RoseApp::RoseApp(Platform platform) : window_(nullptr), platform_(platform), is_running(false)
	{
		switch (platform)
		{
		case RoseApp::Platform::None:
			LOG_ERROR("No platform has been specified");
			break;
		case RoseApp::Platform::OpenGL:
			LOG_INFO("Successfully created OpenGL app");
			is_running = true;
			window_ = std::make_unique<rose::WindowGLFW>();
			break;
		default:
			break;
		}
	};

	void RoseApp::Run()
	{
		while (is_running)
		{
			glClearColor(0.95f, 0.23f, 0.42f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			window_->Update();
		}
	};

	RoseApp::Platform RoseApp::GetPlatform() const
	{
		return platform_;
	};
}
