#include <vector>
#include <iostream>
#include <string>
#include "../hypervector/hypervector.h"

class Piece : hypervector<bool, 2> {
public:
  Piece()
    : hypervector<bool, 2>() {
  };

  static Piece CreatePieceI() {
    return Piece(1, 4, 2U);
  }

  static Piece CreatePieceL() {
    Piece piece(2, 3);
    piece.at(1, 0) = false;
    piece.at(1, 1) = false;
    return piece;
  }

  static Piece CreatePieceJ() {
    Piece piece(2, 3);
    piece.at(0, 0) = false;
    piece.at(0, 1) = false;
    return piece;
  }

  static Piece CreatePieceT() {
    Piece piece(3, 2);
    piece.at(0, 1) = false;
    piece.at(2, 1) = false;
    return piece;
  }

  static Piece CreatePieceZ() {
    Piece piece(3, 2, 2U);
    piece.at(0, 1) = false;
    piece.at(2, 0) = false;
    return piece;
  }

  static Piece CreatePieceS() {
    Piece piece(3, 2, 2U);
    piece.at(0, 0) = false;
    piece.at(2, 1) = false;
    return piece;
  }

  static Piece CreatePieceO() {
    return Piece(2, 2, 3U);
  }

  bool RotateRight() {
    if(m_rotationCount >= 3U) {
      return false;
    }

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

    return true;
  }

private:
  Piece(size_type countX, size_type countY, unsigned int rotationCount = 0U)
    : hypervector<bool, 2>(countX, countY, true)
    , m_rotationCount(rotationCount) {
  };

private:
  unsigned int m_rotationCount;
};

class Color {
public:
  Color()
    : m_colorCode(41U) {
  }

  Color(Color const &other) = default;

  Color &operator++() {
    Increment();
    return *this;
  }

  Color operator++(int) {
    Color const tmp(*this);
    Increment();
    return tmp;
  }

  friend std::ostream &operator<<(std::ostream &os, Color const &color);

private:
  void Increment() {
    ++m_colorCode;
    Limit();
  }

  void Limit() {
    if(m_colorCode > 107U) {
      m_colorCode = 41U;
    } else if((m_colorCode > 47U) && (m_colorCode < 100U)) {
      m_colorCode = 100U;
    }
  }

private:
  unsigned int m_colorCode;
};

std::ostream &operator<<(std::ostream &os, Color const &color) {
  auto const str = std::to_string(color.m_colorCode);
  os << "\e[" + str + "m";
  return os;
}

using Board = hypervector<Color, 2>;

std::ostream &operator<<(std::ostream &os, Board const &board) {
  for(size_t y = 0; y < board.size(1); ++y) {
    for(size_t x = 0; x < board.size(0); ++x) {
      std::cout << board.at(x, y);
      std::cout << ' ';
    }
    std::cout << "\n";
  }
  return os;
}

void ResetTerminalColor() {
  std::cout << "\x1B[0m";
}

int main(int argc, char **argv) {
  // create Board
  size_t dimX = 4;
  size_t dimY = 3;
  Board board(dimX, dimY);

  // test Board print
  Color color;
  for(size_t y = 0; y < board.size(1); ++y) {
    for(size_t x = 0; x < board.size(0); ++x) {
      board.at(x, y) = color++;
    }
  }
  std::cout << board << "\n";

  // create Pieces
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

  // cleanup
  ResetTerminalColor();
}

