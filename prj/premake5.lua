
solution "glslViewer"
    configurations {"Debug", "Release"}


    project "glslViewer"

    kind "ConsoleApp"

    files 
    {
        "../src/**"
    }

    if _ACTION == "vs2019" then
        defines {"DRIVER_GLFW", "GLFW_INCLUDE_GLCOREARB"}
    end 
    includedirs
    {
        "../include/", 
        "../libs/glfw/include",
        "../src/"
    }
