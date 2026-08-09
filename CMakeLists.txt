cmake_minimum_required(VERSION 3.25) # CMake 3.25+ natively supports C++26
project(Cpp26Sandbox LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enforce C++26 standard
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Ensures clean, standard-compliant code

# Define the executable target
add_executable(sandbox src/main.cpp
                src/watcher.cpp
                src/utils.cpp
                src/parser.cpp
                src/ast.cpp)

if(WIN32)
    target_link_libraries(sandbox PRIVATE ws2_32)
endif()