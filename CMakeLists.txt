cmake_minimum_required(VERSION 3.5)

set(VERSION_MAJOR 3)
set(VERSION_MINOR 3)
set(VERSION_PATCH 0)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

project(glslViewer 
        VERSION ${VERSION}
        LANGUAGES C CXX )

set(CMAKE_CXX_STANDARD 14)
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

set(CORE_HEADERS
    "${PROJECT_SOURCE_DIR}/src/core/glslViewer.h"
    "${PROJECT_SOURCE_DIR}/src/core/sceneRender.h"
    "${PROJECT_SOURCE_DIR}/src/core/uniforms.h"
    "${PROJECT_SOURCE_DIR}/src/core/tools/command.h"
    "${PROJECT_SOURCE_DIR}/src/core/tools/console.h"
    "${PROJECT_SOURCE_DIR}/src/core/tools/files.h"
    "${PROJECT_SOURCE_DIR}/src/core/tools/job.h"
    "${PROJECT_SOURCE_DIR}/src/core/tools/lockFreeQueue.h"
    "${PROJECT_SOURCE_DIR}/src/core/tools/record.h"
    "${PROJECT_SOURCE_DIR}/src/core/tools/text.h"
    "${PROJECT_SOURCE_DIR}/src/core/tools/tracker.h"
)

set(CORE_SOURCES 
    "${PROJECT_SOURCE_DIR}/src/core/glslViewer.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/sceneRender.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/uniforms.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/tools/console.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/tools/record.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/tools/text.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/tools/tracker.cpp"
)

add_executable(glslViewer 
    "${PROJECT_SOURCE_DIR}/src/main.cpp"
    ${CORE_SOURCES}
)


