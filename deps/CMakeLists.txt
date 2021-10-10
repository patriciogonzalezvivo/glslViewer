## Ada
add_subdirectory(ada)
add_subdirectory(phonedepth)

if (NOT EMSCRIPTEN) 

    # LIBLO FOR OSC 
    # -------------------
    set(WITH_TOOLS OFF)
    set(WITH_TESTS OFF)
    set(WITH_EXAMPLES OFF)
    set(WITH_CPP_TESTS OFF)
    set(WITH_STATIC ON)
    add_subdirectory(liblo/cmake)
    
endif()

