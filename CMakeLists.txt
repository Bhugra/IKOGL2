cmake_policy(SET CMP0156 NEW)
cmake_minimum_required(VERSION 3.10)

# Project Name
project(ikogl2)

# Set C++ and C Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Add GLAD Library
add_library(glad ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include)

# Add GLFW Library
add_subdirectory(external/glfw)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Add Executable
add_executable(ikogl2 
    src/main.cpp
    src/NetworkClient.cpp
)

# Include Directories
target_include_directories(ikogl2 PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
)

# Link Libraries
target_link_libraries(ikogl2 PRIVATE
    glad 
    glfw 
    OpenGL::GL
    ws2_32
)
