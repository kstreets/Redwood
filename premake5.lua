workspace "Redwood"

	architecture "x64"

	configurations {
		"Debug",
		"Release",
		"Production"
	}

outputDir = "%{cfg.buildcfg}-%{cfg.architecture}"

project "Redwood"

	location "Redwood"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/"..outputDir.."/%{prj.name}")
	objdir ("obj/"..outputDir.."/%{prj.name}")

	files {
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	includedirs {
		"%{prj.name}/vendor/spdlog/include"
	}

	filter "system:windows"
		cppdialect "C++20"
		staticruntime "On"
		systemversion "latest"

		defines {
			"RWD_BUILD_DLL",
		}

		postbuildcommands {
			("{COPY} %{cfg.buildtarget.relpath} ../bin/"..outputDir.."/Sandbox")
		}

	filter "configurations:Debug"
		symbols "On" 
		defines {
			"RWD_DEBUG"
		}

	filter "configurations:Release"
		optimize "On" 
		defines {
			"RWD_RELEASE"
		}

	filter "configurations:Production"
		optimize "On" 
		defines {
			"RWD_PRODUCTION"
		}

project "Sandbox"

	location "Sandbox"
	kind "ConsoleApp"
	language "C++"

	targetdir ("bin/"..outputDir.."/%{prj.name}")
	objdir ("obj/"..outputDir.."/%{prj.name}")

	files {
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	includedirs {
		"Redwood/src"
	}

	links {
		"Redwood"
	}

	filter "system:windows"
		cppdialect "C++20"
		staticruntime "On"
		systemversion "latest"

	filter "configurations:Debug"
		symbols "On" 
		defines {
			"RWD_DEBUG"
		}

	filter "configurations:Release"
		optimize "On" 
		defines {
			"RWD_RELEASE"
		}

	filter "configurations:Production"
		optimize "On" 
		defines {
			"RWD_PRODUCTION"
		}
