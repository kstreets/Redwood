#include "pch.h"
#include "glad/glad.h"
#include "core/Window.h"
#include "OpenGLContext.h"

namespace rwd {

	OpenGLContext::~OpenGLContext() {
		SDL_GL_DeleteContext(mSdlGLContext);
	}

	void OpenGLContext::Init(Window* windowHandle) {
		mWindowHandle = windowHandle;

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		mSdlGLContext = SDL_GL_CreateContext(mWindowHandle->mSdlWindow);
		gladLoadGLLoader(SDL_GL_GetProcAddress);
	}

	void OpenGLContext::SwapBuffers() {
		SDL_GL_SwapWindow(mWindowHandle->mSdlWindow);
	}

}