
solution "glslViewer"
    configurations {"Debug", "Release"}

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
        "../libs/glfw/src/*.c",
        "../libs/glfw/src/*.h"
    }

    excludes
    {
        "../libs/glfw/src/cocoa*",        
        "../libs/glfw/src/glx*",
        "../libs/glfw/src/linux*",
        "../libs/glfw/src/null*",        
        "../libs/glfw/src/posix*",
        "../libs/glfw/src/wl*",
        "../libs/glfw/src/x11*",
        "../libs/glfw/src/xkb*",
    }

    defines 
    {
        "_GLFW_WIN32"
    }

    includedirs
    {
        "../libs/glfw",         
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

    
    if _ACTION == "vs2019" then
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

    if _ACTION == "vs2019" then
        defines {"DRIVER_GLFW", "GLFW_INCLUDE_GLCOREARB", "_USE_MATH_DEFINES", "PLATFORM_WINDOWS", "GLEW_STATIC"}

        links 
        {
            "opengl32.lib", "Ws2_32.lib", "Winmm.lib"
        }
    end 

    includedirs
    {
        "../include/", 
        "../libs/glfw/include",
        "../include/glew-2.1.0/include",
        "../src/"
    }

    links 
    {
        "oscpack","rgbe", "skylight", "glfw", "glew"
    }

 
------------------------------------------------------------------------