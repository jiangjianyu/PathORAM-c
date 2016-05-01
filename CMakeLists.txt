cmake_minimum_required(VERSION 3.0)
project(PathOram)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/main.c src/bucket.h src/oram.h src/crypt.h src/socket.h src/client.h src/server.h src/crypt.c src/socket.h src/server.c src/args.h src/daemon.h src/bucket.c src/log.h src/log.c src/stash.h src/stash.c src/client.c src/args.c)
add_executable(PathOram ${SOURCE_FILES})
