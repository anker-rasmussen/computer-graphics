cmake_minimum_required(VERSION 3.10)
project(Lab2)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(GLFW REQUIRED glfw3)

# Find FreeImage (not always available via pkg-config)
find_library(FREEIMAGE_LIBRARY freeimage)
find_path(FREEIMAGE_INCLUDE_DIR FreeImage.h)

if(NOT FREEIMAGE_LIBRARY OR NOT FREEIMAGE_INCLUDE_DIR)
    message(FATAL_ERROR "FreeImage library not found")
endif()

# Source files
set(SOURCES
    Lab2/Game.cpp
    Lab2/GameWindow.cpp
    Lab2/HighResolutionTimer.cpp
    Lab2/Shaders.cpp
    Lab2/Texture.cpp
    Lab2/Sphere.cpp
    Lab2/VertexBufferObject.cpp
    Lab2/MatrixStack.cpp
)

# Header files
set(HEADERS
    Lab2/Common.h
    Lab2/Game.h
    Lab2/GameWindow.h
    Lab2/HighResolutionTimer.h
    Lab2/Shaders.h
    Lab2/Texture.h
    Lab2/Sphere.h
    Lab2/VertexBufferObject.h
    Lab2/MatrixStack.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${FREEIMAGE_INCLUDE_DIR}
    Lab2/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${FREEIMAGE_LIBRARY}
)

# Copy data directory to build directory
file(COPY Lab2/data DESTINATION ${CMAKE_BINARY_DIR})

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
