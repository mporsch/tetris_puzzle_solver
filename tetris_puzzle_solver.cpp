#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <algorithm>
#include <csignal>
#include <sstream>
#include <limits>
#include "../hypervector/hypervector.h"

//#define DEBUG_BOARD_COLOR
//#define DEBUG_SOLVER_STEPS
//#define DEBUG_SOLVABLE_CHECK

namespace {
  void ResetTerminalColor() {
    std::cout << "\x1B[0m";
  }

  void Cleanup(int) {
    ResetTerminalColor();
    exit(EXIT_SUCCESS);
  }
} // unnamed namespace

class Piece : public hypervector<bool, 2> {
public:
  struct Type {
    char shape;
    unsigned char rotationCount;

    bool operator==(Type const &other) const {
      return ((shape == other.shape) &&
              (rotationCount == other.rotationCount));
    }
  };

public:
  Piece()
    : hypervector<bool, 2>() {
  };

  static Piece CreatePieceI(unsigned int id) {
    return Piece(1, 4, id, 'i', 2);
  }

  static Piece CreatePieceL(unsigned int id) {
    Piece piece(2, 3, id, 'l');
    piece.at(1, 0) = false;
    piece.at(1, 1) = false;
    return piece;
  }

  static Piece CreatePieceJ(unsigned int id) {
    Piece piece(2, 3, id, 'j');
    piece.at(0, 0) = false;
    piece.at(0, 1) = false;
    return piece;
  }

  static Piece CreatePieceT(unsigned int id) {
    Piece piece(3, 2, id, 't');
    piece.at(0, 1) = false;
    piece.at(2, 1) = false;
    return piece;
  }

  static Piece CreatePieceZ(unsigned int id) {
    Piece piece(3, 2, id, 'z', 2);
    piece.at(0, 1) = false;
    piece.at(2, 0) = false;
    return piece;
  }

  static Piece CreatePieceS(unsigned int id) {
    Piece piece(3, 2, id, 's', 2);
    piece.at(0, 0) = false;
    piece.at(2, 1) = false;
    return piece;
  }

  static Piece CreatePieceO(unsigned int id) {
    return Piece(2, 2, id, 'o', 3);
  }

  bool IsBlockEmpty(size_t posX, size_t posY) const {
    return !this->at(posX, posY);
  }

  unsigned int GetId() const {
    return m_id;
  }

  Type GetType() const {
    return m_type;
  }

  size_t GetBlockCount() const {
    return std::accumulate(this->begin(), this->end(), 0,
      [](size_t sum, bool filled) {
        if(filled) {
          ++sum;
        }
        return sum;
      });
  }

  bool RotateRight() {
    if(m_type.rotationCount >= 3) {
      return false;
    } else {
      ++m_type.rotationCount;
    }

    // transpose
    Piece transposed(this->size(1), this->size(0), 0, this->GetType().shape);
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
  Piece(size_t sizeX, size_t sizeY, unsigned int id, char type, unsigned char rotationCount = 0)
    : hypervector<bool, 2>(sizeX, sizeY, true)
    , m_id(id)
    , m_type({type, rotationCount}) {
  };

private:
  unsigned int m_id;
  Type m_type;
};


class Color {
private:
  enum ColorCode {
    Black        =  40,
    Red          =  41,
    Green        =  42,
    Yellow       =  43,
    Blue         =  44,
    Magenta      =  45,
    Cyan         =  46,
    LightGray    =  47,
    DarkGray     = 100,
    LightRed     = 101,
    LightGreen   = 102,
    LightYellow  = 103,
    LightBlue    = 104,
    LightMagenta = 105,
    LightCyan    = 106
  };

public:
  Color()
    : m_colorCode(Black) {
  }

  static Color FromId(unsigned int id) {
    static std::vector<ColorCode> const colorLut = GetColorLut();

    Color ret;
    ret.m_colorCode = colorLut.at(id % colorLut.size());
    return ret;
  }

  Color(Color const &other) = default;
  Color &operator=(Color const &other) = default;

  bool operator==(Color const &other) const {
    return (m_colorCode == other.m_colorCode);
  }

  bool operator!=(Color const &other) const {
    return (m_colorCode != other.m_colorCode);
  }

