## Ada
add_subdirectory(ada)
add_subdirectory(phonedepth)

if (NOT EMSCRIPTEN) 
    add_subdirectory(liblo/cmake)
endif()