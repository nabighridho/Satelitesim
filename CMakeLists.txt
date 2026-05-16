cmake_minimum_required(VERSION 3.10)
project(satellite_sim)

cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/Renderer.cpp
    src/shader.cpp
    src/textures.cpp
    src/Camera.cpp
    src/Window.cpp
    src/Mesh.cpp
    src/SceneObject.cpp
    src/SatelliteScene.cpp
    src/InfoPanel.cpp
    src/Starfield.cpp
    src/MainMenu.cpp
    src/vendor/stb_image.cpp
    src/vendor/glad.c
)

add_executable(satellite_sim ${SOURCES})

target_include_directories(satellite_sim PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
target_compile_options(satellite_sim PRIVATE -O2)

if (WIN32)
    target_compile_definitions(satellite_sim PRIVATE GLFW_STATIC)
    target_link_directories(satellite_sim PRIVATE
        ${PROJECT_SOURCE_DIR}/../lib
        ${PROJECT_SOURCE_DIR}/deps/assimp-build/lib
        ${PROJECT_SOURCE_DIR}/deps/assimp-build/contrib/zlib
    )
    target_link_libraries(satellite_sim assimp zlibstatic glfw3 opengl32 gdi32)
elseif (UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(assimp REQUIRED)
    target_link_libraries(satellite_sim glfw OpenGL::GL dl pthread assimp::assimp)
elseif (APPLE)
    find_library(OPENGL_LIBRARY OpenGL)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(satellite_sim ${OPENGL_LIBRARY} ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
endif()
