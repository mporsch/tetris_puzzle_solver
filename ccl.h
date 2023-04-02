#pragma once

#include "board.h"

#include "hypervector.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <unordered_map>

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

    std::unordered_map<Label, ConnectedComponentTemp> connectedComponents;

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
  std::unordered_map<Label, ConnectedComponent> m_ccs;
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
