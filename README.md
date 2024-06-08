# tetris_puzzle_solver
Solver for tetris-like puzzles as found in the game 'The Talos Principle'.

Printout uses ANSI terminal color codes and works in bash, Windows git bash or powershell (when wrapped in braces or piped to Write-Host).

Run e.g. with `$ ./tetris_puzzle_solver -w 8 -h 8 -T 4 -J 4 -L 1 -O 3 -Z 1 -S 1 -I 2` (8x8 board size, 4x T piece, 4x J piece, 1x L piece, 3x O piece, 1x Z piece, 1x S piece, 2x I piece).

![example](https://user-images.githubusercontent.com/1180665/43682949-dc416838-9882-11e8-8023-d44b5392c0f3.png)

Requires https://github.com/mporsch/hypervector.

Build using CMake.
