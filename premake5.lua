workspace "Redwood"

	architecture "x64"

	configurations {
		"Debug",
		"Release",
		"Production"
	}

sdlFolder = "SDL2-2.30.0"
outputDir = "%{cfg.buildcfg}-%{cfg.architecture}"

project "Redwood"

	location "Redwood"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/"..outputDir.."/%{prj.name}")
	objdir ("obj/"..outputDir.."/%{prj.name}")

	pchheader "pch.h"
	pchsource "%{prj.name}/src/pch.cpp"

	files {
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/glad/glad/*.c",
	}

	includedirs {
		"%{prj.name}/src",
		"%{prj.name}/vendor/"..sdlFolder.."/include",
		"%{prj.name}/vendor/spdlog/include",
		"%{prj.name}/vendor/glad",
	}

	libdirs {
		"%{prj.name}/vendor/"..sdlFolder.."/lib/x64/",
	}

	links {
		"SDL2.lib",
	}

	-- Exlcude files from using precompiled headers (tokens don't work in the filters)

	filter "files:Redwood/vendor/glad/glad/*.c"
		flags "NoPCH"


	-- Platforms

	filter "system:windows"
		cppdialect "C++20"
		staticruntime "On"
		systemversion "latest"

		defines {
			"RWD_BUILD_DLL",
			"SDL_MAIN_HANDLED",
		}

		postbuildcommands {
			-- Make the sandbox output dir if needed so copying the .dll will never fail
			("{MKDIR} ../bin/"..outputDir.."/Sandbox"),
			("{COPY} %{cfg.buildtarget.relpath} ../bin/"..outputDir.."/Sandbox"),
		}

	-- Configurations

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

	postbuildcommands {
		("{COPY} ../%{wks.name}/vendor/"..sdlFolder.."/lib/x64/*.dll ../bin/"..outputDir.."/%{prj.name}")
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

-- Download SDL2 release .zip and extract it

print("Downloading SDL2...")

sdlZip = "Redwood/vendor/sld2.zip"
sdlUrl = "https://github.com/libsdl-org/SDL/releases/download/release-2.30.0/SDL2-devel-2.30.0-VC.zip"

http.download(sdlUrl, sdlZip)
zip.extract(sdlZip, "Redwood/vendor/")
os.remove(sdlZip)

