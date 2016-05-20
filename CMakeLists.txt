cmake_minimum_required(VERSION 3.0)
project(PathOram)

set(CMAKE_EXE_LINKER_FLAGS "-pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(SOURCE_FILES src/main.c src/bucket.h src/oram.h src/crypt.h src/socket.h src/client.h src/server.h src/crypt.c src/socket.h src/server.c src/args.h src/daemon.h src/bucket.c src/log.h src/log.c src/stash.h src/stash.c src/client.c src/args.c src/socket.c src/daemon.c src/performance.c)
set(SOURCE_LIB_FILE src/bucket.h src/oram.h src/crypt.h src/socket.h src/client.h src/crypt.c src/socket.h src/args.h src/daemon.h src/bucket.c src/log.h src/log.c src/stash.h src/stash.c src/client.c src/args.c src/socket.c src/daemon.c)
set(HEADER_FILES src/client.h src/bucket.h src/oram.h src/crypt.h src/socket.h src/server.h src/args.h src/daemon.h src/log.h src/stash.h src/uthash.h src/utlist.h src/performance.h)
add_executable(pathOram ${SOURCE_FILES})
add_executable(path_perforamce src/performance_test.c src/performance.c src/log.c)
target_link_libraries(pathOram sodium m)
add_library(pathoram SHARED ${SOURCE_LIB_FILE})
#target_include_directories(pathoram PRIVATE src)
set_target_properties(pathOram
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "build/bin"
        )
set_target_properties(path_perforamce
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "build/bin"
        )
set(LIBRARY_OUTPUT_PATH build/lib)

install(TARGETS pathOram pathoram
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        )
install(FILES ${HEADER_FILES} DESTINATION include/pathoram)
install(FILES src/pathoram.h DESTINATION include)