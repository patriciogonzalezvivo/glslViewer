## Ada
add_subdirectory(ada)

if (NOT EMSCRIPTEN) 
add_subdirectory(liblo/cmake)
endif()

# add_subdirectory(phonedepth)
# add_subdirectory(miniaudio)
# add_subdirectory(thread_pool)
# add_subdirectory(tinygltf)
# add_subdirectory(tinyobjloader)
# add_subdirectory(tinyply)

