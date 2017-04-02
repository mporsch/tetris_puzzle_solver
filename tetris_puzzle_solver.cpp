#include <vector>
#include <iostream>
#include <string>
#include "../hypervector/hypervector.h"

//#define DEBUG_BOARD_COLOR

class Piece : public hypervector<bool, 2> {
public:
  Piece()
    : hypervector<bool, 2>() {
  };

  static Piece CreatePieceI(unsigned int id) {
    return Piece(1, 4, id, 2U);
  }

  static Piece CreatePieceL(unsigned int id) {
    Piece piece(2, 3, id);
    piece.at(1, 0) = false;
    piece.at(1, 1) = false;
    return piece;
  }

  static Piece CreatePieceJ(unsigned int id) {
    Piece piece(2, 3, id);
    piece.at(0, 0) = false;
    piece.at(0, 1) = false;
    return piece;
  }

  static Piece CreatePieceT(unsigned int id) {
    Piece piece(3, 2, id);
    piece.at(0, 1) = false;
    piece.at(2, 1) = false;
    return piece;
  }

  static Piece CreatePieceZ(unsigned int id) {
    Piece piece(3, 2, id, 2U);
    piece.at(0, 1) = false;
    piece.at(2, 0) = false;
    return piece;
  }

  static Piece CreatePieceS(unsigned int id) {
    Piece piece(3, 2, id, 2U);
    piece.at(0, 0) = false;
    piece.at(2, 1) = false;
    return piece;
  }

  static Piece CreatePieceO(unsigned int id) {
    return Piece(2, 2, id, 3U);
  }

  bool IsBlockEmpty(size_t posX, size_t posY) const {
    return !this->at(posX, posY);
  }

  unsigned int GetId() const {
    return m_id;
  }

  bool RotateRight() {
    if(m_rotationCount >= 3U) {
      return false;
    } else {
      ++m_rotationCount;
    }

    // transpose
    Piece transposed(this->size(1), this->size(0), 0U);
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
  Piece(size_t sizeX, size_t sizeY, unsigned int id, unsigned int rotationCount = 0U)
    : hypervector<bool, 2>(sizeX, sizeY, true)
    , m_id(id)
    , m_rotationCount(rotationCount) {
  };

private:
  unsigned int m_id;
  unsigned int m_rotationCount;
};

class Color {
public:
  Color()
    : m_colorCode(40U) { // black as default
  }

  static Color FromId(unsigned int id) {
    static std::vector<unsigned int> colorLut = GetColorLut();

    Color ret;
    ret.m_colorCode = colorLut.at(id % colorLut.size());
    return ret;
  }

  Color(Color const &other) = default;

  friend std::ostream &operator<<(std::ostream &os, Color const &color);

private:
  static std::vector<unsigned int> GetColorLut() {
    std::vector<unsigned int> ret;

    for(unsigned int i = 41U; i < 48U; ++i) {
      ret.emplace_back(i);
    }
    for(unsigned int i = 100U; i < 107U; ++i) {
      ret.emplace_back(i);
    }

    return ret;
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
      std::cout << ' ' << ' ';
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

#ifdef DEBUG_BOARD_COLOR
  // test Board print
  unsigned int id = 0;
  for(size_t y = 0; y < board.size(1); ++y) {
    for(size_t x = 0; x < board.size(0); ++x) {
      board.at(x, y) = Color::FromId(id++);
      std::cout << board << "\n";
    }
  }
  board = Board(boardWidth, boardHeight);
#endif

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
    pieces.emplace_back(Piece::CreatePieceI(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < piecesCountL; ++i) {
    pieces.emplace_back(Piece::CreatePieceL(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < piecesCountJ; ++i) {
    pieces.emplace_back(Piece::CreatePieceJ(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < piecesCountT; ++i) {
    pieces.emplace_back(Piece::CreatePieceT(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < piecesCountZ; ++i) {
    pieces.emplace_back(Piece::CreatePieceZ(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < piecesCountS; ++i) {
    pieces.emplace_back(Piece::CreatePieceS(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < piecesCountO; ++i) {
    pieces.emplace_back(Piece::CreatePieceO(static_cast<unsigned int>(pieces.size())));
  }

  // cleanup
  ResetTerminalColor();
}

