workspace "VulkanEngine"
    configurations { "Debug", "Release" }
    architecture "x64"
	startproject "VulkanEngine"
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

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
IncludeDir = {}
IncludeDir ["Vulkan"] = "C:/VulkanSDK/1.3.243.0/Include"
IncludeDir["GLFW"] = "VulkanEngine/vendor/GLFW/include"
IncludeDir["glm"] = "VulkanEngine/vendor/glm"
IncludeDir["stb_image"] = "VulkanEngine/vendor/stb_image"


rule "ShaderCompilation"
    location "VulkanEngine/shaders"
    display "Shader"
    fileextension {".frag", ".vert"}
    buildmessage 'Compiling %(Filename) with glslc'
    buildcommands { 'glslc.exe "%(FullPath)" -o "shaders/bin/%(Filename)%(Extension).spv"' }
    buildoutputs { "VulkanEngine/shaders/bin/%(Filename)%(Extension).spv" }


project "VulkanEngine"
    kind "ConsoleApp"
    language "C++"
    location "VulkanEngine"
    rules { "ShaderCompilation" }
    cppdialect "C++17"

    targetdir("bin/" .. outputdir ..  "/%{prj.name}")
	objdir("bin-int/" .. outputdir ..  "/%{prj.name}")

    files 
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
        "%{prj.name}/shaders/src/**.frag",
        "%{prj.name}/shaders/src/**.vert",
        "%{prj.name}/shaders/CompileShaders.bat"
    }

    includedirs
	{
		"%{prj.name}/src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.glm}",
        "%{IncludeDir.Vulkan}"
	}
   

    libdirs
    {
        "%{prj.name}/vendor/GLFW/lib-vc2022",
        "C:/VulkanSDK/1.3.243.0/Lib"
    }
    
    links
    {
        "glfw3.lib",
        "vulkan-1.lib"
    }


    
    -- local compileShadersScript = [[
    --     @echo off

    --     setlocal enabledelayedexpansion

    --     set "shader_dir=shaders/src"
    --     set "output_dir=shaders/bin"
    --     set "compiler=glslc.exe"

    --     for %%f in ("%shader_dir%\\*.vert" "%shader_dir%\\*.frag") do (
    --         set "filename=%%~nf"
    --         set "extension=%%~xf"
    --         %compiler% "%%f" -o "%output_dir%\\!filename!!extension!.spv"
    --     )

    --     endlocal
    --     ]]

  

    -- prebuildcommands
    -- {
    --     compileShadersScript
    -- }

    -- Set up the project for shader sorcery.

    filter "system:windows"
        systemversion "latest"

        defines 
        { 
            "GLM_FORCE_DEPTH_ZERO_TO_ONE", 
            "GLM_FORCE_RADIANS" 
        }
    
    
    filter "configurations:Debug"
        defines "_DEBUG"
        runtime "Debug"
        symbols "On"
        targetname "VulkanTest_Debug"
    
    filter "configurations:Release"
        defines "_RELEASE"
        runtime "Release"
        symbols "Off"
        optimize "On"
        targetname "VulkanTest_Release"



       