include_directories(deps/liblo)
target_include_directories(glslViewer PRIVATE deps)
target_link_libraries(glslViewer PRIVATE vera)
# target_compile_definitions(vera PUBLIC SUPPORT_PLY_BINARY)
target_compile_definitions(glslViewer PRIVATE DEBUG)
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
    # set(LFLAGS "${LFLAGS} -s NO_DISABLE_EXCEPTION_CATCHING")

    # Copy the default index.html 
    file(COPY   assets/index.html DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

    # Add examples folder to the .data file system 
    file(COPY   examples DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
    set(LFLAGS "${LFLAGS} --preload-file examples")

    set_target_properties(glslViewer PROPERTIES LINK_FLAGS "${LFLAGS}")
    target_link_libraries(glslViewer PRIVATE glfw webxr)
    
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
    )

    include(GNUInstallDirs)
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
        install(TARGETS glslViewer DESTINATION ${CMAKE_INSTALL_BINDIR})
        
        if (NOT APPLE)
            target_link_libraries(glslViewer PRIVATE atomic)

            # Install glslViewer Icon
            install(FILES "${PROJECT_SOURCE_DIR}/assets/glslViewer.png" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps)
            install(FILES "${PROJECT_SOURCE_DIR}/assets/glslViewer.desktop" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
            
            # Install supported MIME file types by GlslViewer
            install(FILES "${PROJECT_SOURCE_DIR}/assets/glslViewer-types.xml" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mime/packages)
            find_program(XDG-MIME_EXECUTABLE xdg-mime)
            install(CODE "execute_process(COMMAND ${XDG-MIME_EXECUTABLE} install --novendor ${CMAKE_INSTALL_FULL_DATAROOTDIR}/mime/packages/glslViewer-types.xml)")
            install(CODE "execute_process(COMMAND ${XDG-MIME_EXECUTABLE} default ${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications/glslViewer.desktop model/lst)")
            install(CODE "execute_process(COMMAND ${XDG-MIME_EXECUTABLE} default ${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications/glslViewer.desktop model/ply)")
            install(CODE "execute_process(COMMAND ${XDG-MIME_EXECUTABLE} default ${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications/glslViewer.desktop model/obj)")
            install(CODE "execute_process(COMMAND ${XDG-MIME_EXECUTABLE} default ${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications/glslViewer.desktop model/gltf-binary)")
            install(CODE "execute_process(COMMAND ${XDG-MIME_EXECUTABLE} default ${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications/glslViewer.desktop model/gltf+json)")
            find_program(UPDATE_MIME_DATABASE update-mime-database)
            install(CODE "execute_process(COMMAND ${UPDATE_MIME_DATABASE} ${CMAKE_INSTALL_FULL_DATAROOTDIR}/mime )")
            find_program(UPDATE_DESKTOP_DATABASE update-desktop-database)
            install(CODE "execute_process(COMMAND ${UPDATE_DESKTOP_DATABASE} ${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications)")
            
            # Add a thumbnailer for 3D Models
            install(FILES "${PROJECT_SOURCE_DIR}/assets/glslViewer.thumbnailer" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/thumbnailers)
            install(FILES "${PROJECT_SOURCE_DIR}/assets/glslThumbnailer.sh"
                    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
                    DESTINATION ${CMAKE_INSTALL_BINDIR}
                    RENAME "glslThumbnailer")

            # Add ScreenSaver script
            install(FILES "${PROJECT_SOURCE_DIR}/assets/glslScreenSaver.py"
                    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
                    DESTINATION ${CMAKE_INSTALL_BINDIR}
                    RENAME "glslScreenSaver")
            install(FILES "${PROJECT_SOURCE_DIR}/assets/glslScreenSaver.frag" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/glslViewer")
            install(FILES "${PROJECT_SOURCE_DIR}/assets/glslScreenSaver.yaml" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/glslViewer")

            find_program(XDG-DESKTOP-MENU_EXECUTABLE xdg-desktop-menu)
            execute_process(COMMAND ${XDG-DESKTOP-MENU_EXECUTABLE} install --novendor ${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications/glslViewer.desktop)

            # set(CPACK_GENERATOR "DEB")
            set(DEBSRC_BUILD_DEPENDS debhelper git cmake xorg-dev libgbm-dev libglu1-mesa-dev libavcodec-dev libavfilter-dev  libavdevice-dev libavformat-dev libavutil-dev libswscale-dev libv4l-dev libjpeg-dev libpng-dev libtiff-dev)
            set(DEBSRC_PACKAGE_DEPENDS ffmpeg libavcodec-extra libavfilter-extra xorg-x11-server-Xvfb )

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

if (PYTHON_BINDINGS)
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter Development)

    # pybind11
    set(DEPS_FOLDER "${PROJECT_SOURCE_DIR}/deps")
    add_subdirectory("${DEPS_FOLDER}/pybind11")
    include_directories(${pybind11_INCLUDE_DIRS})

    set(PYBIND_INCLUDES
        "${PROJECT_SOURCE_DIR}/src/python/engine.h"
        "${PROJECT_SOURCE_DIR}/src/python/engine.cpp"
        "${PROJECT_SOURCE_DIR}/src/python/headless.h"
        "${PROJECT_SOURCE_DIR}/src/python/headless.cpp"
        "${PROJECT_SOURCE_DIR}/src/python/bindings.cpp"
        ${CORE_SOURCES}
        ${CORE_HEADERS}
    )

    pybind11_add_module(PyGlslViewer ${PYBIND_INCLUDES})
    target_compile_definitions(PyGlslViewer PUBLIC PYTHON_RENDER)
    target_compile_definitions(PyGlslViewer PRIVATE GLSLVIEWER_VERSION_MAJOR=${VERSION_MAJOR})
    target_compile_definitions(PyGlslViewer PRIVATE GLSLVIEWER_VERSION_MINOR=${VERSION_MINOR})
    target_compile_definitions(PyGlslViewer PRIVATE GLSLVIEWER_VERSION_PATCH=${VERSION_PATCH})
    target_include_directories(PyGlslViewer PUBLIC ${DEPS_FOLDER})
    target_link_libraries(PyGlslViewer PRIVATE vera pybind11::module)

endif()