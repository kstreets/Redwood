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

		void DrawFrame();
		void SwapBuffers() override;
		void ResizeRenderingSurface(const u32 width, const u32 height) override;
	private:
		void CreateVulkanInstance();
		bool VerifyValidationLayers();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapChain();
		void CreateSwapChainImageViews();
		void CreateRenderPass();
		void CreateGraphicsPipeline();
		void CreateFrameBuffers();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();

		void RecordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex);

		void RecreateSwapChain();
		void DestroySwapChain();

		QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device);
		SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice device);
		SwapChainSettings GetOptimalSwapChainSettings(const SwapChainSupportDetails& supportDetails);
	private:
		VkInstance mVulkanInstance;
		VkSurfaceKHR mSurface;

		VkSwapchainKHR mSwapChain;
		VkFormat mSwapChainImageFormat;
		VkExtent2D mSwapChainExtent;

		std::vector<VkImage> mSwapChainImages;
		std::vector<VkImageView> mSwapChainImageViews;
		std::vector<VkFramebuffer> mSwapChainFramebuffers;

		VkCommandPool mCommandPool;
		std::vector<VkCommandBuffer> mCommandBuffers;

		std::vector<VkSemaphore> mImageAvailableSemaphores;
		std::vector<VkSemaphore> mRenderFinishedSemaphores;
		std::vector<VkFence> mInFlightFences;

		VkRenderPass mRenderPass;
		VkPipelineLayout mPipelineLayout;
		VkPipeline mGraphicsPipeline;

		VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
		VkDevice mVulkanDevice;

		VkQueue mGraphicsQueue;
		VkQueue mPresentQueue;

		u32 mCurFrame;

		bool mRecreateSwapChain;
	};

}


