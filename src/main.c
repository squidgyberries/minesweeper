#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <raylib.h>
#include <raymath.h>

const Color number_colors[] = {LIGHTGRAY, BLUE, GREEN, RED, DARKBLUE, MAROON, SKYBLUE, BLACK, GRAY};

const int board_width = 9;
const int board_height = 9;
int num_mines = 10;
int mines_left = 10;

const uint8_t number_mask = 0x0F; // 0b00001111

const uint8_t cell_display_state_closed = 0;
const uint8_t cell_display_state_open = 1;
const uint8_t cell_display_state_flagged = 2;
const uint8_t cell_display_state_mine = 3;
const uint8_t cell_display_state_mistake = 4;
const uint8_t cell_display_state_flag_mistake = 5;
const uint8_t cell_display_state_press = 6;
static inline uint8_t getDisplayState(uint8_t cell) { return cell >> 4; }

static inline uint8_t setDisplayState(uint8_t cell, uint8_t state) { return (cell & number_mask) | (state << 4); }

static inline uint8_t getNumber(uint8_t cell) { return cell & number_mask; }

// [0, n)
int randint(int n) {
  const int limit = RAND_MAX - (RAND_MAX % n);

  int r;
  while ((r = rand()) >= limit)
    ;

  return r % n;
}

bool findCollisionCell(Vector2 mouse_pos, Vector2 top_left, int *out_x, int *out_y) {
  for (int x = 0; x < board_width; ++x) {
    for (int y = 0; y < board_height; ++y) {
      Rectangle cell_rect = {top_left.x + x * 20.0f + 2.0f, top_left.y + y * 20.0f + 2.0f, 16.0f, 16.0f};
      if (CheckCollisionPointRec(mouse_pos, cell_rect)) {
        *out_x = x;
        *out_y = y;
        return true;
      }
    }
  }
  *out_x = -1;
  *out_y = -1;
  return false;
}

static inline void openCellIfClosed(uint8_t board[board_height][board_width], int x, int y);

void openCell(uint8_t board[board_height][board_width], int x, int y) {
  if (getDisplayState(board[y][x]) == cell_display_state_flagged) {
    return;
  }
  if (getNumber(board[y][x]) != 9) {
    board[y][x] = setDisplayState(board[y][x], cell_display_state_open);
  } else {
    board[y][x] = setDisplayState(board[y][x], cell_display_state_mistake);
  }
  const bool left_edge = x == 0;
  const bool right_edge = x == (board_width - 1);
  const bool top_edge = y == 0;
  const bool bottom_edge = y == (board_height - 1);
  if (getNumber(board[y][x]) == 0) {
    if (!left_edge) {
      openCellIfClosed(board, x - 1, y);
      if (!top_edge) {
        openCellIfClosed(board, x - 1, y - 1);
      }
      if (!bottom_edge) {
        openCellIfClosed(board, x - 1, y + 1);
      }
    }
    if (!right_edge) {
      openCellIfClosed(board, x + 1, y);
      if (!top_edge) {
        openCellIfClosed(board, x + 1, y - 1);
      }
      if (!bottom_edge) {
        openCellIfClosed(board, x + 1, y + 1);
      }
    }
    if (!top_edge) {
      openCellIfClosed(board, x, y - 1);
    }
    if (!bottom_edge) {
      openCellIfClosed(board, x, y + 1);
    }
  }
}

static inline void openCellIfClosed(uint8_t board[board_height][board_width], int x, int y) {
  if (getDisplayState(board[y][x]) == cell_display_state_closed) {
    openCell(board, x, y);
  }
}

void toggleFlagged(uint8_t board[board_height][board_width], int x, int y) {
  if (getDisplayState(board[y][x]) != cell_display_state_flagged) {
    board[y][x] = setDisplayState(board[y][x], cell_display_state_flagged);
    --mines_left;
  } else {
    board[y][x] = setDisplayState(board[y][x], cell_display_state_closed);
    ++mines_left;
  }
}

