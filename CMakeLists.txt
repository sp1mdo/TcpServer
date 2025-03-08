cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(TcpServer VERSION 1.0 LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable target
add_executable(TcpServer
    main.cpp
    BaseServer.cpp
    Modbus.cpp
    Telnet.cpp
    Prompt.cpp
)

# Include directories for header files
target_include_directories(TcpServer PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_include_directories(${PROJECT_NAME} PRIVATE
    "inc"
)

target_compile_options(TcpServer PRIVATE
    -Wall
    -Werror
)
