cmake_minimum_required(VERSION 3.0)

set(VCPKG_ROOT
    D:/vcpkg)

set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")

project(demo CXX C)

set(SOURCES
    src/main.cpp
    src/skybox.cpp
    src/Mirror.cpp
)

add_executable(demo
    ${SOURCES}
)

target_include_directories(demo
    PUBLIC
        include
        textures)

if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -Wall")
endif()

add_custom_command(TARGET demo PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shader shader)

find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

target_link_libraries(demo 
    PRIVATE
        opengl32
        glu32
        glad::glad
        glm::glm
        glfw
        assimp-vc142-mt
)