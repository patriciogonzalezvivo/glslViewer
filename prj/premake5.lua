------------------------------------------------------------------------

local function IsVisualStudio()
    return _ACTION == "vs2017" or _ACTION == "vs2019"
end

------------------------------------------------------------------------

solution "glslViewer"
    configurations {"Debug", "Release"}
    platforms {"x32", "x64"}
    startproject("glslViewer")

------------------------------------------------------------------------
project "rgbe"
    
    kind "StaticLib"

    files 
    {
        "../include/rgbe/**"
    }

    includedirs
    {
        "../include/", 
        "../include/rgbe", 
    }

------------------------------------------------------------------------

project "glew"
    
    kind "StaticLib"

    files 
    {
        "../include/glew-2.1.0/src/glew.c"
    }

    defines 
    {
        "GLEW_STATIC"
    }

    includedirs
    {
        "../include/glew-2.1.0/include",         
    }

------------------------------------------------------------------------

project "skylight"
    
    kind "StaticLib"

    files 
    {
        "../include/skylight/**"
    }

    includedirs
    {
        "../include/", 
        "../include/skylight", 
    }

------------------------------------------------------------------------
    
project "glfw"
    
    kind "StaticLib"

    files 
    {
        "../include/glfw/src/*.c",
        "../include/glfw/src/*.h"
    }

    excludes
    {
        "../include/glfw/src/cocoa*",        
        "../include/glfw/src/glx*",
        "../include/glfw/src/linux*",
        "../include/glfw/src/null*",        
        "../include/glfw/src/posix*",
        "../include/glfw/src/wl*",
        "../include/glfw/src/x11*",
        "../include/glfw/src/xkb*",
    }
    if IsVisualStudio() then
        defines 
        {
            "_GLFW_WIN32"
        }
    end

    includedirs
    {
        "../include/glfw",         
    }
------------------------------------------------------------------------
project "oscpack"
    
    kind "StaticLib"

    files 
    {
        "../include/oscpack/**"
    }

    includedirs
    {
        "../include/", 
        "../include/oscpack", 
    }

    
    if IsVisualStudio() then
        defines 
        {
            "__WIN32__"
        }

        excludes 
        {
            "../include/oscpack/ip/posix/**"  
        }
    end
------------------------------------------------------------------------
project "glslViewer"

    kind "ConsoleApp"

    files 
    {
        "../src/**"
    }

    if IsVisualStudio() then
        defines {"DRIVER_GLFW", "GLFW_INCLUDE_GLCOREARB", "_USE_MATH_DEFINES", "PLATFORM_WINDOWS", "GLEW_STATIC"}

        links 
        {
            "opengl32.lib", "Ws2_32.lib", "Winmm.lib"
        }
    end 

    includedirs
    {
        "../include/", 
        "../include/glfw/include",
        "../include/glew-2.1.0/include",
        "../src/"
    }

    links 
    {
        "oscpack","rgbe", "skylight", "glfw", "glew"
    }

    debugdir("../examples/3D/01_lighting")
    debugargs("head.ply")
------------------------------------------------------------------------