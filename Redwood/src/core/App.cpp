#include "pch.h"
#include "SDL.h"
#include "Log.h"
#include "Window.h"
#include "Events.h"
#include "App.h"

namespace rwd {

	App::App() {
		mRunning = true;
		mWindow = Window::Create();
		windowCloseEventHandler.Subscribe(BIND_EVENT_FN(App::OnWindowClose));
	}

	App::~App() {
		delete mWindow;
	}

	void App::Run() {
		SDL_Init(SDL_INIT_EVERYTHING);
		Log::Init();

		while (mRunning) {
			MainUpdateLoop();
		}

		SDL_Quit();
	}

	void App::MainUpdateLoop() {
		mWindow->Update();
	}

	void App::OnWindowClose(const WindowCloseEvent& e) {
		mRunning = false;
	}

}
