enum class CellType { Empty, Mine };
enum class CellState { Hidden, Revealed, Flagged };

struct Cell {
  CellType type = CellType::Empty;
  CellState state = CellState::Hidden;
  int neighborMines = 0; // <-- add this
};
