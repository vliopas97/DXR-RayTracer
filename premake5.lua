workspace "RayTracerDXR"
    architecture "x64"
    startproject "RayTracerDXR"
    toolset "v143"

    configurations
    {
        "Debug",
        "Release"
    }

    OutputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "RayTracerDXR"
    location "RayTracerDXR"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++latest"
    staticruntime "off"
    floatingpoint "fast"

    targetdir ("bin/" .. OutputDir .. "/%{prj.name}")
    objdir ("bin-int/" .. OutputDir .. "/%{prj.name}")

    linkoptions { "/SUBSYSTEM:WINDOWS"}

    includedirs
    {
        "%{prj.name}/src",
        "%{wks.location}/ThirdParty/dxc",
        "%{wks.location}/ThirdParty/glm"
    }
    
    libdirs
    {
        "%{wks.location}/dxcompiler"
    }

    links
    {
        "d3d12.lib",
        "DXGI.lib"
    }

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.hlsl"
    }
    
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        symbols "on"
        optimize "Full"
        flags {"LinkTimeOptimization"}

        defines
        {
            "NDEBUG"
        }
