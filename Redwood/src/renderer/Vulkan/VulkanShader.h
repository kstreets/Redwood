#pragma once
#include "vulkan/vulkan.h"
#include "core/Core.h"
#include "renderer/Shader.h"

namespace rwd {

	class VulkanShader : public Shader {
	public:
		VulkanShader() = default;
		VulkanShader(std::string& vertFile, std::string& fragFile);
		VulkanShader(std::string&& vertFile, std::string&& fragFile);

		VkShaderModule VertexModule() const;
		VkShaderModule FragmentModule() const;

		void CreateShaderModules(const VkDevice device);
		void FreeShaderModules(const VkDevice device);
	private:
		std::string mVertexFileString;
		std::string mFragmentFileString;

		VkShaderModule mVertShaderModule;
		VkShaderModule mFragShaderModule;
	};

}

