#include "hypervector.h"

#include <algorithm>
#include <cassert>
#include <csignal>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

//#define DEBUG_BOARD_COLOR
//#define DEBUG_SOLVER_STEPS
//#define DEBUG_CCL_STEPS
//#define DEBUG_CCL_AGGREGATION
//#define DEBUG_SOLVABLE_CHECK

namespace {

static std::string const colorReset = "\x1B[0m";

void Cleanup(int) {
  std::cout << colorReset;
  exit(EXIT_SUCCESS);
}

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
    hypervector<unsigned char, 2> transposed(this->size(1), this->size(0));
    for(size_t x = 0; x < this->size(0); ++x) {
      for(size_t y = 0; y < this->size(1); ++y) {
        transposed.at(y, x) = this->at(x, y);
      }
    }

    this->resize(this->size(1), this->size(0));

    // mirror vertically
    auto const offsetX = this->size(0) - 1;
    for(size_t x = 0; x < this->size(0); ++x) {
      for(size_t y = 0; y < this->size(1); ++y) {
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


class Color {
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
    ret.m_colorCode = colorLut[id % colorLut.size()];
    return ret;
  }

  operator bool() const {
    return (m_colorCode != Black);
  }

  friend std::ostream &operator<<(std::ostream &os, Color const &color);

private:
  static std::vector<ColorCode> GetColorLut() {
    std::vector<ColorCode> ret;

    for(int i = Red; i <= LightGray; ++i) {
      ret.push_back(static_cast<ColorCode>(i));
    }
    for(int i = DarkGray; i <= LightCyan; ++i) {
      ret.push_back(static_cast<ColorCode>(i));
    }

    return ret;
  }

private:
  ColorCode m_colorCode;
};

std::ostream &operator<<(std::ostream &os, Color const &color) {
  os << "\x1B[" << color.m_colorCode << "m" << ' ' << ' ';
  return os;
}


struct Board : public hypervector<Color, 2> {
  Board(size_t sizeX, size_t sizeY, Color color = Color())
    : hypervector<Color, 2>(sizeX, sizeY, color) {
  }

  bool MayInsert(Piece const &piece, size_t posX, size_t posY) const {
    // check if piece exceeds the board
    if((posX + piece.size(0) > this->size(0)) ||
       (posY + piece.size(1) > this->size(1))) {
      return false;
    }

    for(size_t y = 0; y < piece.size(1); ++y) {
      for(size_t x = 0; x < piece.size(0); ++x) {
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
    for(size_t y = 0; y < piece.size(1); ++y) {
      for(size_t x = 0; x < piece.size(0); ++x) {
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
    for(size_t x = 0; x < board.size(0); ++x) {
      std::cout << board.at(x, y);
    }
    if(y < board.size(1) - 1) {
      std::cout << colorReset << "\n";
    } else {
      break;
    }
  }
  os << colorReset;
  return os;
}


// labeler for the empty blocks of the board that are yet to be filled
struct ConnectedComponentLabeler {
  struct SubBoard {
    Board board;
    size_t offsetX;
    size_t offsetY;
  };

private:
  struct RegionOfInterest {
    size_t left;
    size_t right;
    size_t top;
    size_t bottom;

    void Add(size_t x, size_t y) {
      left = std::min(left, x);
      right = std::max(right, x);
      top = std::min(top, y);
      bottom = std::max(bottom, y);
    }

    void Add(RegionOfInterest const &other) {
      left = std::min(left, other.left);
      right = std::max(right, other.right);
      top = std::min(top, other.top);
      bottom = std::max(bottom, other.bottom);
    }
  };

  using Label = unsigned int;
  enum {
    Unlabeled = 0
  };

  struct ConnectedComponent {
    RegionOfInterest roi;
    unsigned int size;
  };

  friend std::ostream &operator<<(
      std::ostream &os,
      hypervector<Label, 2> const &labelImage);

public:
  ConnectedComponentLabeler(Board const &board)
    : m_labelImage(board.size(0), board.size(1), Unlabeled)
    , m_ccs() {
    struct ConnectedComponentTemp : ConnectedComponent {
      Label parent;

      ConnectedComponentTemp(ConnectedComponent const &cc)
        : ConnectedComponent(cc)
        , parent(Unlabeled) {
      }
    };

    std::map<Label, ConnectedComponentTemp> connectedComponents;

    // method to assign new labels
    Label nextLabel = 1;
    auto newLabel = [&](Label &current, size_t x, size_t y) {
      current = nextLabel++;
      auto created = connectedComponents.insert(std::make_pair(current,
        ConnectedComponent{
          RegionOfInterest{x, x, y, y},
          1
        })).second;
      assert(created);
    };

    // method to propagate labels
    auto propagateLabel = [&](Label &current, Label neighbor, size_t x, size_t y) {
      current = neighbor;
      auto &&cc = connectedComponents.at(current);
      cc.roi.Add(x, y);
      ++cc.size;
    };

    // method to unify labels
    auto unifyLabel = [&](Label &current, Label left, Label above, size_t x, size_t y) {
      current = left;
      auto &&cc = connectedComponents.at(left);
      cc.roi.Add(x, y);
      ++cc.size;
      connectedComponents.at(above).parent = left;
    };

    // method to propagate, assign new or unify labels
    auto label = [&](Label &current, Label left, Label above, size_t x, size_t y) {
      if((left != Unlabeled) && (above != Unlabeled) && (left != above)) {
        unifyLabel(current, left, above, x, y);
      } else if(left != Unlabeled) {
        propagateLabel(current, left, x, y);
      } else if(above != Unlabeled) {
        propagateLabel(current, above, x, y);
      } else {
        newLabel(current, x, y);
      }

      #ifdef DEBUG_CCL_STEPS
          std::cout << m_labelImage << " CCL:";
          for(auto &&p : connectedComponents) {
            std::cout << " ["
              << Color::FromId(p.first) << colorReset
              << ": parent " << Color::FromId(p.second.parent) << colorReset
              << ", width " << p.second.roi.right - p.second.roi.left + 1
              << ", height " << p.second.roi.bottom - p.second.roi.top + 1
              << ", size " << p.second.size << "]";
          }
          std::cout << std::endl;
      #endif
    };

    // first line
    {
      Label left = Unlabeled;
      for(size_t x = 0; x < board.size(0); ++x) {
        Label &current = m_labelImage.at(x, 0);
        if(board.IsBlockEmpty(x, 0)) {
          label(current, left, Unlabeled, x, 0);
        }
        left = current;
      }
    }

    // remaining board
    {
      for(size_t y = 1; y < board.size(1); ++y) {
        Label left = Unlabeled;
        for(size_t x = 0; x < board.size(0); ++x) {
          Label &current = m_labelImage.at(x, y);
          if(board.IsBlockEmpty(x, y)) {
            Label above = m_labelImage.at(x, y - 1);
            label(current, left, above, x, y);
          }
          left = current;
        }
      }
    }

#ifdef DEBUG_CCL_AGGREGATION
    std::cout << m_labelImage << " CCL: non-aggregated" << std::endl;
#endif

    // aggregate results
    for(auto &&p : connectedComponents) {
      auto &&cc = p.second;
      if(cc.parent != Unlabeled) {
        auto const parentLabel = connectedComponents.at(cc.parent).parent;
        if(parentLabel != Unlabeled) {
          cc.parent = parentLabel;
        }
      }
    }
    for(size_t y = 0; y < board.size(1); ++y) {
      for(size_t x = 0; x < board.size(0); ++x) {
        auto &label = m_labelImage.at(x, y);
        if(label != Unlabeled) {
          auto const parentLabel = connectedComponents.at(label).parent;
          if(parentLabel != Unlabeled) {
            label = parentLabel;
          }
        }
      }
    }
    for(auto &&p : connectedComponents) {
      auto &&from = p.second;
      if(from.parent == Unlabeled) {
        auto &&to = m_ccs[p.first];
        to.roi.Add(from.roi);
        to.size += from.size;
      } else {
        auto &&to = m_ccs[from.parent];
        to.roi.Add(from.roi);
        to.size += from.size;
      }
    }

    m_minLabel = GetMinLabel();

#ifdef DEBUG_CCL_AGGREGATION
    std::cout << m_labelImage << " CCL: aggregated, minimum-size connected component: "
    << Color::FromId(m_minLabel) << colorReset
    << " size=" << GetMinSize() << std::endl;
#endif
  }

  unsigned int GetMinSize() const {
    return m_ccs.at(m_minLabel).size;
  }

  // get board where the minimum-size component's blocks are uncolored
  SubBoard GetMin() const {
    auto &&cc = m_ccs.at(m_minLabel);
    SubBoard sub{
      Board(cc.roi.right - cc.roi.left + 1,
            cc.roi.bottom - cc.roi.top + 1,
            Color::FromId(m_minLabel)),
      cc.roi.left,
      cc.roi.top
    };

    for(size_t y = 0; y < sub.board.size(1); ++y) {
      for(size_t x = 0; x < sub.board.size(0); ++x) {
        if(m_labelImage.at(x + cc.roi.left, y + cc.roi.top) == m_minLabel) {
          sub.board.at(x, y) = Color();
        }
      }
    }

    return sub;
  }

private:
  Label GetMinLabel() const {
    Label minLabel = Unlabeled;
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
  hypervector<Label, 2> m_labelImage;
  std::map<Label, ConnectedComponent> m_ccs;
  Label m_minLabel;
};

std::ostream &operator<<(
    std::ostream &os,
    hypervector<ConnectedComponentLabeler::Label, 2> const &labelImage) {
  for(size_t y = 0;; ++y) {
    for(size_t x = 0; x < labelImage.size(0); ++x) {
      auto const label = labelImage.at(x, y);
      auto const color = (label == ConnectedComponentLabeler::Unlabeled ? Color() : Color::FromId(label));
      os << color;
    }
    if(y < labelImage.size(1) - 1) {
      os << colorReset << "\n";
    } else {
      break;
    }
  }
  os << colorReset;
  return os;
}


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

} // unnamed namespace

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
      auto const color = Color::FromId(id);
      board.at(x, y) = color;
      std::cout << board << " Board: color id " << id++
        << ", color " << color << colorReset << std::endl;
    }
  }
  // restore board
  board = Board(cmd.boardWidth, cmd.boardHeight);
#endif

  // create Pieces
  std::vector<Piece> pieces;
  for(unsigned int i = 0; i < cmd.piecesCountI; ++i) {
    pieces.push_back(Piece::CreateI(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < cmd.piecesCountL; ++i) {
    pieces.push_back(Piece::CreateL(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < cmd.piecesCountJ; ++i) {
    pieces.push_back(Piece::CreateJ(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < cmd.piecesCountT; ++i) {
    pieces.push_back(Piece::CreateT(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < cmd.piecesCountZ; ++i) {
    pieces.push_back(Piece::CreateZ(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < cmd.piecesCountS; ++i) {
    pieces.push_back(Piece::CreateS(static_cast<unsigned int>(pieces.size())));
  }
  for(unsigned int i = 0; i < cmd.piecesCountO; ++i) {
    pieces.push_back(Piece::CreateO(static_cast<unsigned int>(pieces.size())));
  }

  auto const expectedPiecesBlockCount = cmd.boardWidth * cmd.boardHeight;
  auto const receivedPiecesBlockCount = std::accumulate(
    begin(pieces), end(pieces), static_cast<unsigned int>(0),
    [](unsigned int sum, Piece const &piece) -> unsigned int {
      return sum + piece.GetBlockCount();
    });
  if(receivedPiecesBlockCount > expectedPiecesBlockCount) {
    std::cout << "Not solvable (too many pieces)\n";
  } else {
    if(receivedPiecesBlockCount < expectedPiecesBlockCount) {
      std::cout << "Multiple solutions possible (too few pieces)\n";
    }

    Solver solver;

    // prepare Solver optimizations
    auto minBlockCount = std::numeric_limits<unsigned int>::max();
    for(auto &&piece : pieces) {
      minBlockCount = std::min(minBlockCount, piece.GetBlockCount());
    }
    solver.pieceMinimumBlockCount = minBlockCount;

    // run recursive solver
    if(!solver.Solve(std::move(board),
                     Solver::BlackList(board.size(0), board.size(1)),
                     begin(pieces),
                     end(pieces))) {
      std::cout << "No exact solution found\n";
    }
  }

  return EXIT_SUCCESS;
}
