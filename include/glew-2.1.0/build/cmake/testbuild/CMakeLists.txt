cmake_minimum_required(VERSION 2.8.12)
project(glew-cmake-test)

find_package(GLEW REQUIRED CONFIG)
find_package(GLEW REQUIRED CONFIG) # call twice to test multiple call
find_package(OpenGL REQUIRED)

add_executable(cmake-test main.c)
set_target_properties(cmake-test PROPERTIES DEBUG_POSTFIX _d)
target_link_libraries(cmake-test PRIVATE GLEW::GLEW ${OPENGL_LIBRARIES})
target_include_directories(cmake-test PRIVATE ${OPENGL_INCLUDE_DIR})

if(CMAKE_VERSION VERSION_LESS 3.0)
    set(cgex $<CONFIGURATION>)
else()
    set(cgex $<CONFIG>)
endif()

target_compile_definitions(cmake-test PRIVATE
    -DGLEW_CMAKE_TEST_CONFIG=${cgex}
    -DGLEW_CMAKE_TEST_TARGET_FILE_NAME=$<TARGET_FILE_NAME:GLEW::GLEW>
    -DGLEW_CMAKE_TEST_TARGET_TYPE=$<TARGET_PROPERTY:GLEW::GLEW,TYPE>
    )

install(TARGETS cmake-test DESTINATION bin)
