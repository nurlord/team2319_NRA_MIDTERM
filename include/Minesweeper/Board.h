#pragma once
#include <vector>
#include "Cell.h"

class Board {
public:
  int width, height, mineCount;
  std::vector<Cell> cells;

  Board(int w, int h, int mines);

  void reset();
  bool reveal(int x, int y);
  void toggleFlag(int x, int y);
  Cell &get(int x, int y);
  const Cell &get(int x, int y) const;

  void calculateNumbers(); // add this declaration
  bool checkWin();         // add this declaration
  void revealAllMines();
};
