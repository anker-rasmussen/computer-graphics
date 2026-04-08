cmake_minimum_required(VERSION 3.10)
project(OpenGLTemplate)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(GLFW REQUIRED glfw3)
find_package(Freetype REQUIRED)
pkg_check_modules(ASSIMP REQUIRED assimp)

# Find FreeImage
find_library(FREEIMAGE_LIBRARY freeimage)
find_path(FREEIMAGE_INCLUDE_DIR FreeImage.h)

if(NOT FREEIMAGE_LIBRARY OR NOT FREEIMAGE_INCLUDE_DIR)
    message(FATAL_ERROR "FreeImage library not found. Install with: sudo pacman -S freeimage")
endif()

# Optional FMOD support
option(USE_FMOD "Enable FMOD audio support" ON)

# Source files
set(SOURCES
    OpenGLTemplate/Audio.cpp
    OpenGLTemplate/Camera.cpp
    OpenGLTemplate/Cubemap.cpp
    OpenGLTemplate/FreeTypeFont.cpp
    OpenGLTemplate/Game.cpp
    OpenGLTemplate/GameWindow.cpp
    OpenGLTemplate/HighResolutionTimer.cpp
    OpenGLTemplate/MatrixStack.cpp
    OpenGLTemplate/OpenAssetImportMesh.cpp
    OpenGLTemplate/Plane.cpp
    OpenGLTemplate/Shaders.cpp
    OpenGLTemplate/Skybox.cpp
    OpenGLTemplate/Ship.cpp
    OpenGLTemplate/ParticleSystem.cpp
    OpenGLTemplate/Sphere.cpp
    OpenGLTemplate/AnimatedMesh.cpp
    OpenGLTemplate/Bridge.cpp
    OpenGLTemplate/CatmullRom/CatmullRom.cpp
    OpenGLTemplate/FrameBufferObject.cpp
    OpenGLTemplate/Texture.cpp
    OpenGLTemplate/VertexBufferObject.cpp
    OpenGLTemplate/VertexBufferObjectIndexed.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${ASSIMP_INCLUDE_DIRS}
    ${FREEIMAGE_INCLUDE_DIR}
    OpenGLTemplate
    OpenGLTemplate/CatmullRom
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${ASSIMP_LIBRARIES}
    ${FREEIMAGE_LIBRARY}
)

if(USE_FMOD)
    find_path(FMOD_INCLUDE_DIR fmod.hpp PATHS /usr/include/fmodex)
    find_library(FMOD_LIBRARY NAMES fmodex64 fmodex)
    if(FMOD_INCLUDE_DIR AND FMOD_LIBRARY)
        target_compile_definitions(${PROJECT_NAME} PRIVATE USE_FMOD)
        target_include_directories(${PROJECT_NAME} PRIVATE ${FMOD_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME} ${FMOD_LIBRARY})
        message(STATUS "FMOD enabled: ${FMOD_LIBRARY}")
    else()
        message(WARNING "USE_FMOD is ON but FMOD not found, building without audio")
    endif()
endif()

# Symlink resources to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/OpenGLTemplate/resources
        ${CMAKE_BINARY_DIR}/resources
    COMMENT "Symlinking resources directory"
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
