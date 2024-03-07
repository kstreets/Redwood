#include "pch.h"
#include "SDL_vulkan.h"
#include "core/Log.h"
#include "VulkanContext.h"

namespace rwd {

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation",
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	VulkanContext::VulkanContext(SDL_Window* sdlWindow) 
		: Context(sdlWindow)
	{
		CreateVulkanInstance();
		SelectPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateSwapChainImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFrameBuffers();
	}

	// Order of destroying matters!
	VulkanContext::~VulkanContext() {
		vkDestroyPipeline(mVulkanDevice, mGraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(mVulkanDevice, mPipelineLayout, nullptr);
		vkDestroyRenderPass(mVulkanDevice, mRenderPass, nullptr);

		for (const auto& framebuffer : mSwapChainFramebuffers) {
			vkDestroyFramebuffer(mVulkanDevice, framebuffer, nullptr);
		}

		for (const auto& imageView : mSwapChainImageViews) {
			vkDestroyImageView(mVulkanDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(mVulkanDevice, mSwapChain, nullptr);
		vkDestroyDevice(mVulkanDevice, nullptr);
		vkDestroySurfaceKHR(mVulkanInstance, mSurface, nullptr);
		vkDestroyInstance(mVulkanInstance, nullptr);
	}

	void VulkanContext::SwapBuffers() {

	}

	void VulkanContext::CreateVulkanInstance() {
		// Specify app info, technically optional but could provide optimizations
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

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
				mPhysicalDevice = device;
				RWD_LOG("Chosen Vulkan device '{0}'", props.deviceName);
				break;
			}
		}

		RWD_ASSERT(mPhysicalDevice != VK_NULL_HANDLE, "Failed to choose Vulkan device");
	}

	void VulkanContext::CreateLogicalDevice() {
		QueueFamilyIndices queueIndices = FindQueueFamilies(mPhysicalDevice);

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
		VkResult result = vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mVulkanDevice);

		if (result != VK_SUCCESS) {
			RWD_LOG_ERROR("Failed to create logical Vulkan device");
		}

		// Next we grab references to ALL device queues which were created along side our Vulkan device
		// Device queues get cleaned up when destroying the Vulkan device their associated with
		{
			vkGetDeviceQueue(mVulkanDevice, queueIndices.graphicsFamily.value(), 0, &mGraphicsQueue);
			vkGetDeviceQueue(mVulkanDevice, queueIndices.presentFamily.value(), 0, &mPresentQueue);
		}
	}

	void VulkanContext::CreateSwapChain() {
		auto swapChainSupport = QuerySwapChainSupport(mPhysicalDevice);
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
			QueueFamilyIndices indices = FindQueueFamilies(mPhysicalDevice);
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
		VkResult result = vkCreateSwapchainKHR(mVulkanDevice, &createInfo, nullptr, &mSwapChain);

		RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan swap chain");

		// Grab the swap chain images, which could be more that our previously
		// defined image count since that was used to specify the MINUMUM number
		vkGetSwapchainImagesKHR(mVulkanDevice, mSwapChain, &imageCount, nullptr);
		mSwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(mVulkanDevice, mSwapChain, &imageCount, mSwapChainImages.data());

		// Keep a reference to these swap chain settings
		mSwapChainImageFormat = swapChainSettings.surfaceFormat.format;
		mSwapChainExtent = swapChainSettings.extent;
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
			VkResult result = vkCreateImageView(mVulkanDevice, &createInfo, nullptr, &mSwapChainImageViews[i]);
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
			if (vkCreateShaderModule(mVulkanDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
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
		VkPipelineVertexInputStateCreateInfo vertexInputInfo { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 0,
			.pVertexBindingDescriptions = nullptr,
			.vertexAttributeDescriptionCount = 0,
			.pVertexAttributeDescriptions = nullptr,
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

		VkResult result = vkCreatePipelineLayout(mVulkanDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout);

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

		result = vkCreateGraphicsPipelines(mVulkanDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline);

		RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan graphics pipeline");

		vkDestroyShaderModule(mVulkanDevice, fragShaderModule, nullptr);
		vkDestroyShaderModule(mVulkanDevice, vertShaderModule, nullptr);
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

			VkResult result = vkCreateFramebuffer(mVulkanDevice, &framebufferInfo, nullptr, &mSwapChainFramebuffers[i]);

			RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan framebuffer");
		}
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

		VkRenderPassCreateInfo renderPassInfo {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &colorAttachment,
			.subpassCount = 1,
			.pSubpasses = &subpass,
		};

		VkResult result = vkCreateRenderPass(mVulkanDevice, &renderPassInfo, nullptr, &mRenderPass);

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
		chosenSettings.extent = supportDetails.capabilities.currentExtent;

		return chosenSettings;
	}

}

