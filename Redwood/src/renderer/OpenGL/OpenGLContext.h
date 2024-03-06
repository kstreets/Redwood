#pragma once
#include "renderer/Context.h"

namespace rwd {

	class OpenGLContext : public Context {
	public:
		OpenGLContext(SDL_Window* sdlWindow);
		~OpenGLContext();

		void SwapBuffers() override;
	private:
		SDL_GLContext mSdlGLContext;
	};

}
