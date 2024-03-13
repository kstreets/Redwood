#pragma once
#include "pch.h"
#include "vulkan/vulkan.h"
#include "core/Core.h"
#include "renderer/Renderer.h"
#include "renderer/Mesh.h"
#include "VulkanContext.h"

namespace rwd {

	class VulkanMesh;

	class VulkanRenderer : public Renderer {
	public:
		void Init(Ref<VulkanContext> context);
		void Deinit();

		void DrawMesh(Mesh& mesh, Shader& shader) override;
		void SetClearColor() override;
		void Clear() override;

		void DrawFrame();
	private:
		void CreateSwapChain();
		void CreateSwapChainImageViews();
		void CreateFrameBuffers();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateRenderPass();
		void CreateSyncObjects();

		VkPipeline CreatePipelineForShader(Shader& shader);
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex);

		void RecreateSwapChain();
		void DestroySwapChain();

		void CreateVulkanMesh(Mesh& mesh);
		void CopyMeshToGpu(VulkanMesh& vulkanMesh);
		SwapChainSettings GetOptimalSwapChainSettings(const SwapChainSupportDetails& supportDetails);
	private:
		Ref<VulkanContext> mContext;
		std::vector<VkPipeline> mPipelines;

		VkSwapchainKHR mSwapChain;
		VkFormat mSwapChainImageFormat;
		VkExtent2D mSwapChainExtent;

		std::vector<VkSemaphore> mImageAvailableSemaphores;
		std::vector<VkSemaphore> mRenderFinishedSemaphores;
		std::vector<VkFence> mInFlightFences;

		std::vector<VkImage> mSwapChainImages;
		std::vector<VkImageView> mSwapChainImageViews;
		std::vector<VkFramebuffer> mSwapChainFramebuffers;

		VkCommandPool mCommandPool;
		std::vector<VkCommandBuffer> mCommandBuffers;

		VkPipelineLayout mPipelineLayout;
		VkRenderPass mRenderPass;

		u32 mCurFrame;

		VmaAllocator mAllocator;
	};

}

