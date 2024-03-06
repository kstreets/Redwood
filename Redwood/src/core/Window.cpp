#include "pch.h"
#include "SDL.h"
#include "Events.h"
#include "renderer/OpenGL/OpenGLContext.h"
#include "renderer/Vulkan/VulkanContext.h"
#include "Window.h"

namespace rwd {

	Window::~Window() {
		SDL_DestroyWindow(mSdlWindow);
	}

	void Window::SetTitle(const char* title) const {
		SDL_SetWindowTitle(mSdlWindow, title);
	}

	void Window::SetResolution(i32 x, i32 y) const {
		SDL_SetWindowSize(mSdlWindow, x, y);
	}

	void Window::Update() {
		SDL_Event sdlEvent;
		while (SDL_PollEvent(&sdlEvent)) {
			switch (sdlEvent.type) {
				case(SDL_QUIT):
					windowCloseEventHandler.Dispatch(WindowCloseEvent());
					return;

				case(SDL_WINDOWEVENT): {
					switch (sdlEvent.window.event) {
						case(SDL_WINDOWEVENT_RESIZED): {
							int newWidth = sdlEvent.window.data1;
							int newHeight = sdlEvent.window.data2;

							WindowResizeEvent e;
							e.width = newWidth;
							e.height = newHeight;
							windowResizeEventHandler.Dispatch(e);
							break;
						}
					}
					break;
				}

				case(SDL_KEYDOWN):case(SDL_KEYUP): {
					//Input::ProcessKeyboardEvent(sdlEvent.key);
					break;
				}
			}
		}

		mContext->SwapBuffers();
	}

	Window* Window::Create() {
		u32 windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
		//u32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

		Window* window = new Window();
		window->mSdlWindow = SDL_CreateWindow("Redwood", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, windowFlags);

		//window->mContext = MakeRef<OpenGLContext>();
		window->mContext = MakeRef<VulkanContext>(window->mSdlWindow);
		//window->mContext->Init(window->mSdlWindow);

		return window;
	}

}