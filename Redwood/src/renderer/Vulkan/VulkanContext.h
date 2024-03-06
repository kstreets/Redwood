#pragma once
#include <optional>
#include "core/Core.h"
#include "vulkan/vulkan.h"
#include "renderer/Context.h"

namespace rwd {

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
	private:
		void CreateVulkanInstance();
		bool VerifyValidationLayers();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapChain();

		QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device);
		SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice device);
		SwapChainSettings GetOptimalSwapChainSettings(const SwapChainSupportDetails& supportDetails);
	private:
		VkInstance mVulkanInstance;
		VkSurfaceKHR mSurface;
		VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
		VkDevice mVulkanDevice;
		VkQueue mGraphicsQueue;
		VkQueue mPresentQueue;
	};

}


