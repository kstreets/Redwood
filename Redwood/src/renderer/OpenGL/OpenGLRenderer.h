#pragma once
#include "renderer/Renderer.h"

namespace rwd {

	class OpenGLRenderer : public Renderer {
	public:
		void SetClearColor() override;
		void Clear() override;
	};

}

