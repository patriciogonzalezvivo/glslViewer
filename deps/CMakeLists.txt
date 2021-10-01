## Ada
add_subdirectory(ada)

if (NOT EMSCRIPTEN) 
    set(WITH_TOOLS OFF)
    set(WITH_TESTS OFF)
    set(WITH_EXAMPLES OFF)
    set(WITH_CPP_TESTS OFF)
    option(WITH_STATIC OFF)
    add_subdirectory(liblo/cmake)
endif()

add_subdirectory(phonedepth)
