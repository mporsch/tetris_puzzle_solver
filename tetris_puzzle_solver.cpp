#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <csignal>
#include <sstream>
#include <limits>
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

struct CommandLineArguments {
  size_t boardWidth;
  size_t boardHeight;
  unsigned int piecesCountI;
  unsigned int piecesCountL;
  unsigned int piecesCountJ;
  unsigned int piecesCountT;
  unsigned int piecesCountZ;
  unsigned int piecesCountS;
  unsigned int piecesCountO;

  CommandLineArguments(int argc, char **argv)
  try : piecesCountI(0U)
      , piecesCountL(0U)
      , piecesCountJ(0U)
      , piecesCountT(0U)
      , piecesCountZ(0U)
      , piecesCountS(0U)
      , piecesCountO(0U) {
    if((argc < 5) || (argc % 2 != 1)) {
      throw std::invalid_argument("Invalid number of arguments");
    } else {
      bool gotWidth = false;
      bool gotHeight = false;

      for(int argi = 1; argi < argc; argi += 2) {
        std::string const identifier = std::string(argv[argi]);
        std::string const value = std::string(argv[argi + 1]);

        if(identifier == "-w") {
          ParseValue(boardWidth, value, "board width");
          gotWidth = true;
        } else if(identifier == "-h") {
          ParseValue(boardHeight, value, "board height");
          gotHeight = true;
        } else if(identifier == "-I") {
          ParseValue(piecesCountI, value, "number of 'I' pieces");
        } else if(identifier == "-L") {
          ParseValue(piecesCountL, value, "number of 'L' pieces");
        } else if(identifier == "-J") {
          ParseValue(piecesCountJ, value, "number of 'J' pieces");
        } else if(identifier == "-T") {
          ParseValue(piecesCountT, value, "number of 'T' pieces");
        } else if(identifier == "-Z") {
          ParseValue(piecesCountZ, value, "number of 'Z' pieces");
        } else if(identifier == "-S") {
          ParseValue(piecesCountS, value, "number of 'S' pieces");
        } else if(identifier == "-O") {
          ParseValue(piecesCountO, value, "number of 'O' pieces");
        } else {
          throw std::invalid_argument("Unknown argument: '" + identifier + "'");
        }
      }

      if(!gotWidth || !gotHeight) {
        throw std::invalid_argument("Board width and height must be provided");
      }
    }
  }
  catch(std::invalid_argument const &e) {
    std::cout << e.what() << "\n\n";
    std::cout << "Usage:\n" << argv[0];
    std::cout << R"(
  -w <board width>
  -h <board height>
  -I <number of 'I' pieces> (optional)
  -L <number of 'L' pieces> (optional)
  -J <number of 'J' pieces> (optional)
  -T <number of 'T' pieces> (optional)
  -Z <number of 'Z' pieces> (optional)
  -S <number of 'S' pieces> (optional)
  -O <number of 'O' pieces> (optional)
)";
    exit(EXIT_FAILURE);
  }

private:
  template<typename T>
  void ParseValue(T &value, std::string const &arg, std::string const &argName) {
    long long num;
    if(!(std::istringstream(arg) >> num) ||
       (num < std::numeric_limits<T>::min()) ||
       (num > std::numeric_limits<T>::max())) {
      throw std::invalid_argument("Invalid " + argName + ": '" + arg + "'");
    }
    value = static_cast<T>(num);
  }
};

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

  // parse command line arguments
  CommandLineArguments const cmd(argc, argv);

  // create Board
  Board board(cmd.boardWidth, cmd.boardHeight);

#ifdef DEBUG_BOARD_COLOR
  // test Board print
  unsigned int id = 0;
  for(size_t y = 0; y < board.size(1); ++y) {
    for(size_t x = 0; x < board.size(0); ++x) {
      board.at(x, y) = Color::FromId(id++);
      std::cout << board << "\n";
    }
  }
  // restore board
  board = Board(cmd.boardWidth, cmd.boardHeight);
#endif

  // create Pieces
  std::vector<Piece> pieces;
  for(unsigned int i = 0; i < cmd.piecesCountI; ++i) {
    pieces.emplace_back(Piece::CreatePieceI(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < cmd.piecesCountL; ++i) {
    pieces.emplace_back(Piece::CreatePieceL(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < cmd.piecesCountJ; ++i) {
    pieces.emplace_back(Piece::CreatePieceJ(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < cmd.piecesCountT; ++i) {
    pieces.emplace_back(Piece::CreatePieceT(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < cmd.piecesCountZ; ++i) {
    pieces.emplace_back(Piece::CreatePieceZ(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < cmd.piecesCountS; ++i) {
    pieces.emplace_back(Piece::CreatePieceS(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < cmd.piecesCountO; ++i) {
    pieces.emplace_back(Piece::CreatePieceO(static_cast<unsigned int>(pieces.size())));
  }

  // run recursive solver
  if(!Solve(board, pieces)) {
    std::cout << "No exact solution found\n";
  }

  Cleanup(0);
  return EXIT_SUCCESS;
}

