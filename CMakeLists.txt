cmake_minimum_required (VERSION 3.1.0)

project(tetris_puzzle_solver CXX)

option(DEBUG_BOARD_COLOR "" FALSE)
option(DEBUG_SOLVER_STEPS "" FALSE)
option(DEBUG_CCL_STEPS "" FALSE)
option(DEBUG_CCL_AGGREGATION "" FALSE)
option(DEBUG_SOLVABLE_CHECK "" FALSE)

find_package(hypervector REQUIRED)

add_executable(tetris_puzzle_solver tetris_puzzle_solver.cpp)
target_compile_features(tetris_puzzle_solver PRIVATE cxx_std_11)
target_compile_definitions(tetris_puzzle_solver PRIVATE
  $<$<BOOL:${DEBUG_BOARD_COLOR}>:DEBUG_BOARD_COLOR>
  $<$<BOOL:${DEBUG_SOLVER_STEPS}>:DEBUG_SOLVER_STEPS>
  $<$<BOOL:${DEBUG_CCL_STEPS}>:DEBUG_CCL_STEPS>
  $<$<BOOL:${DEBUG_CCL_AGGREGATION}>:DEBUG_CCL_AGGREGATION>
  $<$<BOOL:${DEBUG_SOLVABLE_CHECK}>:DEBUG_SOLVABLE_CHECK>
)
target_link_libraries(tetris_puzzle_solver hypervector)
