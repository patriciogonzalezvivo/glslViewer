## Ada
add_subdirectory(vera)

if (NOT EMSCRIPTEN) 
    add_subdirectory(liblo/cmake)
endif()