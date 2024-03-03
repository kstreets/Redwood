#pragma once
#include "Core.h"

struct SDL_Window;

namespace rwd {

	class RWD_API Window {
	public:
		friend class App;

		Window() = default;
		~Window();

		void SetTitle(const char* title) const;
		void SetResolution(i32 x, i32 y) const;
	private:
		void Update();
		static Window* Create();
		SDL_Window* mSdlWindow;
	};

}