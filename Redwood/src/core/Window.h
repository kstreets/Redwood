#pragma once
#include "Core.h"

struct SDL_Window;
class Context;

namespace rwd {

	class RWD_API Window {
	public:
		friend class App;
		friend class OpenGLContext;

		Window() = default;
		~Window();

		void SetTitle(const char* title) const;
		void SetResolution(i32 x, i32 y) const;
	private:
		void Update();
		static Window* Create();
	private:
		SDL_Window* mSdlWindow;
		Ref<Context> mContext;
	};

}