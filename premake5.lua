workspace "D11_SA"
	configurations { "Debug", "Release" }
	platforms { "x64" }
	location "Build"
	startproject "Client"
	language "C++"
	cppdialect "C++23"
	system "windows"
	filter "system:windows"
		toolset "v145"
		buildoptions { "/utf-8" }

	filter "platforms:x64"
		architecture "x86_64"

	filter "configurations:Debug"
		symbols "On"
		runtime "Debug"

	filter {}

-- Visual Studio часто рисует папки решения (solution folders) выше «голых» проектов.
-- Если Client без группы, узел Vendor может оказаться первым. Держим Client в группе,
-- чтобы на верхнем уровне были App и Vendor — и Vendor был последним.
group "App"

project "Client"
	kind "WindowedApp"
	targetdir "Bin"
	objdir ("Build/obj/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}")
	defines { "DISCORD_DISABLE_IO_THREAD" }
	filter "configurations:Release"
		targetname "d11_sa"
	filter "configurations:Debug"
		targetname "d11_sa_d"
	filter {}
	links { "discord-rpc", "imgui", "d3d11", "dxgi", "d3dcompiler" }
	includedirs {
		"source",
		"source/game_sa",
		"source/assets/appicon",
		"Vendor/discord-rpc/discord/include",
		"Vendor/imgui-1.92.7",
		"Vendor/imgui-1.92.7/backends"
	}
	files {
		"source/**.h",
		"source/**.cpp",
		"source/**.rc"
	}
	-- Виртуальные папки в обозревателе решений Visual Studio (файл .vcxproj.filters).
	-- Без этого при ручной правке vcxproj всё схлопывается в один список.
	vpaths {
		["source"] = { "source/main.cpp" },
		["source/app"] = { "source/app/**.h", "source/app/**.cpp" },
		["source/core"] = { "source/core/**.h", "source/core/**.cpp" },
		["source/platform"] = { "source/platform/**.h", "source/platform/**.cpp" },
		["source/game_sa"] = { "source/game_sa/**.h", "source/game_sa/**.cpp" },
		["source/render"] = { "source/render/**.h", "source/render/**.cpp" },
		["source/assets/appicon"] = { "source/assets/appicon/**.h", "source/assets/appicon/**.rc" },
	}

group "Vendor"
include "Vendor/discord-rpc"

project "imgui"
	kind "StaticLib"
	targetdir "Bin"
	objdir ("Build/obj/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}")
	includedirs {
		"Vendor/imgui-1.92.7",
		"Vendor/imgui-1.92.7/backends"
	}
	files {
		"Vendor/imgui-1.92.7/imconfig.h",
		"Vendor/imgui-1.92.7/imgui.h",
		"Vendor/imgui-1.92.7/imgui.cpp",
		"Vendor/imgui-1.92.7/imgui_draw.cpp",
		"Vendor/imgui-1.92.7/imgui_tables.cpp",
		"Vendor/imgui-1.92.7/imgui_widgets.cpp",
		"Vendor/imgui-1.92.7/imgui_demo.cpp",
		"Vendor/imgui-1.92.7/backends/imgui_impl_win32.h",
		"Vendor/imgui-1.92.7/backends/imgui_impl_win32.cpp",
		"Vendor/imgui-1.92.7/backends/imgui_impl_dx11.h",
		"Vendor/imgui-1.92.7/backends/imgui_impl_dx11.cpp"
	}
	defines { "IMGUI_IMPL_WIN32_DISABLE_GAMEPAD" }
