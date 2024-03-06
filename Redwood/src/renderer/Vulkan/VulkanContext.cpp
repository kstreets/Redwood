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
	}

	VulkanContext::~VulkanContext() {
		// Order of destroying matters!
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
		SwapChainSettings chosenSettings;

		{
			for (const auto& availableFormat : supportDetails.formats) {
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					chosenSettings.surfaceFormat = availableFormat;
				}
			}
		}

		{
			bool found = false;
			for (const auto& availablePresentMode : supportDetails.presentModes) {
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
					chosenSettings.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					found = true;
					break;
				}
			}

			// VK_PRESENT_MODE_FIFO is guaranteed to be supported 
			if (!found) {
				chosenSettings.presentMode = VK_PRESENT_MODE_FIFO_KHR;
			}
		}

		{
			// Setting the extent width and height to uint32_t max means we can 
			// and or have to manually specify the width and height in pixels.
			// Currently that is not being supported so if the current extent
			// from the passed in swap chain details is max, then we assert.
			RWD_ASSERT(supportDetails.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max(), 
				"Failed to find appropriate Vulkan swap chain extent resolution");
			chosenSettings.extent = supportDetails.capabilities.currentExtent;
		}

		return chosenSettings;
	}

}

