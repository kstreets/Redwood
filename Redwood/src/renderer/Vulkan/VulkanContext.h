#pragma once
#include <optional>
#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"
#include "core/Core.h"
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

		static const VkDevice VulkanDevice();
		static const VmaAllocator CustomAllocator();

		static u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags props);
		static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
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

		inline static VkCommandPool sCommandPool;
		std::vector<VkCommandBuffer> mCommandBuffers;

		std::vector<VkSemaphore> mImageAvailableSemaphores;
		std::vector<VkSemaphore> mRenderFinishedSemaphores;
		std::vector<VkFence> mInFlightFences;

		VkRenderPass mRenderPass;
		VkPipelineLayout mPipelineLayout;
		VkPipeline mGraphicsPipeline;

		inline static VmaAllocator sCustomAllocator;

		inline static VkPhysicalDevice sPhysicalDevice = VK_NULL_HANDLE;
		inline static VkDevice sVulkanDevice = VK_NULL_HANDLE;

		inline static VkQueue sGraphicsQueue;
		VkQueue mPresentQueue;

		u32 mCurFrame;

		bool mRecreateSwapChain;
	};

}


