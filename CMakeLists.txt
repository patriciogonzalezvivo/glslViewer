cmake_minimum_required(VERSION 3.2)

project(glslViewer)

if (NO_X11 OR FORCE_GBM)
    add_compile_options(-std=c++14 -DGLM_FORCE_CXX14 -fpermissive -Wno-psabi -lpthread)
endif()

file(GLOB ROOT_SOURCE 
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/gl/*.cpp"
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

    # set_target_properties(glslViewer PROPERTIES SUFFIX ".html")
    
    set(LFLAGS "${LFLAGS} -s USE_GLFW=3")
    # set(LFLAGS "${LFLAGS} -s FULL_ES2=1")
    # set(LFLAGS "${LFLAGS} -s FULL_ES3=1")
    # set(LFLAGS "${LFLAGS} -s MIN_WEBGL_VERSION=2")
    # set(LFLAGS "${LFLAGS} -s MAX_WEBGL_VERSION=2")
    # set(LFLAGS "${LFLAGS} -s WEBGL2_BACKWARDS_COMPATIBILITY_EMULATION=1")

    set(LFLAGS "${LFLAGS} -s EXPORTED_FUNCTIONS='[ \"_command\", \"_setFrag\", \"_setVert\", \"_getFrag\", \"_getVert\", \"_main\" ]' ")
    set(LFLAGS "${LFLAGS} -s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\"]' ")

    set(LFLAGS "${LFLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(LFLAGS "${LFLAGS} -s NO_DYNAMIC_EXECUTION=1")

    set(LFLAGS "${LFLAGS} --preload-file examples")
    # set(LFLAGS "${LFLAGS} --preload-file examples/2D")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/00_pipeline/00_background.frag")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/00_pipeline/01_postprocessing.frag")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/01_lighting/00_gooch.frag")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/01_lighting/01_shadows.frag")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/01_lighting/02_cubemap.frag")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/03_models/dragon.obj")
    # set(LFLAGS "${LFLAGS} --preload-file lib")
    # set(LFLAGS "${LFLAGS} --preload-file noise.vert")
    # set(LFLAGS "${LFLAGS} --preload-file extrude.frag")
    # set(LFLAGS "${LFLAGS} --preload-file globe_dots.ply")
    # set(LFLAGS "${LFLAGS} --preload-file globe_pcl.ply")
    # set(LFLAGS "${LFLAGS} --preload-file explosion_00.vert")
    # set(LFLAGS "${LFLAGS} --preload-file explosion_00.frag")
    # set(LFLAGS "${LFLAGS} --preload-file icosphere.ply")
    
    set_target_properties(glslViewer PROPERTIES LINK_FLAGS "${LFLAGS}")
    target_link_libraries(glslViewer PRIVATE glfw)
    
else()
    target_compile_definitions(glslViewer PUBLIC 
        SUPPORT_MULTITHREAD_RECORDING 
        SUPPORT_OSC
        SUPPORT_AUDIO
        # SUPPORT_PLY_BINARY
    )
    target_link_libraries(glslViewer PRIVATE lo_static pthread dl)

    if (NOT APPLE)
        target_link_libraries(glslViewer PRIVATE atomic)
    endif()

    install(TARGETS glslViewer)

endif()