void generateMines(uint8_t board[board_height][board_width], int start_x, int start_y) {
  board[start_y][start_x] = setDisplayState(10, cell_display_state_closed);

  // if (num_mines > board_width * board_height - 1) {
  //   num_mines = board_width * board_height - 1;
  // }

  int available_start_neighbors = (board_width * board_height - 1) - num_mines;
  const bool start_left_edge = start_x == 0;
  const bool start_right_edge = start_x == (board_width - 1);
  const bool start_top_edge = start_y == 0;
  const bool start_bottom_edge = start_y == (board_height - 1);
  int start_neighbors = 0;
  // Go row by row
  if (!start_top_edge) {
    if (!start_left_edge) {
      if (available_start_neighbors-- > 0) {
        board[start_y - 1][start_x - 1] = setDisplayState(10, getDisplayState(board[start_y - 1][start_x - 1]));
        ++start_neighbors;
      }
    }
    if (available_start_neighbors-- > 0) {
      board[start_y - 1][start_x] = setDisplayState(10, getDisplayState(board[start_y - 1][start_x]));
      ++start_neighbors;
    }
    if (!start_right_edge) {
      if (available_start_neighbors-- > 0) {
        board[start_y - 1][start_x + 1] = setDisplayState(10, getDisplayState(board[start_y - 1][start_x + 1]));
        ++start_neighbors;
      }
    }
  }
  if (!start_left_edge) {
    if (available_start_neighbors-- > 0) {
      board[start_y][start_x - 1] = setDisplayState(10, getDisplayState(board[start_y][start_x - 1]));
      ++start_neighbors;
    }
  }
  if (!start_right_edge) {
    if (available_start_neighbors-- > 0) {
      board[start_y][start_x + 1] = setDisplayState(10, getDisplayState(board[start_y][start_x + 1]));
      ++start_neighbors;
    }
  }
  if (!start_bottom_edge) {
    if (!start_left_edge) {
      if (available_start_neighbors-- > 0) {
        board[start_y + 1][start_x - 1] = setDisplayState(10, getDisplayState(board[start_y + 1][start_x - 1]));
        ++start_neighbors;
      }
    }
    if (available_start_neighbors-- > 0) {
      board[start_y + 1][start_x] = setDisplayState(10, getDisplayState(board[start_y + 1][start_x]));
      ++start_neighbors;
    }
    if (!start_right_edge) {
      if (available_start_neighbors-- > 0) {
        board[start_y + 1][start_x + 1] = setDisplayState(10, getDisplayState(board[start_y + 1][start_x + 1]));
        ++start_neighbors;
      }
    }
  }

  int available_cells = board_width * board_height - (1 + start_neighbors);
  for (int i = 0; i < num_mines; ++i) {
    int num = randint(available_cells);
    for (int y = 0; y < board_height; ++y) {
      for (int x = 0; x < board_width; ++x) {
        if (getNumber(board[y][x]) == 0) {
          if (num-- == 0) {
            board[y][x] = setDisplayState(9, getDisplayState(board[y][x]));
          }
        }
      }
    }
    --available_cells;
  }
  for (int y = 0; y < board_height; ++y) {
    for (int x = 0; x < board_width; ++x) {
      if (getNumber(board[y][x]) == 9) {
        continue;
      }
      const bool left_edge = x == 0;
      const bool right_edge = x == (board_width - 1);
      const bool top_edge = y == 0;
      const bool bottom_edge = y == (board_height - 1);
      // clang-format off
      const uint8_t neighbors = (!left_edge && !top_edge && getNumber(board[y - 1][x - 1]) == 9) +
                                (!left_edge && getNumber(board[y][x - 1]) == 9) +
                                (!left_edge && !bottom_edge && getNumber(board[y + 1][x - 1]) == 9) +

                                (!right_edge && !top_edge && getNumber(board[y - 1][x + 1]) == 9) +
                                (!right_edge && getNumber(board[y][x + 1]) == 9) +
                                (!right_edge && !bottom_edge && getNumber(board[y + 1][x + 1]) == 9) +

                                (!top_edge && getNumber(board[y - 1][x]) == 9) +
                                (!bottom_edge && getNumber(board[y + 1][x]) == 9);
      // clang-format on
      board[y][x] = setDisplayState(neighbors, getDisplayState(board[y][x]));
    }
  }
}

