cmake_minimum_required(VERSION 3.10)

option(WITH_TOOLS "Enable building tools." OFF)
option(WITH_TESTS "Enable building tests." OFF)
option(WITH_EXAMPLES "Enable building examples." OFF)
option(WITH_CPP_TESTS "Enable building C++ wrapper tests." OFF)
option(WITH_STATIC "Enable building static library." ON)
option(THREADING "Build with threading support." ON)

if (WITH_STATIC)
  message(STATUS "If you are using the static library build, please keep in mind (and inform yourself of the implications) that liblo is licensed with LGPL v2.1+.")
endif()

include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(TestBigEndian)
include(GNUInstallDirs)

set(PROJECT liblo)
set(PACKAGE_NAME "${PROJECT}")
if (WITH_CPP_TESTS)
  project(${PROJECT} LANGUAGES C CXX)
else()
  project(${PROJECT} LANGUAGES C)
endif()

if(NOT CMAKE_VERSION VERSION_LESS 3.1.0)
    set_property(TARGET ${LIBRARY} PROPERTY C_STANDARD 11)
    if (WITH_CPP_TESTS)
        set_property(TARGET ${LIBRARY} PROPERTY CXX_STANDARD 11)
    endif()
endif()

set(CMAKE_VERBOSE_MAKEFILE OFF)

if (MSVC)
  add_definitions(
    /D_CRT_SECURE_NO_WARNINGS
    /D_CRT_SECURE_NO_DEPRECATE
    /D_CRT_NONSTDC_NO_DEPRECATE
    /D_WINSOCK_DEPRECATED_NO_WARNINGS)

  # On Windows we do not wish the default behaviour of removing "lib"
  # from "liblo.dll".
  if ("${CMAKE_SHARED_LIBRARY_PREFIX}" STREQUAL "")
    set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
  endif()
  if ("${CMAKE_STATIC_LIBRARY_PREFIX}" STREQUAL "")
    set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
  endif()

  # For Visual Studio, ensure import lib can be distinguished from
  # static lib
  if ("${CMAKE_IMPORT_LIBRARY_PREFIX}" STREQUAL "")
    set(CMAKE_IMPORT_LIBRARY_PREFIX "lib")
  endif()
  if ("${CMAKE_IMPORT_LIBRARY_SUFFIX}" STREQUAL ".lib")
    set(CMAKE_STATIC_LIBRARY_SUFFIX "_static.lib")
  endif()
endif()

# set(LIBRARY_SHARED lo_shared)
if (WITH_STATIC)
  set(LIBRARY_STATIC lo_static)
endif()
if (WITH_TOOLS)
  set(OSCDUMP oscdump)
  set(OSCSEND oscsend)
endif()
if (WITH_TESTS)
  set(TESTLO testlo)
  set(SUBTEST subtest)
  if (WITH_CPP_TESTS)
    set(CPPTEST cpp_test)
  endif()
else()
  set(WITH_CPP_TESTS OFF)
endif()
if (WITH_EXAMPLES)
  set(EXAMPLE_CLIENT example_client)
  set(EXAMPLE_SERVER example_server)
  set(EXAMPLE_TCP_ECHO_SERVER example_tcp_echo_server)
  set(NONBLOCKING_SERVER_EXAMPLE nonblocking_server_example)
endif()

set(TOOLS ${OSCDUMP} ${OSCSEND})
set(TESTS ${TESTLO} ${SUBTEST})
list(APPEND TESTS ${CPPTEST})
set(EXAMPLES ${EXAMPLE_CLIENT} ${EXAMPLE_SERVER}
  ${EXAMPLE_TCP_ECHO_SERVER} ${NONBLOCKING_SERVER_EXAMPLE})
set(PROGRAMS ${TOOLS} ${TESTS} ${EXAMPLES})

set(LIBRARY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/address.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/blob.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/bundle.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/message.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/method.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/pattern_match.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/send.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/server.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/timetag.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/version.c
    )

