#include "board.h"
#include "color.h"
#include "piece.h"
#include "solver.h"

#include <algorithm>
#include <csignal>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace {

void Cleanup(int) {
  std::cout << colorReset;
  exit(EXIT_SUCCESS);
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
  // restore empty board
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
