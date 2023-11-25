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
    conformancemode "off"

    targetdir ("bin/")
    objdir ("bin-int/" .. OutputDir .. "/%{prj.name}")

    linkoptions { "/SUBSYSTEM:WINDOWS"}

    includedirs
    {
        "%{prj.name}/src",
        "%{wks.location}/ThirdParty/core",
        "%{wks.location}/ThirdParty/dxc",
        "%{wks.location}/ThirdParty/glm"
    }

    links
    {
        "d3d12.lib",
        "DXGI.lib",
        "dxguid.lib"
    }

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.hlsl"
    }

    filter "files:**.hlsl or files: **.hlsli"
        shadermodel "4.0_level_9_3"
        flags "ExcludeFromBuild"
    
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        targetname "%{prj.name}_d"

    filter "configurations:Release"
        runtime "Release"
        symbols "on"
        targetname "%{prj.name}"
        optimize "Full"
        flags {"LinkTimeOptimization"}

        defines
        {
            "NDEBUG"
        }
