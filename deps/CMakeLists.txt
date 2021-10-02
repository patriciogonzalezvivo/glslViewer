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

    # # MINIAUDIO
    # # --------------------

    # # The split version is used to speed up compilation, the single c file gets compiled
    # # into a library, miniaudio.    
    # add_library(miniaudio STATIC "miniaudio/extras/miniaudio_split/miniaudio.c")
    # target_include_directories(miniaudio PUBLIC "miniaudio/extras/miniaudio_split")
    
    # # add some definitions to cut out stuff we don't need
    # #
    # target_compile_definitions(miniaudio PUBLIC
    #     MA_NO_WAV
    #     MA_NO_MP3           # No MP3 decoding
    #     MA_NO_FLAC          # No FLAC decoding
    #     MA_NO_GENERATION    # No waveform generators
    #     # MA_NO_CUSTOM        # No custom backend
    #     MA_NO_ENCODING 
    #     MA_NO_DECODING
    #     MINIAUDIO_IMPLEMENTATION
    # )

    # # miniaudio dependencies
    # # windows: none
    # # mac: none
    # # linux: pthreads, libm, libdl
    # # bsd: pthreads, libm
    # if (UNIX AND NOT APPLE)
    #    set(THREADS_PREFER_PTHREAD_FLAG ON)
    #    find_package(Threads REQUIRED)
    #    find_library(LIB_M m)
    #    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    #        find_library(LIB_DL dl)
    #        target_link_libraries(miniaudio PRIVATE ${LIB_DL})
    #    endif ()
    #    target_link_libraries(miniaudio PRIVATE Threads::Threads ${LIB_M})
    # endif ()
    
endif()