  friend std::ostream &operator<<(std::ostream &os, Color const &color);

private:
  static std::vector<ColorCode> GetColorLut() {
    std::vector<ColorCode> ret;

    for(int i = Red; i <= LightGray; ++i) {
      ret.emplace_back(static_cast<ColorCode>(i));
    }
    for(int i = DarkGray; i <= LightCyan; ++i) {
      ret.emplace_back(static_cast<ColorCode>(i));
    }

    return ret;
  }

private:
  ColorCode m_colorCode;
};

std::ostream &operator<<(std::ostream &os, Color const &color) {
  auto const str = std::to_string(color.m_colorCode);
  os << "\e[" + str + "m";
  return os;
}


class Board : public hypervector<Color, 2> {
public:
  Board(size_t sizeX, size_t sizeY, Color color = Color())
    : hypervector<Color, 2>(sizeX, sizeY, std::forward<Color>(color)) {
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


class ConnectedComponentLabeler {
private:
  struct ConnectedComponent {
    unsigned int size;
  };

public:
  ConnectedComponentLabeler(Board const &board)
    : m_labelImage(board.size(0), board.size(1), 0)
    , m_ccs() {
    struct ConnectedComponentTemp {
      unsigned int parent;
      unsigned int size;

      ConnectedComponentTemp()
        : parent(0)
        , size(0) {
      }
    };

    // map of label -> connected component
    std::map<unsigned int, ConnectedComponentTemp> connectedComponents;

    // method to assign new labels
    unsigned int nextLabel = 1;
    auto newLabel = [&](unsigned int &current) {
      current = nextLabel++;
      ++connectedComponents[current].size;
    };

    // method to propagate labels
    auto propagateLabel = [&](unsigned int &current, unsigned int neighbor) {
      current = neighbor;
      ++connectedComponents[current].size;
    };

    // method to unify labels
    auto unifyLabel = [&](unsigned int &current, unsigned int left, unsigned int above) {
      current = left;
      ++connectedComponents[left].size;
      connectedComponents.at(above).parent = left;
    };

    // method to propagate, assign new or unify labels
    auto label = [&](unsigned int &current, unsigned int left, unsigned int above) {
      if((left != 0) && (above != 0) && (left != above)) {
        unifyLabel(current, left, above);
      } else if(left != 0) {
        propagateLabel(current, left);
      } else if(above != 0) {
        propagateLabel(current, above);
      } else {
        newLabel(current);
      }
    };

    // first line
    {
      unsigned int left = 0;
      for(size_t x = 0; x < board.size(0); ++x) {
        unsigned int &current = m_labelImage.at(x, 0);
        if(board.IsBlockEmpty(x, 0)) {
          label(current, left, 0);
        }
        left = current;
      }
    }

    // remaining board
    {
      for(size_t y = 1; y < board.size(1); ++y) {
        unsigned int left = 0;
        for(size_t x = 0; x < board.size(0); ++x) {
          unsigned int &current = m_labelImage.at(x, y);
          if(board.IsBlockEmpty(x, y)) {
            unsigned int above = m_labelImage.at(x, y - 1);
            label(current, left, above);
          }
          left = current;
        }
      }
    }

    // aggregate results
    for(auto &&p : connectedComponents) {
      auto &&cc = p.second;
      if(cc.parent != 0) {
        auto const parentLabel = connectedComponents.at(cc.parent).parent;
        if(parentLabel != 0) {
          cc.parent = parentLabel;
        }
      }
    }
    for(size_t y = 0; y < board.size(1); ++y) {
      for(size_t x = 0; x < board.size(0); ++x) {
        auto &label = m_labelImage.at(x, y);
        if(label != 0) {
          auto const parentLabel = connectedComponents.at(label).parent;
          if(parentLabel != 0) {
            label = parentLabel;
          }
        }
      }
    }
    for(auto &&p : connectedComponents) {
      auto &&cc = p.second;
      if(cc.parent == 0) {
        m_ccs[p.first].size += cc.size;
      } else {
        m_ccs[cc.parent].size += cc.size;
      }
    }
  }

  unsigned int GetMinimumSizeConnectedComponentSize() const {
    return m_ccs.at(GetMinimumSizeConnectedComponentLabel()).size;
  }

  Board GetMinimumSizeConnectedComponent() const {
    auto const label = GetMinimumSizeConnectedComponentLabel();

    Board board(m_labelImage.size(0), m_labelImage.size(1), Color::FromId(label));
    for(size_t y = 0; y < m_labelImage.size(1); ++y) {
      for(size_t x = 0; x < m_labelImage.size(0); ++x) {
        if(m_labelImage.at(x, y) == label) {
          board.at(x, y) = Color();
        }
      }
    }
    return board;
  }

private:
  unsigned int GetMinimumSizeConnectedComponentLabel() const {
    auto minLabel = 0;
    auto minSize = std::numeric_limits<unsigned int>::max();
    for(auto &&p : m_ccs) {
      auto &&cc = p.second;
      if(cc.size < minSize) {
        minSize = cc.size;
        minLabel = p.first;
      }
    }
    return minLabel;
  }

private:
  hypervector<unsigned int, 2> m_labelImage;
  std::map<unsigned int, ConnectedComponent> m_ccs; // map of label -> connected component
};


class Solver {
public:
  using BlackList = hypervector<std::vector<Piece::Type>, 2>;

public:
  Solver()
  : m_pieceMinimumBlockCount(-1) {
  }

  bool Solve(Board const &board, BlackList blackList, std::vector<Piece> pieces) const {
    if(pieces.empty()) {
      std::cout << board << "\n";
      return board.IsSolved();
    } else {
#ifdef DEBUG_SOLVER_STEPS
      std::cout << board << "\n";
#endif
    }

    ConnectedComponentLabeler ccl(board);

    bool isSolvable = (ccl.GetMinimumSizeConnectedComponentSize() >= m_pieceMinimumBlockCount);
    if(!isSolvable) {
#ifdef DEBUG_SOLVABLE_CHECK
      ResetTerminalColor();
      std::cout << "unsolvable:\n" << board << "\n";
#endif
      return false;
    }

    auto const subBoard = ccl.GetMinimumSizeConnectedComponent();

    for(size_t piecesOrder = 0; piecesOrder < pieces.size(); ++piecesOrder) {
      auto piece = pieces.at(0);
      do {
        for(size_t y = 0; y < subBoard.size(1); ++y) {
          for(size_t x = 0; x < subBoard.size(0); ++x) {
            bool isBlackListed = (std::end(blackList.at(x, y)) != std::find(
              std::begin(blackList.at(x, y)),
              std::end(blackList.at(x, y)),
              piece.GetType()));
            if(!isBlackListed &&
               subBoard.MayInsert(piece, x, y)) {
              // next iteration with updated board and pieces list
              if(Solve(board.Insert(piece, x, y),
                       blackList,
                       std::vector<Piece>(std::next(pieces.begin()), pieces.end()))) {
                return true;
              } else {
                blackList.at(x, y).push_back(piece.GetType());
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

  void SetPieceMinimumBlockCount(int pieceMinimumBlockCount) {
    m_pieceMinimumBlockCount = pieceMinimumBlockCount;
  }

private:
  int m_pieceMinimumBlockCount;
};


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
  try : piecesCountI(0)
      , piecesCountL(0)
      , piecesCountJ(0)
      , piecesCountT(0)
      , piecesCountZ(0)
      , piecesCountS(0)
      , piecesCountO(0) {
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

  size_t const piecesBlockCount = std::accumulate(begin(pieces), end(pieces), 0,
    [](size_t sum, Piece const &piece) -> size_t {
      return sum + piece.GetBlockCount();
    });
  if(piecesBlockCount > cmd.boardWidth * cmd.boardHeight) {
    std::cout << "Not solvable (too many pieces)\n";
  } else {
    Solver solver;

    // prepare Solver optimizations
    size_t minBlockCount = std::numeric_limits<size_t>::max();
    for(auto &&piece : pieces) {
      minBlockCount = std::min(minBlockCount, piece.GetBlockCount());
    }
    solver.SetPieceMinimumBlockCount(minBlockCount);

    // run recursive solver
    if(!solver.Solve(board, Solver::BlackList(board.size(0), board.size(1)), pieces)) {
      std::cout << "No exact solution found\n";
    }
  }

  ResetTerminalColor();
  return EXIT_SUCCESS;
}
