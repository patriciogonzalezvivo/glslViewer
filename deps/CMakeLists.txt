## Ada
add_subdirectory(ada)

if (NOT EMSCRIPTEN) 
    add_subdirectory(liblo/cmake)
endif()