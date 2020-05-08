cmake_minimum_required (VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project (tetris_puzzle_solver CXX)
add_executable (tetris_puzzle_solver tetris_puzzle_solver.cpp)
