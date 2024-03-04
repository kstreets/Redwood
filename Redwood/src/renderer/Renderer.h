#pragma once

namespace rwd {

	class Renderer {
	public:
		virtual void SetClearColor() = 0;
		virtual void Clear() = 0;
	};

}
