#pragma once
#include "SDL.h"

namespace rwd {

	class Window;

	class Context {
	public:
		Context(SDL_Window* sdlWindow);
		virtual ~Context() = default;

		virtual void SwapBuffers() = 0;
		virtual void ResizeRenderingSurface(const u32 width, const u32 height) = 0;
	protected:
		SDL_Window* mSdlWindow;
	};

	inline Context::Context(SDL_Window* sdlWindow)
		: mSdlWindow(sdlWindow) { }

}
