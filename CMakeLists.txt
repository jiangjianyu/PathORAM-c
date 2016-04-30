cmake_minimum_required(VERSION 3.5)
project(PathOram)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/main.cpp src/bucket.h src/filing.h src/oram.h src/crypt.h src/socket.h src/client.h src/server.h src/crypt.c src/socket.h src/server.c src/args.h src/main.c src/daemon.h src/bucket.c src/log.h src/log.c)
add_executable(PathOram ${SOURCE_FILES})