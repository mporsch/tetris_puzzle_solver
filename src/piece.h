#pragma once

#include "hypervector.h"

#include <algorithm>
#include <numeric>

struct Piece : public hypervector<unsigned char, 2> {
  struct Type {
    char shape;
    unsigned char rotationCount;

    bool operator==(Type const &other) const {
      return ((shape == other.shape) &&
              (rotationCount == other.rotationCount));
    }
  };

  unsigned int id;
  Type type;

  Piece() = default;

  static Piece CreateI(unsigned int id) {
    return Piece(1, 4, id, 'i', 2);
  }

  static Piece CreateL(unsigned int id) {
    Piece piece(2, 3, id, 'l');
    piece.at(1, 0) = 0x00;
    piece.at(1, 1) = 0x00;
    return piece;
  }

  static Piece CreateJ(unsigned int id) {
    Piece piece(2, 3, id, 'j');
    piece.at(0, 0) = 0x00;
    piece.at(0, 1) = 0x00;
    return piece;
  }

  static Piece CreateT(unsigned int id) {
    Piece piece(3, 2, id, 't');
    piece.at(0, 1) = 0x00;
    piece.at(2, 1) = 0x00;
    return piece;
  }

  static Piece CreateZ(unsigned int id) {
    Piece piece(3, 2, id, 'z', 2);
    piece.at(0, 1) = 0x00;
    piece.at(2, 0) = 0x00;
    return piece;
  }

  static Piece CreateS(unsigned int id) {
    Piece piece(3, 2, id, 's', 2);
    piece.at(0, 0) = 0x00;
    piece.at(2, 1) = 0x00;
    return piece;
  }

  static Piece CreateO(unsigned int id) {
    return Piece(2, 2, id, 'o', 3);
  }

  bool IsBlockEmpty(size_t posX, size_t posY) const {
    return !this->at(posX, posY);
  }

  unsigned int GetBlockCount() const {
    return std::accumulate(
      this->begin(), this->end(), static_cast<unsigned int>(0),
      [](unsigned int sum, unsigned char filled) -> unsigned int {
        if(filled) {
          ++sum;
        }
        return sum;
      });
  }

  bool RotateRight() {
    if(type.rotationCount >= 3) {
      return false;
    } else {
      ++type.rotationCount;
    }

    // transpose
    hypervector<unsigned char, 2> transposed(this->sizeOf<1>(), this->sizeOf<0>());
    for(size_t x = 0; x < this->sizeOf<0>(); ++x) {
      for(size_t y = 0; y < this->sizeOf<1>(); ++y) {
        transposed.at(y, x) = this->at(x, y);
      }
    }

    this->resize(this->sizeOf<1>(), this->sizeOf<0>());

    // mirror vertically
    auto const offsetX = this->sizeOf<0>() - 1;
    for(size_t x = 0; x < this->sizeOf<0>(); ++x) {
      for(size_t y = 0; y < this->sizeOf<1>(); ++y) {
        this->at(x, y) = transposed.at(offsetX - x, y);
      }
    }

    return true;
  }

private:
  Piece(size_t sizeX,
        size_t sizeY,
        unsigned int id,
        char type,
        unsigned char rotationCount = 0)
    : hypervector<unsigned char, 2>(sizeX, sizeY, 0xFF)
    , id(id)
    , type({type, rotationCount}) {
  }
};
