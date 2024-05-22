workspace "VulkanEngine"
    configurations { "Debug", "Release", "Dist" }
    architecture "x64"
    startproject "VulkanEngine"

    flags { "MultiProcessorCompile" }

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

    IncludeDir = {}
    IncludeDir["Vulkan"] = "C:/VulkanSDK/1.3.243.0/Include"
    IncludeDir["GLFW"] = "VulkanEngine/vendor/GLFW/include"
    IncludeDir["glm"] = "VulkanEngine/vendor/glm"
    IncludeDir["stb_image"] = "VulkanEngine/vendor/stb_image"
    IncludeDir["TinyLoader"] = "VulkanEngine/vendor/TinyObjLoader"


    files{
        "VulkanEngine/shaders/src/**.vert",
        "VulkanEngine/shaders/src/**.frag",
        "VulkanEngine/assets/**.*"
    }
    
  filter "configurations:Debug"
        postbuildcommands {
            '{COPY} "%{wks.location}/VulkanEngine/assets" "%{cfg.buildtarget.directory}/assets"',
            '{COPY} "%{wks.location}/VulkanEngine/shaders/bin" "%{cfg.buildtarget.directory}/shaders/bin"'
        }
        
    filter "configurations:Release"
        postbuildcommands {
            '{COPY} "%{wks.location}/VulkanEngine/assets" "%{cfg.buildtarget.directory}/assets"',
            '{COPY} "%{wks.location}/VulkanEngine/shaders/bin" "%{cfg.buildtarget.directory}/shaders/bin"'
        }
        
    filter "configurations:Dist"
        postbuildcommands {
            '{COPY} "%{wks.location}/VulkanEngine/assets" "%{cfg.buildtarget.directory}/assets"',
            '{COPY} "%{wks.location}/VulkanEngine/shaders/bin" "%{cfg.buildtarget.directory}/shaders/bin"'
        }

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
    cppdialect "C++20"

    targetdir("bin/" .. outputdir .. "/%{prj.name}")
    objdir("bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "pch.h"
    pchsource "VulkanEngine/src/pch.cpp"

    files {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/vendor/stb_image/**.cpp",
        "%{prj.name}/vendor/stb_image/**.h",
        "%{prj.name}/vendor/TinyObjLoader/**.cpp",
        "%{prj.name}/vendor/TinyObjLoader/**.h",
        "%{prj.name}/vendor/glm/glm/**.hpp",
        "%{prj.name}/vendor/glm/glm/**.inl"
    }


    includedirs {
        "%{prj.name}/src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.Vulkan}",
        "%{IncludeDir.TinyLoader}"
    }

    libdirs {
        "%{prj.name}/vendor/GLFW/lib-vc2022",
        "C:/VulkanSDK/1.3.243.0/Lib"
    }

    links {
        "glfw3.lib",
        "vulkan-1.lib"
    }


    filter "system:windows"
        systemversion "latest"
        defines {
            "GLM_FORCE_DEPTH_ZERO_TO_ONE",
            "GLM_FORCE_RADIANS"
        }

    filter "configurations:Debug"
        defines "DEBUG"
        runtime "Debug"
        symbols "On"
        flags { "NoIncrementalLink", "LinkTimeOptimization" }
        editandcontinue "Off"
        linkoptions { "/fsanitize=address" }
        targetname "VulkanEngine_Debug"

    filter "configurations:Release"
        defines "RELEASE"
        runtime "Release"
        symbols "On"
        optimize "Debug"
        targetname "VulkanEngine_Release"

    filter "configurations:Dist"
        defines "DIST"
        runtime "Release"
        symbols "Off"
        optimize "Full"
        targetname "VulkanEngine_Dist"