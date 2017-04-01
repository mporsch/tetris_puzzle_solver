#include <vector>
#include <iostream>
#include "../hypervector/hypervector.h"

class Piece : hypervector<bool, 2> {
public:
  Piece()
    : hypervector<bool, 2>() {
  };

  static Piece CreatePieceI() {
    return Piece(1, 4, true);
  }

  static Piece CreatePieceL() {
    Piece piece(2, 3, true);
    piece.at(1, 0) = false;
    piece.at(1, 1) = false;
    return piece;
  }

  static Piece CreatePieceJ() {
    Piece piece(2, 3, true);
    piece.at(0, 0) = false;
    piece.at(0, 1) = false;
    return piece;
  }

  static Piece CreatePieceT() {
    Piece piece(3, 2, true);
    piece.at(0, 1) = false;
    piece.at(2, 1) = false;
    return piece;
  }

  static Piece CreatePieceZ() {
    Piece piece(3, 2, true);
    piece.at(0, 1) = false;
    piece.at(2, 0) = false;
    return piece;
  }

  static Piece CreatePieceS() {
    Piece piece(3, 2, true);
    piece.at(0, 0) = false;
    piece.at(2, 1) = false;
    return piece;
  }

  static Piece CreatePieceO() {
    return Piece(2, 2, true);
  }

  void RotateRight() {
    // transpose
    Piece transposed(this->size(1), this->size(0));
    for(size_t x = 0; x < this->size(0); ++x) {
      for(size_t y = 0; y < this->size(1); ++y) {
        transposed.at(y, x) = this->at(x, y);
      }
    }

    this->resize(this->size(1), this->size(0));

    // mirror vertically
    for(size_t x = 0; x < this->size(0); ++x) {
      for(size_t y = 0; y < this->size(1); ++y) {
        this->at(x, y) = transposed.at(this->size(0) - x - 1, y);
      }
    }
  }

private:
  Piece(size_type countX, size_type countY)
    : hypervector<bool, 2>(countX, countY) {
  };

  Piece(size_type countX, size_type countY, bool value)
    : hypervector<bool, 2>(countX, countY, bool(value)) { // ???
  };
};

using Board = hypervector<char, 2>;

std::ostream &operator<<(std::ostream &os, Board const &board) {
  for(size_t y = 0; y < board.size(1); ++y) {
    for(size_t x = 0; x < board.size(0); ++x) {
      std::cout << board.at(x, y);
    }
    std::cout << "\n";
  }
  return os;
}

int main(int argc, char **argv) {
  size_t dimX = 4;
  size_t dimY = 3;
  Board board(dimX, dimY, ' ');
  std::cout << board << "\n";

  char num = '0';
  for(size_t y = 0; y < board.size(1); ++y) {
    for(size_t x = 0; x < board.size(0); ++x) {
      board.at(x, y) = num++;
    }
  }
  std::cout << board << "\n";

  unsigned int piecesCountI = 0;
  unsigned int piecesCountL = 1;
  unsigned int piecesCountJ = 0;
  unsigned int piecesCountT = 2;
  unsigned int piecesCountZ = 0;
  unsigned int piecesCountS = 0;
  unsigned int piecesCountO = 0;
  std::vector<Piece> pieces;
  for(unsigned int i = 0; i < piecesCountI; ++i) {
    pieces.emplace_back(Piece::CreatePieceI());
  }
  for(unsigned int i = 0; i < piecesCountL; ++i) {
    pieces.emplace_back(Piece::CreatePieceL());
  }
  for(unsigned int i = 0; i < piecesCountJ; ++i) {
    pieces.emplace_back(Piece::CreatePieceJ());
  }
  for(unsigned int i = 0; i < piecesCountT; ++i) {
    pieces.emplace_back(Piece::CreatePieceT());
  }
  for(unsigned int i = 0; i < piecesCountZ; ++i) {
    pieces.emplace_back(Piece::CreatePieceZ());
  }
  for(unsigned int i = 0; i < piecesCountS; ++i) {
    pieces.emplace_back(Piece::CreatePieceS());
  }
  for(unsigned int i = 0; i < piecesCountO; ++i) {
    pieces.emplace_back(Piece::CreatePieceO());
  }


}

