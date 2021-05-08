cmake_minimum_required(VERSION 3.10)

project(server)
set(CMAKE_CXX_STANDARD 17)
file(GLOB SRC src/*.cpp)
add_executable(server ${SRC})

add_subdirectory(fmt/)

target_include_directories(server PUBLIC include/)

target_link_libraries(FMT)