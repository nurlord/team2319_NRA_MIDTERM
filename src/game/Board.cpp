#include "Minesweeper/Board.h"
#include <cstdlib>
#include <ctime>
#include <vector>

Board::Board(int w, int h, int mines)
    : width(w), height(h), mineCount(mines), cells(w * h), firstMove(true) {
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

  firstMove = true;

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

bool Board::reveal(int x, int y) {
  Cell &cell = get(x, y);
  if (cell.state != CellState::Hidden)
    return false;

  if (firstMove) {
    firstMove = false;
    if (cell.type == CellType::Mine) {
      relocateMine(x, y);
    }
  }

  if (cell.state != CellState::Hidden)
    return false;
  cell.state = CellState::Revealed;

  if (cell.type == CellType::Mine) {
    return true;
  }

  if (cell.neighborMines == 0) {
    for (int dy = -1; dy <= 1; ++dy)
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0)
          continue;
        int nx = x + dx, ny = y + dy;
        if (nx >= 0 && nx < width && ny >= 0 && ny < height)
          reveal(nx, ny);
      }
  }

  return false;
}

void Board::revealAllMines() {
  for (auto &cell : cells) {
    if (cell.type == CellType::Mine)
      cell.state = CellState::Revealed;
  }
}

void Board::relocateMine(int safeX, int safeY) {
  const int safeIndex = safeY * width + safeX;
  cells[safeIndex].type = CellType::Empty;
  cells[safeIndex].state = CellState::Hidden;

  std::vector<int> candidates;
  candidates.reserve(cells.size());
  for (int i = 0; i < static_cast<int>(cells.size()); ++i) {
    if (i == safeIndex)
      continue;
    if (cells[i].type != CellType::Mine)
      candidates.push_back(i);
  }

  if (!candidates.empty()) {
    int targetIndex = candidates[rand() % candidates.size()];
    cells[targetIndex].type = CellType::Mine;
    cells[targetIndex].state = CellState::Hidden;
  }

  calculateNumbers();
}
