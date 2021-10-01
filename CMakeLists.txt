cmake_minimum_required(VERSION 3.2)

project(glslViewer)

if (NO_X11 OR FORCE_GBM)
    add_compile_options(-std=c++14 -DGLM_FORCE_CXX14 -fpermissive -Wno-psabi -lpthread)
endif()

file(GLOB ROOT_SOURCE 
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/io/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/types/*.cpp"
)

add_executable(glslViewer ${ROOT_SOURCE})

set_target_properties(glslViewer PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

# The compiled ADA 
add_subdirectory(deps)

target_include_directories(glslViewer PUBLIC ${PROJECT_SOURCE_DIR}/deps )
target_link_libraries(glslViewer PRIVATE ada phonedepth)

if (EMSCRIPTEN)    
    file(COPY   examples DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

    set_target_properties(glslViewer PROPERTIES SUFFIX ".html")
    
    set(LFLAGS "${LFLAGS} -s USE_GLFW=3")
    # set(LFLAGS "${LFLAGS} -s FULL_ES2=1")
    # set(LFLAGS "${LFLAGS} -s FULL_ES3=1")
    set(LFLAGS "${LFLAGS} -s MAX_WEBGL_VERSION=2")
    set(LFLAGS "${LFLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(LFLAGS "${LFLAGS} -s NO_DYNAMIC_EXECUTION=1")

    set(LFLAGS "${LFLAGS} --preload-file examples")
    
    set_target_properties(glslViewer PROPERTIES LINK_FLAGS "${LFLAGS}")
    target_link_libraries(glslViewer PRIVATE glfw)
    
else()
    target_compile_definitions(glslViewer PUBLIC 
        MULTITHREAD_RECORDING 
        SUPPORT_OSC
    )
    target_link_libraries(glslViewer PRIVATE lo_static pthread)

endif()