int main(void) {
  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(40 + board_width * 20, 100 + board_height * 20, "minesweeper");

  srand(time(NULL));

  // Generate board
  // clang-format off
  // uint8_t (*board)[board_width] = calloc(1, sizeof(uint8_t[board_height][board_width]));
  // uint8_t (*board)[board_width] = calloc(1, sizeof(uint8_t * board_width * board_height));
  // uint8_t (*board)[board_width] = calloc(board_height, sizeof(*board));
  // clang-format on
  uint8_t(*board)[board_width] = calloc(board_width * board_height, sizeof(uint8_t));
  if (num_mines > board_width * board_height - 1) {
    num_mines = board_width * board_height - 1;
    mines_left = num_mines;
  }

  bool is_first_open = true;
  int last_press_x = 0;
  int last_press_y = 0;
  while (!WindowShouldClose()) {
    Vector2 top_left = (Vector2){20.0f, 80.0f};

    Vector2 mouse_pos = GetMousePosition();
    int mouse_cell_x, mouse_cell_y;
    const bool mouse_is_on_cell = findCollisionCell(mouse_pos, top_left, &mouse_cell_x, &mouse_cell_y);
    const uint8_t mouse_display_state = getDisplayState(board[mouse_cell_y][mouse_cell_x]);
    if (getDisplayState(board[last_press_y][last_press_x]) == cell_display_state_press) {
      board[last_press_y][last_press_x] = setDisplayState(board[last_press_y][last_press_x], cell_display_state_closed);
    }
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
      if (mouse_is_on_cell && mouse_display_state == cell_display_state_closed) {
        board[mouse_cell_y][mouse_cell_x] = setDisplayState(board[mouse_cell_y][mouse_cell_x], cell_display_state_press);
        last_press_x = mouse_cell_x;
        last_press_y = mouse_cell_y;
      }
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
      if (mouse_is_on_cell) {
        if (is_first_open) {
          generateMines(board, mouse_cell_x, mouse_cell_y);
          is_first_open = false;
        }
        openCell(board, mouse_cell_x, mouse_cell_y);
      }
    }
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
      if (mouse_is_on_cell) {
        if (mouse_display_state == cell_display_state_closed || mouse_display_state == cell_display_state_flagged) {
          toggleFlagged(board, mouse_cell_x, mouse_cell_y);
        }
      }
    }

    BeginDrawing();

    ClearBackground(WHITE);

    // Draw board
    for (int y = 0; y < board_height; ++y) {
      for (int x = 0; x < board_width; ++x) {
        const uint8_t cell_number = getNumber(board[y][x]);
        const uint8_t cell_display_state = getDisplayState(board[y][x]);
        const Vector2 cell_top_left = Vector2Add(top_left, (Vector2){x * 20.0f + 1.0f, y * 20.0f + 1.0f});
        const Vector2 cell_size = {18.0f, 18.0f};
        if (cell_display_state == cell_display_state_closed) {
          DrawRectangleV(cell_top_left, cell_size, BLACK);
        } else if (cell_display_state == cell_display_state_open) {
          DrawRectangleV(cell_top_left, cell_size, LIGHTGRAY);
          if (cell_number != 0) {
            char str[2];
            sprintf(str, "%u", cell_number);
            DrawText(str, cell_top_left.x + 4, cell_top_left.y, 20, number_colors[cell_number]);
          }
        } else if (cell_display_state == cell_display_state_flagged) {
          DrawRectangleV(cell_top_left, cell_size, BLACK);
          DrawText("F", cell_top_left.x + 4, cell_top_left.y, 20, RED);
        } else if (cell_display_state == cell_display_state_mine) {
          DrawRectangleV(cell_top_left, cell_size, LIGHTGRAY);
          DrawText("M", cell_top_left.x + 4, cell_top_left.y, 20, BLACK);
        } else if (cell_display_state == cell_display_state_mistake) {
          DrawRectangleV(cell_top_left, cell_size, RED);
          DrawText("M", cell_top_left.x + 4, cell_top_left.y, 20, BLACK);
        } else if (cell_display_state == cell_display_state_flag_mistake) {
          DrawRectangleV(cell_top_left, cell_size, LIGHTGRAY);
          DrawText("M", cell_top_left.x + 4, cell_top_left.y, 20, RED);
        } else if (cell_display_state == cell_display_state_press) {
          DrawRectangleV(cell_top_left, cell_size, LIGHTGRAY);
        }
      }
    }

    // Draw UI
    char mines_left_str[10];
    sprintf(mines_left_str, "%d", mines_left);
    DrawText(mines_left_str, 20, 20, 40, BLACK);

    EndDrawing();
  }

  CloseWindow();

  free(board);

  return 0;
}
