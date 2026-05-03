cmake_minimum_required(VERSION 3.10)

project(ClientServer)

# set(CMAKE_C_FLAGS_DEBUG "-g3 -O0")
# set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
# set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME}
               src/main.cpp)
