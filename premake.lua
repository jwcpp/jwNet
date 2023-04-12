workspace "network"
configurations { "Debug", "Release" }
platforms {'x32', 'x64'}
--If no system is specified, Premake will identify and target the current operating system.
--system { "windows", "linux" }

filter "system:windows"
	defines {'_CRT_SECURE_NO_WARNINGS'}
	characterset ("MBCS") --Multi-byte Character Set; currently Visual Studio only
	

-- Debug and Release
filter { "configurations:Debug" }
	defines { "DEBUG", "_DEBUG" }
	symbols "On"
filter { "configurations:Release" }
	defines { "NDEBUG" }
    optimize "On"

-- x32 and x64
filter { "platforms:x32" }
	architecture "x86"
filter { "platforms:x64" }
	architecture "x86_64"


project "net"
	location "src"
	language "C++"
	buildoptions {"-std=c++17"}
	cppdialect "c++17"
	kind "ConsoleApp"
	targetdir "bin"
	objdir "obj/net"
	local codedir = "src";
	files { codedir.."/*.h",codedir.."/*.cpp"}
	targetname "net"
	filter "system:windows"
		local codedir = "src/iocp";
		files { codedir.."/*.h",codedir.."/*.cpp"}
	filter "system:linux"
		local codedir = "src/epoll";
		files { codedir.."/*.h",codedir.."/*.cpp"}
