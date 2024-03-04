#pragma once

namespace rwd {

	class Window;

	class Context {
	public:
		virtual void Init(Window* windowHandle) = 0;
		virtual void SwapBuffers() = 0;
	};

}