set(LIBRARY_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../lo/lo_cpp.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lo/lo_errors.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lo/lo_lowlevel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lo/lo_macros.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lo/lo_osc_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lo/lo_serverthread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lo/lo_throw.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lo/lo_types.h
    ${CMAKE_CURRENT_BINARY_DIR}/lo/lo_endian.h
    ${CMAKE_CURRENT_BINARY_DIR}/lo/lo.h
    )

set(OSCDUMP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/oscdump.c)
set(OSCSEND_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/oscsend.c)
set(TESTLO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../src/testlo.c)
set(SUBTEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../src/subtest.c)
if (WITH_CPP_TESTS)
    set(CPPTEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../src/cpp_test.cpp)
endif()
set(EXAMPLE_CLIENT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/example_client.c)
set(EXAMPLE_SERVER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/example_server.c)
set(EXAMPLE_TCP_ECHO_SERVER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/example_tcp_echo_server.c)
set(NONBLOCKING_SERVER_EXAMPLE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/nonblocking_server_example.c)

check_symbol_exists(poll poll.h HAVE_POLL)
check_symbol_exists(select sys/select.h HAVE_SELECT)
if(NOT HAVE_POLL AND NOT HAVE_SELECT)
  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(HAVE_SELECT 1)
    message(STATUS "Windows configuration: Assuming select exists.")
  else()
    message(FATAL_ERROR "Neither select nor poll was found.")
  endif()
endif()
check_symbol_exists(getifaddrs "sys/types.h;ifaddrs.h" HAVE_GETIFADDRS)
check_symbol_exists(inet_pton "arpa/inet.h" HAVE_INET_PTON)

