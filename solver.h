#pragma once

#include "board.h"
#include "ccl.h"
#include "piece.h"

#include "hypervector.h"

#include <algorithm>
#include <iostream>
#include <vector>

struct Solver {
  using BlackList = hypervector<std::vector<Piece::Type>, 2>;

  unsigned int pieceMinimumBlockCount;

  Solver()
    : pieceMinimumBlockCount(0) {
  }

  // recursive function performing depth-first tree search
  // operates on its own copy of board and blacklist
  // but shares pieces and modifies their order
  bool Solve(Board board,
             BlackList blackList,
             std::vector<Piece>::iterator firstPiece,
             std::vector<Piece>::iterator lastPiece) const {
    auto const piecesCount = std::distance(firstPiece, lastPiece);
    if(!piecesCount) {
      // all pieces have been placed
      std::cout << board << "\n\n";
      return board.IsSolved();
    } else {
#ifdef DEBUG_SOLVER_STEPS
      static unsigned long long iterationCount = 0;
      std::cout << board
        << " Solver: pieces remaining " << piecesCount
        << ", iteration " << iterationCount++ << std::endl;
#endif
    }

    // determine the remaining board blocks,
    // especially the smallest, most restrictive remaining space
    ConnectedComponentLabeler ccl(board);

    bool isSolvable = (ccl.GetMinSize() >= pieceMinimumBlockCount);
    if(!isSolvable) {
#ifdef DEBUG_SOLVABLE_CHECK
      std::cout << board << " Solver: unsolvable" << std::endl;
#endif
      return false;
    }

    auto const sub = ccl.GetMin();

    for(ptrdiff_t piecesOrder = 0; piecesOrder < piecesCount; ++piecesOrder) {
      auto piece = *firstPiece;
      do {
        for(size_t y = 0; y < sub.board.size(1); ++y) {
          for(size_t x = 0; x < sub.board.size(0); ++x) {
            if(sub.board.MayInsert(piece, x, y)) {
              auto &&blackListEntry = blackList.at(x + sub.offsetX, y + sub.offsetY);
              bool isBlackListed = (end(blackListEntry) != std::find(
                begin(blackListEntry), end(blackListEntry), piece.type));
              if(!isBlackListed) {
                // next iteration with updated board and pieces list
                if(Solve(board.Insert(piece, x + sub.offsetX, y + sub.offsetY),
                         blackList,
                         std::next(firstPiece),
                         lastPiece)) {
                  return true;
                } else {
                  // do not try the same type in the same spot again, if we have multiple
                  blackListEntry.push_back(piece.type);
                }
              }
            }
          }
        }
      } while(piece.RotateRight()); // try again with this piece rotated

      // try another order of pieces (rotate left)
      std::rotate(firstPiece, std::next(firstPiece), lastPiece);
    }

    return false;
  }
};
