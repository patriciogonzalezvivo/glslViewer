cmake_minimum_required(VERSION 3.2)

project(glslViewer)

if (NO_X11 OR FORCE_GBM)
    add_compile_options(-std=c++14 -DGLM_FORCE_CXX14 -fpermissive -Wno-psabi -lpthread)
endif()

# The compiled ADA 
add_subdirectory(deps)

file(GLOB ROOT_SOURCE "${PROJECT_SOURCE_DIR}/src/*.cpp")
add_executable(glslViewer
    ${ROOT_SOURCE}
)

set_target_properties(glslViewer PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(glslViewer PUBLIC 
    "${PROJECT_SOURCE_DIR}/deps/"
)

if (EMSCRIPTEN)    
    set_target_properties(glslViewer PROPERTIES SUFFIX ".html")
    set(LFLAGS "${LFLAGS} -s USE_GLFW=3")
    set(LFLAGS "${LFLAGS} -s NO_DYNAMIC_EXECUTION=1")
    
    set_target_properties(glslViewer PROPERTIES LINK_FLAGS "${LFLAGS}")
    target_link_libraries(glslViewer PRIVATE ada glfw pthread)
    
else()

    target_link_libraries(glslViewer PRIVATE ada)
    target_link_libraries(glslViewer PRIVATE pthread)
endif()


