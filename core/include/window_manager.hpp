#pragma once

#include<iostream>

#include<GLFW/glfw3.h>

namespace rose
{
	/*
	For now, only GLFW windows are supported. However, we have an abstract base class in the case
	that we add support in the future for other window creation APIs
	*/
	class Window
	{
	public:
		Window();

		virtual ~Window();

		virtual void EnableVSync(bool enable) = 0;

		virtual void Update() = 0;
		
	private:

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