if(THREADING)
  if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    option(CMAKE_THREAD_PREFER_PTHREAD "Prefer pthread to other thread libraries." OFF)
  else()
    option(CMAKE_THREAD_PREFER_PTHREAD "Prefer pthread to other thread libraries." ON)
  endif()
  include(FindThreads)
  if (NOT Threads_FOUND)
    message(FATAL_ERROR "No supported thread library found.")
  endif()
  if(CMAKE_USE_WIN32_THREADS_INIT AND NOT CMAKE_THREAD_PREFER_PTHREAD)
    set(HAVE_WIN32_THREADS ON)
  elseif(CMAKE_USE_PTHREADS_INIT)
    set(HAVE_LIBPTHREAD ON)
  endif()
  set(LIBRARY_SOURCES ${LIBRARY_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/../src/server_thread.c)
endif()

set(BUILD_LANGUAGE C CACHE STRING "Build language (C or CXX)")
mark_as_advanced(BUILD_LANGUAGE)
set_source_files_properties(
    ${LIBRARY_SOURCES} ${OSCDUMP_SOURCES} ${OSCSEND_SOURCES}
    ${TESTLO_SOURCES} ${EXAMPLE_CLIENT_SOURCES}
    ${EXAMPLE_SERVER_SOURCES} ${EXAMPLE_TCP_ECHO_SERVER_SOURCES}
    ${NONBLOCKING_SERVER_EXAMPLE_SOURCES}
    PROPERTIES LANGUAGE ${BUILD_LANGUAGE})

if (WITH_CPP_TESTS)
    set_source_files_properties(${CPPTEST_SOURCES}
        PROPERTIES LANGUAGE CXX)
endif()

# Library
# add_library(${LIBRARY_SHARED} SHARED ${LIBRARY_SOURCES})
if (WITH_STATIC)
  add_library(${LIBRARY_STATIC} STATIC ${LIBRARY_SOURCES})
endif()

# Tools
if (WITH_TOOLS)
  add_executable(${OSCDUMP} ${OSCDUMP_SOURCES})
  add_executable(${OSCSEND} ${OSCSEND_SOURCES})
endif()

# Tests
if (WITH_TESTS)
  add_executable(${TESTLO} ${TESTLO_SOURCES})
  add_executable(${SUBTEST} ${SUBTEST_SOURCES})
endif()
if (WITH_CPP_TESTS)
  add_executable(${CPPTEST} ${CPPTEST_SOURCES})
endif()

# Examples
if (WITH_EXAMPLES)
  add_executable(${EXAMPLE_CLIENT} ${EXAMPLE_CLIENT_SOURCES})
  add_executable(${EXAMPLE_SERVER} ${EXAMPLE_SERVER_SOURCES})
  add_executable(${EXAMPLE_TCP_ECHO_SERVER} ${EXAMPLE_TCP_ECHO_SERVER_SOURCES})
  add_executable(${NONBLOCKING_SERVER_EXAMPLE} ${NONBLOCKING_SERVER_EXAMPLE_SOURCES})
endif()

if(THREADING)
    set(ENABLE_THREADS 1)

    # target_link_libraries(${LIBRARY_SHARED} Threads::Threads)
    if (WITH_STATIC)
      target_link_libraries(${LIBRARY_STATIC} Threads::Threads)
    endif()

    set(THREADS_INCLUDE
      "#include \"lo/lo_serverthread.h\"")
else()
    set(THREADS_INCLUDE
      "/* lo/lo_serverthread.h unavailable (THREADING=OFF) */")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    # target_link_libraries(${LIBRARY_SHARED} "user32" "wsock32" "ws2_32" "iphlpapi")
    target_link_libraries(${LIBRARY_STATIC} "user32" "wsock32" "ws2_32" "iphlpapi")
    if(THREADING AND Threads_FOUND)
      # target_link_libraries(${LIBRARY_SHARED} ${CMAKE_THREAD_LIBS_INIT})
      if (WITH_STATIC)
        target_link_libraries(${LIBRARY_STATIC} ${CMAKE_THREAD_LIBS_INIT})
      endif()
    endif()

    # set_target_properties(${LIBRARY_SHARED} PROPERTIES
        # COMPILE_DEFINITIONS "LIBLO_DLL")
    if (WITH_STATIC)
      set_target_properties(${LIBRARY_STATIC} PROPERTIES
          COMPILE_DEFINITIONS "LIBLO_LIB")
    endif()

    set(DLLNAME "")
    if(THREADING)
        set(DEFTHREADS "")
    else()
        set(DEFTHREADS ";;")
    endif()
else()
  # target_link_libraries(${LIBRARY_SHARED} "m")
  if (WITH_STATIC)
    target_link_libraries(${LIBRARY_STATIC} "m")
  endif()
endif()

set(LO_INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
# target_include_directories(${LIBRARY_SHARED} PUBLIC ${LO_INCLUDE_DIRS})
if (WITH_STATIC)
  target_include_directories(${LIBRARY_STATIC} PUBLIC ${LO_INCLUDE_DIRS})
endif()

foreach(PROG ${PROGRAMS})
  target_include_directories(${PROG} PUBLIC ${LO_INCLUDE_DIRS})
  # target_link_libraries(${PROG} PUBLIC ${LIBRARY_SHARED})
endforeach(PROG)

foreach(PROG ${TOOLS})
  set_property(TARGET ${PROG} PROPERTY RUNTIME_OUTPUT_DIRECTORY "tools")
endforeach(PROG)

foreach(PROG ${TESTS})
  set_property(TARGET ${PROG} PROPERTY RUNTIME_OUTPUT_DIRECTORY "tests")
endforeach(PROG)

foreach(PROG ${EXAMPLES})
  set_property(TARGET ${PROG} PROPERTY RUNTIME_OUTPUT_DIRECTORY "examples")
endforeach(PROG)

if (WITH_CPP_TESTS)
    set_property(TARGET ${CPPTEST} PROPERTY CXX_STANDARD 11)
endif()

# parse version info from configure.ac
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../configure.ac"
    CONFIGURE_AC_LINE REGEX "AC_INIT\\(\\[liblo\\].*\\)")
string(REGEX MATCHALL "[0-9]+" CONFIGURE_AC_LIST ${CONFIGURE_AC_LINE})
list(GET CONFIGURE_AC_LIST 0 CONFIGURE_AC_MAJOR)
list(GET CONFIGURE_AC_LIST 1 CONFIGURE_AC_MINOR)
set(PACKAGE_VERSION "${CONFIGURE_AC_MAJOR}.${CONFIGURE_AC_MINOR}")

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../configure.ac"
    CONFIGURE_AC_LINE REGEX "m4_define\\(\\[lt_current\\], [0-9]+\\)")
