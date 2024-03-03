#pragma once
#include "Core.h"

namespace rwd {

	class Window;
	class WindowCloseEvent;

	class RWD_API App {
	public:
		App();
		virtual ~App();
		void Run();
	protected:
		Window* mWindow;
	private:
		void MainUpdateLoop();
		void OnWindowClose(const WindowCloseEvent& e);
	private:
		bool mRunning;
	};

	App* CreateApp();
}

