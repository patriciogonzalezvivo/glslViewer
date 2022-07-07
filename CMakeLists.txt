cmake_minimum_required(VERSION 3.2)

set(VERSION_MAJOR 2)
set(VERSION_MINOR 1)
set(VERSION_PATCH 2)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

project(glslViewer 
            VERSION ${VERSION}
            LANGUAGES CXX )

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The compiled vera 
add_subdirectory(deps)

if(MSVC)
    add_definitions( -DPLATFORM_WINDOWS )
endif()

if (NO_X11 OR FORCE_GBM)
    add_compile_options(-std=c++14 -DGLM_FORCE_CXX14 -fpermissive -Wno-psabi -lpthread)
endif()

file(GLOB ROOT_SOURCE 
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/gl/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/types/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/tools/*.cpp"
)

add_executable(glslViewer ${ROOT_SOURCE})

# set_target_properties(glslViewer PROPERTIES
#     CXX_STANDARD 11
#     CXX_STANDARD_REQUIRED ON
# )

include_directories(deps/liblo)
target_include_directories(glslViewer PRIVATE deps)
target_link_libraries(glslViewer PRIVATE vera)
target_compile_definitions(glslViewer PRIVATE GLSLVIEWER_VERSION_MAJOR=${VERSION_MAJOR})
target_compile_definitions(glslViewer PRIVATE GLSLVIEWER_VERSION_MINOR=${VERSION_MINOR})
target_compile_definitions(glslViewer PRIVATE GLSLVIEWER_VERSION_PATCH=${VERSION_PATCH})

if (EMSCRIPTEN)    
    
    set(LFLAGS "${LFLAGS} -s USE_GLFW=3")
    # set(LFLAGS "${LFLAGS} -s FULL_ES2=1")
    # set(LFLAGS "${LFLAGS} -s FULL_ES3=1")
    # set(LFLAGS "${LFLAGS} -s MIN_WEBGL_VERSION=2")
    set(LFLAGS "${LFLAGS} -s MAX_WEBGL_VERSION=2")
    # set(LFLAGS "${LFLAGS} -s WEBGL2_BACKWARDS_COMPATIBILITY_EMULATION=1")

    set(LFLAGS "${LFLAGS} -s EXPORTED_FUNCTIONS='[ \"_command\", \"_setFrag\", \"_setVert\", \"_getFrag\", \"_getVert\", \"_main\" ]' ")
    set(LFLAGS "${LFLAGS} -s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\"]' ")

    set(LFLAGS "${LFLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(LFLAGS "${LFLAGS} -s NO_DYNAMIC_EXECUTION=1")

    # Copy the default index.html 
    file(COPY   assets/index.html DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

    # Add examples folder to the .data file system 
    file(COPY   examples DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
    # set(LFLAGS "${LFLAGS} --preload-file examples/2D/00_tests")
    # set(LFLAGS "${LFLAGS} --preload-file examples/2D/01_buffers")
    # set(LFLAGS "${LFLAGS} --preload-file examples/2D/02_double_buffers")
    # set(LFLAGS "${LFLAGS} --preload-file examples/2D/03_convolution_pyramids")
    set(LFLAGS "${LFLAGS} --preload-file examples/3D/00_pipeline/00_background.frag")
    set(LFLAGS "${LFLAGS} --preload-file examples/3D/00_pipeline/01_postprocessing.frag")
    set(LFLAGS "${LFLAGS} --preload-file examples/3D/01_lighting/00_gooch.frag")
    set(LFLAGS "${LFLAGS} --preload-file examples/3D/01_lighting/01_shadows.frag")
    set(LFLAGS "${LFLAGS} --preload-file examples/3D/01_lighting/02_cubemap.frag")
    set(LFLAGS "${LFLAGS} --preload-file examples/3D/01_lighting/04_fresnel.vert")
    set(LFLAGS "${LFLAGS} --preload-file examples/3D/01_lighting/04_fresnel.frag")
    set(LFLAGS "${LFLAGS} --preload-file examples/3D/01_lighting/uffizi_cross.jpg")
    set(LFLAGS "${LFLAGS} --preload-file examples/3D/01_lighting/uffizi_latlong.jpg")
    set(LFLAGS "${LFLAGS} --preload-file examples/3D/03_models/duck.glb")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/03_models/default.mtl")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/03_models/default.png")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/03_models/dragon.obj")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/04_vertex/pcl.ply")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/04_vertex/pcl.vert")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/04_vertex/pcl.frag")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/04_vertex/pcl_trail.frag")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/04_vertex/pcl_poisson_fill.frag")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/04_vertex/physarum.frag")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/04_vertex/physarum.vert")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/04_vertex/spin.vert")
    # set(LFLAGS "${LFLAGS} --preload-file examples/3D/04_vertex/stars.vert")
    
    set_target_properties(glslViewer PROPERTIES LINK_FLAGS "${LFLAGS}")
    target_link_libraries(glslViewer PRIVATE glfw)
    
else()
    find_package(Curses)
    if (CURSES_FOUND)
        target_compile_definitions(glslViewer PUBLIC SUPPORT_NCURSES)
        include_directories(${CURSES_INCLUDE_DIR})
        target_link_libraries(glslViewer PRIVATE ${CURSES_LIBRARY})
    endif()

    target_compile_definitions(glslViewer PUBLIC 
        SUPPORT_MULTITHREAD_RECORDING 
        SUPPORT_OSC
        # SUPPORT_PLY_BINARY
    )

    include(InstallRequiredSystemLibraries)
    set(CPACK_PACKAGE_NAME "glslViewer")
    set(CPACK_PACKAGE_CONTACT "Patricio Gonzalez Vivo <patriciogonzalezvivo@gmail.com>")
    set(CPACK_PACKAGE_VENDOR http://patriciogonzalezvivo.com)
    set(CPACK_PACKAGE_HOMEPAGE_URL https://github.com/patriciogonzalezvivo/glslViewer)
    set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GlslViewer is a flexible console-base OpenGL Sandbox to display 2D/3D GLSL shaders.")
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/DESCRIPTION")
    set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
    set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/assets/glslViewer.png")

    # resolve architecture. The reason i "change" i686 to i386 is that debian packages
    # require i386 so this is for the future
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
        set(SYSTEM_ARCH "i386")
    else()
        set(SYSTEM_ARCH ${CMAKE_SYSTEM_PROCESSOR})
    endif()

    # set a default system name - use CMake setting (Linux|Windows|...)
    set(SYSTEM_NAME ${CMAKE_SYSTEM_NAME})

    # for msvc the SYSTEM_NAME is set win32/64 instead of "Windows"
    if(MSVC)
        if(CMAKE_CL_64)
            set(SYSTEM_NAME "win64")
        else()
            set(SYSTEM_NAME "win32")
        endif()
    endif()
    set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${VERSION}-${SYSTEM_NAME}-${SYSTEM_ARCH}")

    if (MSVC)
	    target_link_libraries(glslViewer PRIVATE 
            lo_static 
            Ws2_32.lib 
            Iphlpapi.lib
        )

        install(TARGETS glslViewer)

        set(CPACK_GENERATOR "ZIP")

    else()
        target_link_libraries(glslViewer PRIVATE pthread dl lo_static)
        install(TARGETS glslViewer DESTINATION bin)

        if (NOT APPLE)
            target_link_libraries(glslViewer PRIVATE atomic)
            install(FILES "${PROJECT_SOURCE_DIR}/assets/glslViewer.png" DESTINATION share/pixmaps)
            install(FILES "${PROJECT_SOURCE_DIR}/assets/glslViewer.desktop" DESTINATION share/applications)

            # set(CPACK_GENERATOR "DEB")
            set(DEBSRC_BUILD_DEPENDS debhelper git cmake xorg-dev libgbm-dev libglu1-mesa-dev libavcodec-dev libavfilter-dev  libavdevice-dev libavformat-dev libavutil-dev libswscale-dev libv4l-dev libjpeg-dev libpng-dev libtiff-dev)
            set(DEBSRC_PACKAGE_DEPENDS git cmake ffmpeg libavcodec-extra libavfilter-extra)

            set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

            set(CPACK_DEBIAN_PACKAGE_DEPENDS)
            foreach(DEP ${DEBSRC_PACKAGE_DEPENDS})
            set(CPACK_DEBIAN_PACKAGE_DEPEND "${CPACK_DEBIAN_PACKAGE_DEPENDS}, ${DEP}")
            endforeach(DEP ${DEBSRC_PACKAGE_DEPENDS})  

            set(CPACK_DEBIAN_BUILD_DEPENDS)
            foreach(DEP ${DEBSRC_BUILD_DEPENDS})
            set(CPACK_DEBIAN_BUILD_DEPENDS "${CPACK_DEBIAN_BUILD_DEPENDS}, ${DEP}")
            endforeach(DEP ${DEBSRC_BUILD_DEPENDS})

            # find out and set DEB_ARCHITECTURE
            execute_process(
                COMMAND dpkg --print-architecture
                OUTPUT_VARIABLE DEB_ARCHITECTURE
                OUTPUT_STRIP_TRAILING_WHITESPACE            
            )
            message(STATUS "package_details.cmake: your architecture is ${DEB_ARCHITECTURE}")

            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${DEB_ARCHITECTURE})
            set(CPACK_DEBIAN_PACKAGE_SECTION graphics)
            set(CPACK_DEBIAN_PACKAGE_PRIORITY extra )
            set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Patricio Gonzalez Vivo <patriciogonzalezvivo@gmail.com>")
            set(CPACK_DEBIAN_DISTRIBUTION_NAME ubuntu )
            set(CPACK_DEBIAN_DISTRIBUTION_RELEASES bionic focal ) 
            set(CPACK_DEBIAN_CHANGELOG "  * Latest development version.")
            # set(CPACK_DEBIAN_RESOURCE_FILE_CHANGELOG ${PROJECT_SOURCE_DIR}/changelog)
            # set(CPACK_DEBIAN_UPDATE_CHANGELOG ON)
            set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${VERSION}-${DEB_ARCHITECTURE}")
        endif()
        
    endif()

    include(CPack)

endif()



