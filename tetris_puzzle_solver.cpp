#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <csignal>
#include "../hypervector/hypervector.h"

//#define DEBUG_BOARD_COLOR
//#define DEBUG_SOLVER_STEPS

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

  bool operator==(Color const &other) const {
    return (m_colorCode == other.m_colorCode);
  }

  bool operator!=(Color const &other) const {
    return (m_colorCode != other.m_colorCode);
  }

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

class Board : public hypervector<Color, 2> {
public:
  Board(size_t sizeX, size_t sizeY)
    : hypervector<Color, 2>(sizeX, sizeY) {
  }

  Board(Board const &other) = default;

  bool MayInsert(Piece const &piece, size_t posX, size_t posY) const {
    if(posX + piece.size(0) > this->size(0)) {
      return false;
    }
    if(posY + piece.size(1) > this->size(1)) {
      return false;
    }

    for(size_t y = 0; y < piece.size(1); ++y) {
      for(size_t x = 0; x < piece.size(0); ++x) {
        if(!piece.IsBlockEmpty(x, y) && !IsBlockEmpty(posX + x, posY + y)) {
          return false;
        }
      }
    }

    return true;
  }

  Board Insert(Piece const &piece, size_t posX, size_t posY) const {
    Board ret(*this);

    for(size_t y = 0; y < piece.size(1); ++y) {
      for(size_t x = 0; x < piece.size(0); ++x) {
        if(!piece.IsBlockEmpty(x, y)) {
          ret.at(posX + x, posY + y) = Color::FromId(piece.GetId());
        }
      }
    }

    return ret;
  }

  bool IsSolved() const {
    return std::all_of(this->begin(), this->end(),
      [](Color const &color) -> bool {
        return (color != Color());
      });
  }

private:
  bool IsBlockEmpty(size_t posX, size_t posY) const {
    return (this->at(posX, posY) == Color());
  }
};

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

bool Solve(Board const &board, std::vector<Piece> pieces) {
  if(pieces.empty()) {
    std::cout << board << "\n";
    return board.IsSolved();
  } else {
#ifdef DEBUG_SOLVER_STEPS
    std::cout << board << "\n";
#endif
  }

  for(size_t piecesOrder = 0; piecesOrder < pieces.size(); ++piecesOrder) {
    auto piece = pieces.at(0);
    do {
      for(size_t y = 0; y < board.size(1); ++y) {
        for(size_t x = 0; x < board.size(0); ++x) {
          if(board.MayInsert(piece, x, y)) {
            // next iteration with updated board and pieces list
            if(Solve(board.Insert(piece, x, y),
                     std::vector<Piece>(std::next(pieces.begin()), pieces.end()))) {
              return true;
            }
          }
        }
      }
    } while(piece.RotateRight()); // try again with this piece rotated

    // try another order of pieces
    std::rotate(pieces.begin(), std::next(pieces.begin()), pieces.end());
  }

  return false;
}

void ResetTerminalColor() {
  std::cout << "\x1B[0m";
}

void Cleanup(int) {
  ResetTerminalColor();
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  // set up Ctrl-C handler
  if(std::signal(SIGINT, Cleanup)) {
    std::cerr << "Failed to register signal handler\n";
    return EXIT_FAILURE;
  }

  // create Board
  size_t boardWidth = 4;
  size_t boardHeight = 5;
  Board board(boardWidth, boardHeight);

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
  unsigned int piecesCountZ = 1;
  unsigned int piecesCountS = 1;
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

  if(!Solve(board, pieces)) {
    std::cout << "No exact solution found\n";
  }

  Cleanup(0);
  return EXIT_SUCCESS;
}

