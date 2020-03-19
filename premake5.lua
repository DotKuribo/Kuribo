workspace "kuribo"
	architecture "x86"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

function setupPreprocessor()
	filter "configurations:Debug"
		defines "DEBUG"
		symbols "On"
	
	filter "configurations:Release"
		defines "NDEBUG"
		optimize "On"
	
	filter "configurations:Dist"
		defines "NDEBUG"
		optimize "On"
end

function setupSystem()
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"KURIBO_PLATFORM_WINDOWS"
		}
end

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

function setupCppC()
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.location}/**.h",
		"%{prj.location}/**.hpp",
		"%{prj.location}/**.cpp",
		"%{prj.location}/**.hxx",
		"%{prj.location}/**.cxx",
		"%{prj.location}/**.c"

		-- "%{prj.name}/**.h",
		-- "%{prj.name}/**.hpp",
		-- "%{prj.name}/**.cpp",
		-- "%{prj.name}/**.hxx",
		-- "%{prj.name}/**.cxx",
		-- "%{prj.name}/**.c"
	}

	filter "files:*.cpp"
		language "C++"
	filter "files:*.cxx"
		language "C++"
	filter "files:*.c"
		language "C"
end

function setupStaticLib()
	kind "StaticLib"
	defines "_LIB"
end

function setupConsoleApp()
	kind "ConsoleApp"
end
function setupMainApp()
	
	setupConsoleApp()
	setupCppC()
	setupSystem()
	setupPreprocessor()
end


project "kuribo"
	location "./"
	includedirs {
		"./source",
		"./source/vendor"
	}

	setupMainApp()
	