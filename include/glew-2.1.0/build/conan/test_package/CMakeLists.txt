project(GlewTest)
cmake_minimum_required(VERSION 3.0.0)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(testGlew main.c)
target_compile_definitions(testGlew PUBLIC "${CONAN_DEFINES}")
target_link_libraries(testGlew PUBLIC "${CONAN_LIBS}")
set_target_properties(testGlew PROPERTIES LINK_FLAGS "${CONAN_EXE_LINKER_FLAGS}")
