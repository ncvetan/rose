#pragma once

#include<iostream>

#include<GLFW/glfw3.h>

namespace rose
{
	/*
	For now, only GLFW windows are supported. However, we have an abstract base class in the case
	that we add support in the future for other window creation APIs
	*/

	// Interface
	class Window
	{
	public:
		Window();

		virtual ~Window();

		virtual void EnableVSync(bool enable) = 0;

		virtual void Update() = 0;

		virtual void AddEventBinding(int event, const std::function<void ()>&) = 0;
		
		void HandleEvent(int event);

	private:

		// Mapping of events to their registered callback fns
		std::unordered_map<int, std::vector<std::function<void()>>> event_callbacks;

	};

	class WindowGLFW : public Window
	{
	public:

		WindowGLFW(std::string name = "Rose", int height = 720, int width = 1280);

		virtual ~WindowGLFW();

		void SetScrollCallback();

		void SetMouseCallback();

		void SetFramebufferSizeCallback();

		double GetTime();

		virtual void EnableVSync(bool enable);

		virtual void Update();

	private:

		void Initialize();

		void Destroy();

		unsigned int width_;
		unsigned int height_;
		GLFWwindow* window_;
		bool is_glfw_initialized;
		std::string name;
	};
}