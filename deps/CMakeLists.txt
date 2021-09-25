## Ada
add_subdirectory(ada)
add_subdirectory(phonedepth)

if (NOT EMSCRIPTEN) 

    ## Liblo
    ##########################
    add_subdirectory(liblo/cmake)

endif()

# add_subdirectory(miniaudio)
# add_subdirectory(thread_pool)
# add_subdirectory(tinygltf)
# add_subdirectory(tinyobjloader)
# add_subdirectory(tinyply)

