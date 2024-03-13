#include "pch.h"
#include "core/Log.h"
#include "core/System.h"
#include "VulkanShader.h"

namespace rwd {

	VulkanShader::VulkanShader(std::string& vertFile, std::string& fragFile)
		: mVertShaderModule(VK_NULL_HANDLE), mFragShaderModule(VK_NULL_HANDLE),
		  mVertexFileString(vertFile), mFragmentFileString(fragFile)
	{

	}

	VulkanShader::VulkanShader(std::string&& vertFile, std::string&& fragFile)
		: mVertShaderModule(VK_NULL_HANDLE), mFragShaderModule(VK_NULL_HANDLE),
		  mVertexFileString(std::move(vertFile)), mFragmentFileString(std::move(fragFile))
	{

	}

	VkShaderModule VulkanShader::VertexModule() const {
		RWD_ASSERT(mVertShaderModule != VK_NULL_HANDLE, 
			"Need to call CreateShaderModules before accessing vertex module");
		return mVertShaderModule;
	}

	VkShaderModule VulkanShader::FragmentModule() const {
		RWD_ASSERT(mFragShaderModule != VK_NULL_HANDLE, 
			"Need to call CreateShaderModules before accessing fragment module");
		return mFragShaderModule;
	}

	void VulkanShader::CreateShaderModules(const VkDevice device) {
		std::vector<u8> vertCode;
		System::ReadFile(mVertexFileString, vertCode);

		std::vector<u8> fragCode;
		System::ReadFile(mFragmentFileString, fragCode);

		auto CreateShaderModule = [device] (std::vector<u8>& code) {
			VkShaderModuleCreateInfo createInfo { };
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}

			return shaderModule;
		};

		mVertShaderModule = CreateShaderModule(vertCode);
		mFragShaderModule = CreateShaderModule(fragCode);
	}

	void VulkanShader::FreeShaderModules(const VkDevice device) {
		vkDestroyShaderModule(device, mVertShaderModule, nullptr);
		vkDestroyShaderModule(device, mFragShaderModule, nullptr);
	}

}
