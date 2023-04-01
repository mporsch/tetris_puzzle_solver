#pragma once

#include <iostream>
#include <vector>

static char const colorReset[] = "\x1B[0m";

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