string(REGEX MATCHALL "[0-9]+" CONFIGURE_AC_LIST ${CONFIGURE_AC_LINE})
list(GET CONFIGURE_AC_LIST 1 CONFIGURE_AC_CURRENT)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../configure.ac"
    CONFIGURE_AC_LINE REGEX "m4_define\\(\\[lt_revision\\], [0-9]+\\)")
string(REGEX MATCHALL "[0-9]+" CONFIGURE_AC_LIST ${CONFIGURE_AC_LINE})
list(GET CONFIGURE_AC_LIST 1 CONFIGURE_AC_REVISION)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../configure.ac"
    CONFIGURE_AC_LINE REGEX "m4_define\\(\\[lt_age\\], [0-9]+\\)")
string(REGEX MATCHALL "[0-9]+" CONFIGURE_AC_LIST ${CONFIGURE_AC_LINE})
list(GET CONFIGURE_AC_LIST 1 CONFIGURE_AC_AGE)
set(LO_SO_VERSION
    "{${CONFIGURE_AC_CURRENT}, ${CONFIGURE_AC_REVISION}, ${CONFIGURE_AC_AGE}}")

math(EXPR CURRENT_MINUS_AGE "${CONFIGURE_AC_CURRENT} - ${CONFIGURE_AC_AGE}")
# set_target_properties(${LIBRARY_SHARED} PROPERTIES
#     VERSION   "${CURRENT_MINUS_AGE}.${CONFIGURE_AC_AGE}.${CONFIGURE_AC_REVISION}"
#     SOVERSION "${CURRENT_MINUS_AGE}"
#     OUTPUT_NAME "lo"
#     )
if (WITH_STATIC)
  set_target_properties(${LIBRARY_STATIC} PROPERTIES
      OUTPUT_NAME "lo"
      )
endif()

if(NOT DEFINED PRINTF_LL)
    message(STATUS "Check how to print long long int")
    try_compile(PRINTF_LL_AVAILABLE ${CMAKE_CURRENT_BINARY_DIR}
        SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test-printf-ll.c")
    if(PRINTF_LL_AVAILABLE)
        set(PRINTF_LL "ll" CACHE INTERNAL "")
    else()
        try_compile(PRINTF_LL_AVAILABLE ${CMAKE_CURRENT_BINARY_DIR}
            SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test-printf-I64.c")
        if(PRINTF_LL_AVAILABLE)
            set(PRINTF_LL "I64" CACHE INTERNAL "")
        else()
            message(FATAL_ERROR "printf doesn't support long long int")
        endif()
    endif()
endif()

if(MSVC)
    set(VERSION "${CONFIGURE_AC_CURRENT}.${CONFIGURE_AC_REVISION}")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/config-msvc.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/config.h)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/liblo.def.in
        ${CMAKE_CURRENT_BINARY_DIR}/src/liblo.def)
    if (WITH_STATIC)
      set_target_properties(${LIBRARY_STATIC} PROPERTIES
          LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_BINARY_DIR}/src/liblo.def\"")
    endif()
    # set_target_properties(${LIBRARY_SHARED} PROPERTIES
    #     LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_BINARY_DIR}/src/liblo.def\"")
else()
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/config.h)
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../lo/lo.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/lo/lo.h)
test_big_endian(LO_BIGENDIAN)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../lo/lo_endian.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/lo/lo_endian.h)
configure_file(
    ${PROJECT}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT}Config.cmake)

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix "\${prefix}")
set(libdir "\${exec_prefix}/lib")
set(includedir "\${prefix}/include")
set(LIBPTHREAD ${CMAKE_THREAD_LIBS_INIT})
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../${PROJECT}.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT}.pc
    @ONLY)
add_definitions(-DHAVE_CONFIG_H)

# install(
#     TARGETS ${LIBRARY_STATIC} ${LIBRARY_SHARED} ${OSCDUMP} ${OSCSEND}
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib
#     RUNTIME DESTINATION bin)
# install(
#     FILES ${LIBRARY_HEADERS}
#     DESTINATION include/lo)
# install(
#     FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT}Config.cmake
#     DESTINATION lib/cmake/${PROJECT})
# install(
#     FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT}.pc
#     DESTINATION lib/pkgconfig)
