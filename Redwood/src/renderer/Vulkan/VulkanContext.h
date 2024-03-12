#pragma once
#include <optional>
#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"
#include "core/Core.h"
#include "renderer/Context.h"

namespace rwd {

	const u32 MAX_FRAMES_IN_FLIGHT = 2;

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		inline bool IsComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct SwapChainSettings {
		VkSurfaceFormatKHR surfaceFormat;
		VkPresentModeKHR presentMode;
		VkExtent2D extent;
	};

	class VulkanContext : public Context {
	public:
		VulkanContext(SDL_Window* sdlWindow);
		~VulkanContext();

		void SwapBuffers() override;
		void ResizeRenderingSurface(const u32 width, const u32 height) override;

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device = VK_NULL_HANDLE);
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device = VK_NULL_HANDLE);
	private:
		void CreateVulkanInstance();
		bool VerifyValidationLayers();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();
	public:
		VkInstance mInstance;
		VkSurfaceKHR mSurface;

		VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
		VkDevice mDevice = VK_NULL_HANDLE;

		VkQueue mGraphicsQueue;
		VkQueue mPresentQueue;

		u32 mWindowWidth;
		u32 mWindowHeight;

		bool mRecreateSwapChain;
	};

}


