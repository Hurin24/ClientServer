cmake_minimum_required(VERSION 3.10)

project(ClientServer)

set(CMAKE_C_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_CXX_STANDARD 17)

add_executable(TCPServer
               src/TCPServer.cpp
               src/TCPServerApplication/TCPServerApplication.cpp
               src/TCPServerListenSession/TCPServerListenSession.cpp
               src/TCPServerSession/TCPServerSession.cpp
               src/TCPServerResponse/TCPServerResponse.cpp
               src/TCPClientRequest/TCPClientRequest.cpp
               src/TCPSocket/TCPSocket.cpp)

add_executable(TCPClient
               src/TCPClient.cpp
               src/TCPClientApplication/TCPClientApplication.cpp
               src/TCPClientSession/TCPClientSession.cpp
               src/TCPClientRequest/TCPClientRequest.cpp
               src/TCPServerResponse/TCPServerResponse.cpp
               src/TCPSocket/TCPSocket.cpp)