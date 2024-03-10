#include "pch.h"
#include "SDL_vulkan.h"
#include "core/Log.h"
#include "core/Math.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"

namespace rwd {

	struct Vertex {
		Vec2 pos;
		Vec3 color;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}
	};

	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};

	const u32 MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation",
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	VulkanContext::VulkanContext(SDL_Window* sdlWindow)
		: Context(sdlWindow), mCurFrame(0), mRecreateSwapChain(false)
	{
		i32 width, height;
		SDL_GetWindowSize(sdlWindow, &width, &height);
		mSwapChainExtent = VkExtent2D(width, height);

		CreateVulkanInstance();
		SelectPhysicalDevice();
		CreateLogicalDevice();

		VmaAllocatorCreateInfo allocatorCreateInfo { };
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_1;
		allocatorCreateInfo.physicalDevice = sPhysicalDevice;
		allocatorCreateInfo.device = sVulkanDevice;
		allocatorCreateInfo.instance = mVulkanInstance;

		vmaCreateAllocator(&allocatorCreateInfo, &sCustomAllocator);

		CreateSwapChain();
		CreateSwapChainImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFrameBuffers();
		CreateCommandPool();
		CreateCommandBuffers();
		CreateSyncObjects();
	}

	// Order of destroying matters!
	VulkanContext::~VulkanContext() {
		// Wait for operations on the GPU to finish
		vkDeviceWaitIdle(sVulkanDevice);

		vmaDestroyAllocator(sCustomAllocator);

		DestroySwapChain();

		for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(sVulkanDevice, mImageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(sVulkanDevice, mRenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(sVulkanDevice, mInFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(sVulkanDevice, sCommandPool, nullptr);
		vkDestroyPipeline(sVulkanDevice, mGraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(sVulkanDevice, mPipelineLayout, nullptr);
		vkDestroyRenderPass(sVulkanDevice, mRenderPass, nullptr);

		vkDestroyDevice(sVulkanDevice, nullptr);
		vkDestroySurfaceKHR(mVulkanInstance, mSurface, nullptr);
		vkDestroyInstance(mVulkanInstance, nullptr);
	}

	void VulkanContext::DrawFrame() {
		// Wait for previous frame to finish rendering
		vkWaitForFences(sVulkanDevice, 1, &mInFlightFences[mCurFrame], VK_TRUE, UINT64_MAX);

		if (mRecreateSwapChain) {
			RecreateSwapChain();
			mRecreateSwapChain = false;
			return;
		}

		vkResetFences(sVulkanDevice, 1, &mInFlightFences[mCurFrame]);

		// Grab the next image from our swap chain
		u32 imageIndex;
		vkAcquireNextImageKHR(sVulkanDevice, mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mCurFrame], VK_NULL_HANDLE, &imageIndex);

		// Fill the command buffer
		vkResetCommandBuffer(mCommandBuffers[mCurFrame], 0);
		RecordCommandBuffer(mCommandBuffers[mCurFrame], imageIndex);

		// We want to wait with writing colors to the image until its available, 
		// so were specifying the stage of the graphics pipeline that writes to the color attachment. 
		// That means that theoretically the implementation can already start executing our vertex shader 
		// and such while the image is not yet available.
		VkSemaphore waitSemaphores[] = { mImageAvailableSemaphores[mCurFrame]};
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphores[mCurFrame] };

		VkSubmitInfo submitInfo {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

			// Set what pipeline stages we pause and wait for before continuing 
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = waitSemaphores,
			.pWaitDstStageMask = waitStages,

			// Specify command buffer
			.commandBufferCount = 1,
			.pCommandBuffers = &mCommandBuffers[mCurFrame],

			// Define what semaphores to signal when done
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signalSemaphores,
		};

		vkQueueSubmit(sGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurFrame]);

		VkSwapchainKHR swapChains[] = { mSwapChain };
		VkPresentInfoKHR presentInfo {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signalSemaphores,
			.swapchainCount = 1,
			.pSwapchains = swapChains,
			.pImageIndices = &imageIndex,
		};

		vkQueuePresentKHR(mPresentQueue, &presentInfo);
		
		mCurFrame = (mCurFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanContext::SwapBuffers() {

	}

	void VulkanContext::ResizeRenderingSurface(const u32 width, const u32 height) {
		mRecreateSwapChain = true;
		mSwapChainExtent = VkExtent2D(width, height);
	}

	const VkDevice VulkanContext::VulkanDevice() {
		RWD_ASSERT(sVulkanDevice != VK_NULL_HANDLE, "Logical Vulkan device is uninitialized");
		return sVulkanDevice;
	}

	const VmaAllocator VulkanContext::CustomAllocator() {
		return sCustomAllocator;
	}

	u32 VulkanContext::FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags props) {
		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(sPhysicalDevice, &memProps);

		for (u32 i = 0; i < memProps.memoryTypeCount; i++) {
			bool memoryTypeIsSuitable = typeFilter & (1 << i);
			bool memoryTypeMatchesProps = (memProps.memoryTypes[i].propertyFlags & props) == props;
			if (memoryTypeIsSuitable && memoryTypeMatchesProps) {
				return i;
			}
		}

		RWD_LOG_CRIT("Failed to find suitable memory");
	}

	void VulkanContext::CreateVulkanInstance() {
		// Specify app info, technically optional but could provide optimizations
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_1;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// Need to specify extensions to interface with our SDL window since Vulkan is platform agnostic 
		u32 extensionCount;
		SDL_Vulkan_GetInstanceExtensions(mSdlWindow, &extensionCount, nullptr);

		const char** extensionNames = new const char*[extensionCount];
		SDL_Vulkan_GetInstanceExtensions(mSdlWindow, &extensionCount, extensionNames);

		createInfo.enabledExtensionCount = extensionCount;
		createInfo.ppEnabledExtensionNames = extensionNames;

		// Enable validation layers for debugging
		if (VerifyValidationLayers()) {
			createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		// Create our Vulkan instance!
		{
			VkResult result = vkCreateInstance(&createInfo, nullptr, &mVulkanInstance);

			if (result != VK_SUCCESS) {
				RWD_LOG_CRIT("Failed to create Vulkan instance");
			}

			if (result == VK_ERROR_LAYER_NOT_PRESENT) {
				RWD_LOG_ERROR("Vulkan validation layers failed");
			}
		}

		// Create a Vulkan rendering surface for our SDL window 
		// This should be done right after creating the Vulkan instance
		// because it can influence the physical device selection
		{
			SDL_bool result = SDL_Vulkan_CreateSurface(mSdlWindow, mVulkanInstance, &mSurface);
			if (result == SDL_FALSE) {
				RWD_LOG_CRIT("Failed to create Vulkan rendering surface");
			}
		}
	}

	bool VulkanContext::VerifyValidationLayers() {
		u32 layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					RWD_LOG_INFO("Enabled Vulkan validation layer '{0}'", layerName);
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				RWD_LOG_ERROR("Vulkan validation layer does not exist '{0}'", layerName);
				return false;
			}
		}

		return true;
	}

	void VulkanContext::SelectPhysicalDevice() {
		u32 deviceCount = 0;
		vkEnumeratePhysicalDevices(mVulkanInstance, &deviceCount, nullptr);

		RWD_ASSERT(deviceCount > 0, "No Vulkan devices available");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(mVulkanInstance, &deviceCount, devices.data());

		for (const auto& device : devices) {

			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(device, &props);

			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures(device, &features);

			bool isDedicatedGpu = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
			bool hasGeometryShader = features.geometryShader;

			QueueFamilyIndices indices = FindQueueFamilies(device);
			bool supportsQueueFamilies = indices.IsComplete();

			bool supportsExtensions = false;
			{
				u32 extensionCount;
				vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

				std::vector<VkExtensionProperties> availableExtensions(extensionCount);
				vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

				std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

				for (const auto& extension : availableExtensions) {
					requiredExtensions.erase(extension.extensionName);
				}

				supportsExtensions = requiredExtensions.empty();
			}

			bool swapChainAdequate = false;
			{
				if (supportsExtensions) {
					SwapChainSupportDetails details = QuerySwapChainSupport(device);
					swapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
				}
			}

			if (isDedicatedGpu && hasGeometryShader && supportsQueueFamilies && supportsExtensions && swapChainAdequate) {
				sPhysicalDevice = device;
				RWD_LOG("Chosen Vulkan device '{0}'", props.deviceName);
				break;
			}
		}

		RWD_ASSERT(sPhysicalDevice != VK_NULL_HANDLE, "Failed to choose Vulkan device");
	}

	void VulkanContext::CreateLogicalDevice() {
		QueueFamilyIndices queueIndices = FindQueueFamilies(sPhysicalDevice);

		// Before creating our logical device, we need to specify what family queues we want
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		{
			// Its possible for the queue families to be the same,
			// and we cannot pass duplicates so we are using a set
			std::set<u32> uniqueQueueFamilies = { 
				queueIndices.graphicsFamily.value(), 
				queueIndices.presentFamily.value(), 
			};

			for (const u32 queueFamilyIndex : uniqueQueueFamilies) {
				VkDeviceQueueCreateInfo info { };

				info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				info.queueFamilyIndex = queueFamilyIndex;
				info.queueCount = 1;

				// (Required) Assign a priority (0.0f -> 1.0f) to influence the scheduling of command buffer execution 
				f32 queuePriority = 1.0f;
				info.pQueuePriorities = &queuePriority;

				queueCreateInfos.push_back(info);
			}
		}

		// Now we need to specify the device features we want to use 
		VkPhysicalDeviceFeatures deviceFeatures { };
		{

		}

		// Create our device info struct and enable extensions / validation layers 
		VkDeviceCreateInfo createInfo { };
		{
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
			createInfo.pQueueCreateInfos = queueCreateInfos.data();
			createInfo.pEnabledFeatures = &deviceFeatures;

			// Specify the device specific extensions
			createInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();

			// Specify the device specific validation layers
			//  
			// Previous implementations of Vulkan made a distinction between instance 
			// and device specific validation layers, but this is no longer the case.
			// That means that the enabledLayerCount and ppEnabledLayerNames fields of 
			// VkDeviceCreateInfo are ignored by up to date implementations.
			// However, it is still a good idea to set them anyway for backwards compatibility. 

			if (VerifyValidationLayers()) {
				createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
			else {
				createInfo.enabledLayerCount = 0;
			}
		}

		// Create our Vulkan device!
		VkResult result = vkCreateDevice(sPhysicalDevice, &createInfo, nullptr, &sVulkanDevice);

		if (result != VK_SUCCESS) {
			RWD_LOG_ERROR("Failed to create logical Vulkan device");
		}

		// Next we grab references to ALL device queues which were created along side our Vulkan device
		// Device queues get cleaned up when destroying the Vulkan device their associated with
		{
			vkGetDeviceQueue(sVulkanDevice, queueIndices.graphicsFamily.value(), 0, &sGraphicsQueue);
			vkGetDeviceQueue(sVulkanDevice, queueIndices.presentFamily.value(), 0, &mPresentQueue);
		}
	}

	void VulkanContext::CreateSwapChain() {
		auto swapChainSupport = QuerySwapChainSupport(sPhysicalDevice);
		auto swapChainSettings = GetOptimalSwapChainSettings(swapChainSupport);

		// Decide how many images we want in the swap chain.
		// Its recommended to use the minImageCount + 1
		u32 minImageCount = swapChainSupport.capabilities.minImageCount;
		u32 maxImageCount = swapChainSupport.capabilities.maxImageCount;
		u32 imageCount = minImageCount + 1;
		if (imageCount > maxImageCount) {
			imageCount = maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = mSurface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = swapChainSettings.surfaceFormat.format;
		createInfo.imageColorSpace = swapChainSettings.surfaceFormat.colorSpace;
		createInfo.imageExtent = swapChainSettings.extent;
		createInfo.imageArrayLayers = 1;

		// Specifies what kind of operations we use on the images in the swap chain
		// Specifying the use as color attachment means we are going to render directly to them. 
		// But if the usage bit needs to change depending on what we are doing
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// Specify how to handle swap chain images across multiple family queues
		{
			QueueFamilyIndices indices = FindQueueFamilies(sPhysicalDevice);
			uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

			// If the graphics queue family and presentation queue family are the same, 
			// which will be the case on most hardware, then we should stick to exclusive mode, 
			// because concurrent mode requires you to specify at least two distinct queue families.
			// 
			// Regardless we won't need to manually pass ownership between queues with this setup.

			if (indices.graphicsFamily != indices.presentFamily) {
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else {
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0;
				createInfo.pQueueFamilyIndices = nullptr;
			}
		}

		// We can specify if we want images in the swap chain to be transformed in any way,
		// like flipping or rotating the images
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

		// Should we use the alpha channel for blending with other windows
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = swapChainSettings.presentMode;

		// Should we not care about the color of obscured pixels when another window is in front
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		// Create the swap chain!
		VkResult result = vkCreateSwapchainKHR(sVulkanDevice, &createInfo, nullptr, &mSwapChain);

		RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan swap chain");

		// Grab the swap chain images, which could be more that our previously
		// defined image count since that was used to specify the MINUMUM number
		vkGetSwapchainImagesKHR(sVulkanDevice, mSwapChain, &imageCount, nullptr);
		mSwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(sVulkanDevice, mSwapChain, &imageCount, mSwapChainImages.data());

		// Keep a reference to these swap chain settings
		mSwapChainImageFormat = swapChainSettings.surfaceFormat.format;
		//mSwapChainExtent = swapChainSettings.extent;
	}

	void VulkanContext::CreateSwapChainImageViews() {
		mSwapChainImageViews.resize(mSwapChainImages.size());

		for (u32 i = 0; i < mSwapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo { };
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = mSwapChainImages[i];

			// Specify how the image should be interpreted
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = mSwapChainImageFormat;

			// Specify if we want to swizzle any of the color channels around
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			// Specify the image's purpose and which part of it we want to access
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			// Create the image view!
			VkResult result = vkCreateImageView(sVulkanDevice, &createInfo, nullptr, &mSwapChainImageViews[i]);
			RWD_ASSERT(result == VK_SUCCESS, "Failed to create an image view");
		}
	}

	// Temporary helper function for loading SPIR-V
	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	void VulkanContext::CreateGraphicsPipeline() {
		auto vertShaderCode = readFile("../Redwood/src/vert.spv");
		auto fragShaderCode = readFile("../Redwood/src/frag.spv");

		auto CreateShaderModule = [this] (std::vector<char> code) {
			VkShaderModuleCreateInfo createInfo { };
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

			VkShaderModule shaderModule;
			if (vkCreateShaderModule(sVulkanDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}

			return shaderModule;
		};

		VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

		// Specify pipeline stage for vertex shader
		VkPipelineShaderStageCreateInfo vertShaderStageInfo { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			// Set which shader module this stage is going to use
			.module = vertShaderModule,
			// Specify the shader's entry point by name
			.pName = "main",
		};

		// Specify pipeline stage for fragment shader
		VkPipelineShaderStageCreateInfo fragShaderStageInfo { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			// Set which shader module this stage is going to use
			.module = fragShaderModule,
			// Specify the shader's entry point by name
			.pName = "main",
		};

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// Describe the format of the data being passed into the vertex shader
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		VkPipelineVertexInputStateCreateInfo vertexInputInfo { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDescription,
			.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size(),
			.pVertexAttributeDescriptions = attributeDescriptions.data(),
		};

		// Define how we want the vertex data to be drawn (lines, triangles, etc.)
		VkPipelineInputAssemblyStateCreateInfo inputAssembly { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE,
		};

		// Viewport defines the transformation from image to framebuffer 
		// Should normally be (0, 0) -> (width, height)
		// Its possible to have the swap chain images be different sizes than the actual window,
		// so we use the swap chain's width and height instead of the window's just in case
		VkViewport viewport {
			.x = 0.0f,
			.y = 0.0f,
			.width = (f32)mSwapChainExtent.width,
			.height = (f32)mSwapChainExtent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		// The scissor defines what region of the framebuffer that pixels will get drawn to
		// during the rasterizer stage, essentially this is just a mask for the viewport 
		VkRect2D scissor {
			.offset = { 0, 0 },
			.extent = mSwapChainExtent,
		};

		// Define the number of viewports and scissors we are going to use
		VkPipelineViewportStateCreateInfo viewportState {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1,
		};

		// Specify the dynamic states for our graphics pipeline
		// Dynamic states NEED / allow for specifying some fields at draw time
		// so we don't need to create a completely different pipeline

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState { };
		{
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
			dynamicState.pDynamicStates = dynamicStates.data();
		}

		// Define the rasterizer, which takes the output vertices from the vertex shader
		// and turns them into fragments for the fragment shader to color
		VkPipelineRasterizationStateCreateInfo rasterizer {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,

			// Do we clamp or discard the fragments that are beyond the near and far planes?
			.depthClampEnable = VK_FALSE,

			// Should we allow the geometry to pass the rasterizer stage? 
			.rasterizerDiscardEnable = VK_FALSE,

			// Defines how fragments are going to be generated from the vertices
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,

			// Add any depth bias
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,

			// Must define line width to be 1.0
			.lineWidth = 1.0f,
		};

		// Define multi-sampling to preform anti-aliasing (DISABLED for now)
		VkPipelineMultisampleStateCreateInfo multisampling {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f, // Optional
			.pSampleMask = nullptr, // Optional
			.alphaToCoverageEnable = VK_FALSE, // Optional
			.alphaToOneEnable = VK_FALSE, // Optional
		};

		// Define how color blending works on a per frame buffer basis (DISABLED)
		VkPipelineColorBlendAttachmentState colorBlendAttachment { 
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
			.colorBlendOp = VK_BLEND_OP_ADD, // Optional
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
			.alphaBlendOp = VK_BLEND_OP_ADD, // Optional
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};

		// Define how color blending works globally (DISABLED)
		VkPipelineColorBlendStateCreateInfo colorBlending {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY, // Optional
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
			.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}, // Optional
		};

		// Define the uniform values for our shaders
		VkPipelineLayoutCreateInfo pipelineLayoutInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 0, // Optional
			.pSetLayouts = nullptr, // Optional
			.pushConstantRangeCount = 0, // Optional
			.pPushConstantRanges = nullptr, // Optional
		};

		VkResult result = vkCreatePipelineLayout(sVulkanDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout);

		RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan pipeline layout");

		VkGraphicsPipelineCreateInfo pipelineInfo {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,

			// Define shader stages (Vertex and Fragment)
			.stageCount = 2,
			.pStages = shaderStages,

			// Reference all fixed function structures
			.pVertexInputState = &vertexInputInfo,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = nullptr,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicState,

			// Pipeline layout (Uniforms)
			.layout = mPipelineLayout,

			// Render pass
			.renderPass = mRenderPass,
			.subpass = 0,
		};

		result = vkCreateGraphicsPipelines(sVulkanDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline);

		RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan graphics pipeline");

		vkDestroyShaderModule(sVulkanDevice, fragShaderModule, nullptr);
		vkDestroyShaderModule(sVulkanDevice, vertShaderModule, nullptr);
	}

	void VulkanContext::CreateFrameBuffers() {
		mSwapChainFramebuffers.resize(mSwapChainImageViews.size());

		for (u32 i = 0; i < mSwapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				mSwapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo {
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = mRenderPass,
				.attachmentCount = 1,
				.pAttachments = attachments,
				.width = mSwapChainExtent.width,
				.height = mSwapChainExtent.height,
				.layers = 1,
			};

			VkResult result = vkCreateFramebuffer(sVulkanDevice, &framebufferInfo, nullptr, &mSwapChainFramebuffers[i]);

			RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan framebuffer");
		}
	}

	void VulkanContext::CreateCommandPool() {
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(sPhysicalDevice);

		// Each command pool can only allocate command buffers that 
		// are submitted on a single type of queue.

		VkCommandPoolCreateInfo poolInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
		};

		VkResult result = vkCreateCommandPool(sVulkanDevice, &poolInfo, nullptr, &sCommandPool);

		RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan command pool");
	}

	void VulkanContext::CreateCommandBuffers() {
		mCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = sCommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = (uint32_t)mCommandBuffers.size(),
		};

		VkResult result = vkAllocateCommandBuffers(sVulkanDevice, &allocInfo, mCommandBuffers.data());

		RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan command buffers");
	}

	void VulkanContext::CreateSyncObjects() {
		mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		VkFenceCreateInfo fenceInfo {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			// Set it to be signaled on creation so first call to wait doesn't block 
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkCreateSemaphore(sVulkanDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]);
			vkCreateSemaphore(sVulkanDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]);
			vkCreateFence(sVulkanDevice, &fenceInfo, nullptr, &mInFlightFences[i]);
		}
	}

	void VulkanContext::RecordCommandBuffer(VkCommandBuffer cmdBuffer, u32 imageIndex) {
		VkCommandBufferBeginInfo beginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0, // Optional
			.pInheritanceInfo = nullptr, // Optional
		};

		VkResult result = vkBeginCommandBuffer(cmdBuffer, &beginInfo);

		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

		VkRenderPassBeginInfo renderPassInfo {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = mRenderPass,
			.framebuffer = mSwapChainFramebuffers[imageIndex],
			.renderArea {
				.offset = {0, 0}, 
				.extent = mSwapChainExtent,
			},
			.clearValueCount = 1,
			.pClearValues = &clearColor,
		};

		vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

		static VulkanVertexBuffer vertexBuffer((void*)vertices.data(), sizeof(Vertex) * vertices.size());

		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);

		static VulkanIndexBuffer indexBuffer((void*)indices.data(), sizeof(uint16_t) * indices.size());
		vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		// Set the dynamic states that were specified in the pipeline
		{
			VkViewport viewport {
				.x = 0.0f,
				.y = 0.0f,
				.width = (f32)mSwapChainExtent.width,
				.height = (f32)mSwapChainExtent.height,
				.minDepth = 0.0f,
				.maxDepth = 1.0f,
			};

			VkRect2D scissor {
				.offset = { 0, 0 },
				.extent = mSwapChainExtent,
			};

			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
		}

		vkCmdDrawIndexed(cmdBuffer, (uint32_t)indices.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(cmdBuffer);
		vkEndCommandBuffer(cmdBuffer);
	}

	void VulkanContext::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo allocInfo { };
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = sCommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(sVulkanDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo { };
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion { };
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo { };
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(sGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(sGraphicsQueue);

		vkFreeCommandBuffers(sVulkanDevice, sCommandPool, 1, &commandBuffer);
	}

	void VulkanContext::RecreateSwapChain() {
		// Wait for operations on the GPU to finish
		vkDeviceWaitIdle(sVulkanDevice);

		DestroySwapChain();

		CreateSwapChain();
		CreateSwapChainImageViews();
		CreateFrameBuffers();
	}

	void VulkanContext::DestroySwapChain() {
		for (const auto& framebuffer : mSwapChainFramebuffers) {
			vkDestroyFramebuffer(sVulkanDevice, framebuffer, nullptr);
		}

		for (const auto& imageView : mSwapChainImageViews) {
			vkDestroyImageView(sVulkanDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(sVulkanDevice, mSwapChain, nullptr);
	}

	void VulkanContext::CreateRenderPass() {
		VkAttachmentDescription colorAttachment {
			.format = mSwapChainImageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,

			// Define what to do with color and depth data before and after rendering
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,

			// Define what to do with the stencil data before and after rendering
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

			// Define what layout the framebuffer will initially have and what layout
			// we want to automatically transition to at the end of the render pass 
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		// A single render pass may consist of multiple sub passes
		// Each subpass needs to reference a frame buffer attachment
		// which is wrapped in a VKAttachmentReference struct
		VkAttachmentReference colorAttachmentRef {
			// References the index of the attachment from the attachment 
			// description array which is defined in the subpass description
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		// Define our sub passes

		// The index of the attachments in the pColorAttachments array is directly 
		// referenced from the fragment shader with the layout(location = n) directive!
		VkSubpassDescription subpass {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef,
		};

		// End define sub passes

		// There are two implicit sub passes that occur at the start and end of a render pass
		// However, when submitting our drawing commands, we don't wait for the next swap chain
		// image to be ready for use yet, so we can go ahead and execute the vertex shader, but
		// then after we go ahead and wait for setting pixels (Color Attachment) to the image
		// 
		// So technically we won't always have a swap chain image to render to when starting the
		// render pass, so we can't have the starting implicit sub pass execute at the default time
		// 
		// The VkSubpassDependency struct is what we can use to instruct the starting implicit sub
		// pass to occur at a different stage in the pipeline, preferably when we have an image
		VkSubpassDependency dependency {
			.srcSubpass = VK_SUBPASS_EXTERNAL, // Implicit subpass index (setting it to src means start)
			.dstSubpass = 0,                   // Our subpass index

			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Operation to wait for
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Operation to wait for

			.srcAccessMask = 0,                                    // Requires nothing
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // Requires writing to color attachment
		};

		VkRenderPassCreateInfo renderPassInfo {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &colorAttachment,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency,
		};

		VkResult result = vkCreateRenderPass(sVulkanDevice, &renderPassInfo, nullptr, &mRenderPass);

		RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan render pass");
	}

	QueueFamilyIndices VulkanContext::FindQueueFamilies(const VkPhysicalDevice device) {
		u32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		i32 i = 0;
		QueueFamilyIndices indices;

		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.IsComplete()) break;

			i++;
		}

		return indices;
	}

	SwapChainSupportDetails VulkanContext::QuerySwapChainSupport(const VkPhysicalDevice device) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mSurface, &details.capabilities);

		{
			u32 formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, nullptr);

			if (formatCount != 0) {
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, details.formats.data());
			}
		}

		{
			u32 presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, nullptr);

			if (presentModeCount != 0) {
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, details.presentModes.data());
			}
		}

		return details;
	}

	SwapChainSettings VulkanContext::GetOptimalSwapChainSettings(const SwapChainSupportDetails& supportDetails) {
		SwapChainSettings chosenSettings { };

		for (const auto& availableFormat : supportDetails.formats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				chosenSettings.surfaceFormat = availableFormat;
			}
		}

		bool foundPresentMode = false;
		for (const auto& availablePresentMode : supportDetails.presentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				chosenSettings.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
				foundPresentMode = true;
				break;
			}
		}

		// VK_PRESENT_MODE_FIFO is guaranteed to be supported 
		if (!foundPresentMode) {
			chosenSettings.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		}

		// Setting the extent width and height to uint32_t max means we can 
		// and or have to manually specify the width and height in pixels.
		// Currently that is not being supported so if the current extent
		// from the passed in swap chain details is max, then we assert.
		RWD_ASSERT(supportDetails.capabilities.currentExtent.width != UINT32_MAX, 
			"Failed to find appropriate Vulkan swap chain extent resolution");
		chosenSettings.extent = mSwapChainExtent;

		return chosenSettings;
	}

}

