#include "pch.h"
#include "SDL.h"
#include "Log.h"
#include "Window.h"
#include "Events.h"
#include "renderer/OpenGL/OpenGLRenderer.h"
#include "renderer/Vulkan/VulkanRenderer.h"
#include "renderer/Mesh.h"
#include "renderer/Shader.h"
#include "App.h"

namespace rwd {

	VulkanRenderer* renderer;
	Mesh* triangleMesh;
	Shader shader;

	App::App() {
		mRunning = true;

		Log::Init();
		SDL_Init(SDL_INIT_EVERYTHING);

		mWindow = Window::Create();
		windowCloseEventHandler.Subscribe(BIND_EVENT_FN(App::OnWindowClose));

		renderer = new VulkanRenderer;
		renderer->Init(std::dynamic_pointer_cast<VulkanContext>(mWindow->mContext));

		f32 verts[] {
			-0.5, -0.5, 0.0,
			0.5, -0.5, 0.0,
			 0.0, 0.5, 0.0,
		};

		i32 indices[] {
			0, 1, 2
		};

		//triangleMesh = new Mesh(verts, sizeof(f32) * 9, indices, sizeof(i32) * 3, 3);

		//shader = Shader("../Redwood/src/Color.vert", "../Redwood/src/Color.frag");
	}

	App::~App() {
		delete mWindow;
	}

	void App::Run() {
		while (mRunning) {
			MainUpdateLoop();
		}
		SDL_Quit();
	}

	void App::MainUpdateLoop() {
		//static OpenGLRenderer renderer;

		renderer->DrawFrame();
		//renderer.Clear();
		//renderer.DrawMesh(*triangleMesh, shader);
		mWindow->Update();
	}

	void App::OnWindowClose(const WindowCloseEvent& e) {
		mRunning = false;
	}

}
