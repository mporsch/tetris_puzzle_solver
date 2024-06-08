#pragma once

#include "color.h"
#include "piece.h"

#include "hypervector.h"

#include <algorithm>
#include <iostream>

struct Board : public hypervector<Color, 2> {
  Board(size_t sizeX, size_t sizeY, Color color = Color())
    : hypervector<Color, 2>(sizeX, sizeY, color) {
  }

  bool MayInsert(Piece const &piece, size_t posX, size_t posY) const {
    // check if piece exceeds the board
    if((posX + piece.sizeOf<0>() > this->sizeOf<0>()) ||
       (posY + piece.sizeOf<1>() > this->sizeOf<1>())) {
      return false;
    }

    for(size_t y = 0; y < piece.sizeOf<1>(); ++y) {
      for(size_t x = 0; x < piece.sizeOf<0>(); ++x) {
        // where there is a block in the piece, none must be in the board yet
        if(!piece.IsBlockEmpty(x, y) && !IsBlockEmpty(posX + x, posY + y)) {
          return false;
        }
      }
    }

    return true;
  }

  Board Insert(Piece const &piece, size_t posX, size_t posY) const {
    Board ret(*this);

    auto const color = Color::FromId(piece.id);
    for(size_t y = 0; y < piece.sizeOf<1>(); ++y) {
      for(size_t x = 0; x < piece.sizeOf<0>(); ++x) {
        // color the board blocks with the inserted piece
        if(!piece.IsBlockEmpty(x, y)) {
          ret.at(posX + x, posY + y) = color;
        }
      }
    }

    return ret;
  }

  bool IsSolved() const {
    // solved when no uncolored blocks remain
    return std::all_of(this->begin(), this->end(),
      [](Color const &color) -> bool {
        return color;
      });
  }

  bool IsBlockEmpty(size_t posX, size_t posY) const {
    return !this->at(posX, posY);
  }
};

std::ostream &operator<<(std::ostream &os, Board const &board) {
  for(size_t y = 0;; ++y) {
    for(size_t x = 0; x < board.sizeOf<0>(); ++x) {
      std::cout << board.at(x, y);
    }
    if(y < board.sizeOf<1>() - 1) {
      std::cout << colorReset << "\n";
    } else {
      break;
    }
  }
  os << colorReset;
  return os;
}
