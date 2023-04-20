#pragma once

#include <memory>
#include <window_manager.hpp>

namespace rose
{
	class RoseApp
	{
	public:

		enum class Platform {
			None,
			OpenGL
		};

		RoseApp(Platform platform);

		void Run();

		Platform GetPlatform() const;

	private:

		Platform platform_ = Platform::None;
		std::unique_ptr<rose::Window> window_;
		bool is_running;
	};
}

