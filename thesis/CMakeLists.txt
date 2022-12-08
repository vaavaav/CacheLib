cmake_minimum_required (VERSION 3.12)

project (thesis VERSION 0.1)

find_package(cachelib CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)

add_executable(test main.cpp)
target_link_libraries(test cachelib cppzmq)
add_executable(controller control_plane/controller.cpp)
target_link_libraries(controller cppzmq)