#pragma once
#include "renderer/Context.h"

namespace rwd {

	class OpenGLContext : public Context {
	public:
		OpenGLContext(SDL_Window* sdlWindow);
		~OpenGLContext();

		void SwapBuffers() override;
		void ResizeRenderingSurface(const u32 width, const u32 height) override;
	private:
		SDL_GLContext mSdlGLContext;
	};

}
