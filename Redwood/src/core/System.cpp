#include "pch.h"
#include "Log.h"
#include "System.h"

namespace rwd {

	void System::ReadFile(const std::string& filepath, std::vector<u8>& contents) {
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);

		RWD_ASSERT(file.is_open(), "Failed to open file {0}", filepath);

		size_t fileSize = (size_t)file.tellg();
		contents.resize(fileSize);

		file.seekg(0);
		file.read(contents.data(), fileSize);
		file.close();
	}

}
