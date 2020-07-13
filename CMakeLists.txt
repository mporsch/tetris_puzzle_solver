cmake_minimum_required (VERSION 3.0.0)

project(tetris_puzzle_solver CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(hypervector REQUIRED)

add_executable(tetris_puzzle_solver tetris_puzzle_solver.cpp)
target_link_libraries(tetris_puzzle_solver hypervector)
