#pragma once
#include "SDL.h"
#include "renderer/Context.h"

namespace rwd {

	class OpenGLContext : public Context {
	public:
		~OpenGLContext();
		void Init(Window* windowHandle) override;
		void SwapBuffers() override;
	private:
		Window* mWindowHandle;
		SDL_GLContext mSdlGLContext;
	};

}
