
solution "glslViewer"
    configurations {"Debug", "Release"}


    project "glslViewer"

    kind "ConsoleApp"

    files 
    {
        "../src/**"
    }

    if _ACTION == "vs2019" then
        defines {"DRIVER_GLFW", "GLFW_INCLUDE_GLCOREARB", "_USE_MATH_DEFINES", "PLATFORM_WINDOWS"}
    end 
    includedirs
    {
        "../include/", 
        "../libs/glfw/include",
        "../src/"
    }
