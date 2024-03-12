#include "pch.h"
#include "SDL_vulkan.h"
#include "core/Log.h"
#include "core/Math.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"

namespace rwd {

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation",
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	VulkanContext::VulkanContext(SDL_Window* sdlWindow)
		: Context(sdlWindow), mRecreateSwapChain(false)
	{
		i32 width, height;
		SDL_GetWindowSize(sdlWindow, &width, &height);
		mWindowWidth = width;
		mWindowHeight = height;

		CreateVulkanInstance();
		SelectPhysicalDevice();
		CreateLogicalDevice();
	}

	VulkanContext::~VulkanContext() {
		// Wait for operations on the GPU to finish
		vkDeviceWaitIdle(mDevice);

		vkDestroyDevice(mDevice, nullptr);
		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
		vkDestroyInstance(mInstance, nullptr);
	}


	void VulkanContext::SwapBuffers() {

	}

	void VulkanContext::ResizeRenderingSurface(const u32 width, const u32 height) {
		mRecreateSwapChain = true;
		mWindowWidth = width;
		mWindowHeight = height;
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
			VkResult result = vkCreateInstance(&createInfo, nullptr, &mInstance);

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
			SDL_bool result = SDL_Vulkan_CreateSurface(mSdlWindow, mInstance, &mSurface);
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
		vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);

		RWD_ASSERT(deviceCount > 0, "No Vulkan devices available");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

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
		VkResult result = vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice);

		if (result != VK_SUCCESS) {
			RWD_LOG_ERROR("Failed to create logical Vulkan device");
		}

		// Next we grab references to ALL device queues which were created along side our Vulkan device
		// Device queues get cleaned up when destroying the Vulkan device they're associated with
		{
			vkGetDeviceQueue(mDevice, queueIndices.graphicsFamily.value(), 0, &mGraphicsQueue);
			vkGetDeviceQueue(mDevice, queueIndices.presentFamily.value(), 0, &mPresentQueue);
		}
	}

	QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice device) {
		if (device == VK_NULL_HANDLE) {
			device = mPhysicalDevice;
		}

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

	SwapChainSupportDetails VulkanContext::QuerySwapChainSupport(VkPhysicalDevice device) {
		if (device == VK_NULL_HANDLE) {
			device = mPhysicalDevice;
		}

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

}

