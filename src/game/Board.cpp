#include "Minesweeper/Board.h"
#include <cstdlib>
#include <ctime>

Board::Board(int w, int h, int mines)
    : width(w), height(h), mineCount(mines), cells(w * h) {
  srand(time(nullptr));
  reset();
}

void Board::calculateNumbers() {
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      if (get(x, y).type == CellType::Mine)
        continue;

      int count = 0;
      for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
          int nx = x + dx;
          int ny = y + dy;
          if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            if (get(nx, ny).type == CellType::Mine)
              count++;
          }
        }
      }
      get(x, y).neighborMines = count; // use the correct member in Cell
    }
  }
}

bool Board::checkWin() {
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      if (get(x, y).type == CellType::Empty &&
          get(x, y).state != CellState::Revealed)
        return false;
    }
  }
  return true;
}

void Board::reset() {
  for (auto &c : cells) {
    c.state = CellState::Hidden;
    c.type = CellType::Empty;
    c.neighborMines = 0;
  }

  // place mines
  int placed = 0;
  while (placed < mineCount) {
    int idx = rand() % (width * height);
    if (cells[idx].type == CellType::Mine)
      continue;
    cells[idx].type = CellType::Mine;
    placed++;
  }

  // count neighbors
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x) {
      if (get(x, y).type == CellType::Mine)
        continue;
      int count = 0;
      for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
          if (!dx && !dy)
            continue;
          int nx = x + dx, ny = y + dy;
          if (nx >= 0 && nx < width && ny >= 0 && ny < height &&
              get(nx, ny).type == CellType::Mine)
            count++;
        }
      get(x, y).neighborMines = count;
    }
}

Cell &Board::get(int x, int y) { return cells[y * width + x]; }
const Cell &Board::get(int x, int y) const { return cells[y * width + x]; }

void Board::toggleFlag(int x, int y) {
  if (get(x, y).state == CellState::Hidden)
    get(x, y).state = CellState::Flagged;
  else if (get(x, y).state == CellState::Flagged)
    get(x, y).state = CellState::Hidden;
}

void Board::reveal(int x, int y) {
  auto &c = get(x, y);
  if (c.state != CellState::Hidden)
    return;
  c.state = CellState::Revealed;

  if (c.type == CellType::Mine) {
    // Game over
    return;
  }

  if (c.neighborMines == 0) {
    for (int dy = -1; dy <= 1; ++dy)
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0)
          continue;
        int nx = x + dx, ny = y + dy;
        if (nx >= 0 && nx < width && ny >= 0 && ny < height)
          reveal(nx, ny);
      }
  }
}
