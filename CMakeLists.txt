cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
project(server)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

file(GLOB SRC src/*.cpp)
add_executable(server ${SRC})

add_subdirectory(fmt/)

target_include_directories(server PUBLIC include/)

target_link_libraries(server fmt Threads::Threads)
# target_link_libraries(server fmt::fmt)