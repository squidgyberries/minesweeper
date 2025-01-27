#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <raylib.h>
#include <raymath.h>

const Color number_colors[] = {LIGHTGRAY, BLUE,    GREEN, RED, DARKBLUE,
                               MAROON,    SKYBLUE, BLACK, GRAY};

const uint8_t number_mask = 0x0F; // 0b00001111

const uint8_t cell_display_state_open = 0;
const uint8_t cell_display_state_closed = 1;
const uint8_t cell_display_state_flagged = 2;
const uint8_t cell_display_state_mine = 3;
const uint8_t cell_display_state_mistake = 4;
const uint8_t cell_display_state_press = 5;
static inline uint8_t getDisplayState(uint8_t cell) { return cell >> 4; }

static inline uint8_t setDisplayState(uint8_t cell, uint8_t state) {
  return (cell & number_mask) | (state << 4);
}

static inline uint8_t getNumber(uint8_t cell) { return cell & number_mask; }

// [0, n)
int randint(int n) {
  int limit = RAND_MAX - (RAND_MAX % n);

  int r;
  while ((r = rand()) >= limit)
    ;

  return r % n;
}

bool findCollisionCell(Vector2 mouse_pos, Vector2 top_left, int *out_x,
                       int *out_y) {
  for (int x = 0; x < 9; ++x) {
    for (int y = 0; y < 9; ++y) {
      Rectangle cell_rect = {top_left.x + x * 20.0f + 2.0f,
                             top_left.y + y * 20.0f + 2.0f, 16.0f, 16.0f};
      if (CheckCollisionPointRec(mouse_pos, cell_rect)) {
        *out_x = x;
        *out_y = y;
        return true;
      }
    }
  }
  return false;
}

static inline void openCellIfClosed(uint8_t board[9][9], int x, int y);

void openCell(uint8_t board[9][9], int x, int y) {
  if (getNumber(board[y][x]) != 9) {
    board[y][x] = setDisplayState(board[y][x], cell_display_state_open);
  } else {
    board[y][x] = setDisplayState(board[y][x], cell_display_state_mistake);
  }
  if (getNumber(board[y][x]) == 0) {
    if (x > 0) {
      openCellIfClosed(board, x - 1, y);
      if (y > 0) {
        openCellIfClosed(board, x - 1, y - 1);
      }
      if (y < 8) {
        openCellIfClosed(board, x - 1, y + 1);
      }
    }
    if (x < 8) {
      openCellIfClosed(board, x + 1, y);
      if (y > 0) {
        openCellIfClosed(board, x + 1, y - 1);
      }
      if (y < 8) {
        openCellIfClosed(board, x + 1, y + 1);
      }
    }
    if (y > 0) {
      openCellIfClosed(board, x, y - 1);
    }
    if (y < 8) {
      openCellIfClosed(board, x, y + 1);
    }
  }
}

static inline void openCellIfClosed(uint8_t board[9][9], int x, int y) {
  if (getDisplayState(board[y][x]) == cell_display_state_closed) {
    openCell(board, x, y);
  }
}

void toggleFlagged(uint8_t board[9][9], int x, int y) {
  if (getDisplayState(board[y][x]) != cell_display_state_flagged) {
    board[y][x] = setDisplayState(board[y][x], cell_display_state_flagged);
  } else {
    board[y][x] = setDisplayState(board[y][x], cell_display_state_closed);
  }
}

int main(void) {
  printf("hello world\n");

  // Possible window flags
  /*
  FLAG_VSYNC_HINT
  FLAG_FULLSCREEN_MODE    -> not working properly -> wrong scaling!
  FLAG_WINDOW_RESIZABLE
  FLAG_WINDOW_UNDECORATED
  FLAG_WINDOW_TRANSPARENT
  FLAG_WINDOW_HIDDEN
  FLAG_WINDOW_MINIMIZED   -> Not supported on window creation
  FLAG_WINDOW_MAXIMIZED   -> Not supported on window creation
  FLAG_WINDOW_UNFOCUSED
  FLAG_WINDOW_TOPMOST
  FLAG_WINDOW_HIGHDPI     -> errors after minimize-resize, fb size is
  recalculated FLAG_WINDOW_ALWAYS_RUN FLAG_MSAA_4X_HINT
  */
  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(220, 220, "minesweeper");

  srand(time(NULL));

  // Generate board
  uint8_t board[9][9] = {0};
  int available_cells = 9 * 9;
  for (int i = 0; i < 10; ++i) {
    int num = randint(available_cells);
    for (int y = 0; y < 9; ++y) {
      for (int x = 0; x < 9; ++x) {
        if (getNumber(board[y][x]) != 9) {
          if (num-- == 0) {
            board[y][x] = setDisplayState(9, cell_display_state_closed);
          }
        }
      }
    }
    --available_cells;
  }
  for (int y = 0; y < 9; ++y) {
    for (int x = 0; x < 9; ++x) {
      if (getNumber(board[y][x]) == 9) {
        continue;
      }
      bool left_edge = x == 0;
      bool right_edge = x == 8;
      bool top_edge = y == 0;
      bool bottom_edge = y == 8;
      // printf("%u%u%u%u\n", left_edge, right_edge, top_edge, bottom_edge);
      // clang-format off
      uint8_t neighbors = (!left_edge && !top_edge && getNumber(board[y - 1][x - 1]) == 9) +
                          (!left_edge && getNumber(board[y][x - 1]) == 9) +
                          (!left_edge && !bottom_edge && getNumber(board[y + 1][x - 1]) == 9) +

                          (!right_edge && !top_edge && getNumber(board[y - 1][x + 1]) == 9) +
                          (!right_edge && getNumber(board[y][x + 1]) == 9) +
                          (!right_edge && !bottom_edge && getNumber(board[y + 1][x + 1]) == 9) +

                          (!top_edge && getNumber(board[y - 1][x]) == 9) +
                          (!bottom_edge && getNumber(board[y + 1][x]) == 9);
      // clang-format on
      board[y][x] = setDisplayState(neighbors, cell_display_state_closed);
    }
  }

  while (!WindowShouldClose()) {
    Vector2 top_left = (Vector2){20.0f, 20.0f};

    Vector2 mouse_pos = GetMousePosition();
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
      int x, y;
      if (findCollisionCell(mouse_pos, top_left, &x, &y)) {
        openCell(board, x, y);
      }
    }
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
      int x, y;
      if (findCollisionCell(mouse_pos, top_left, &x, &y)) {
        toggleFlagged(board, x, y);
      }
    }

    BeginDrawing();

    ClearBackground(WHITE);

    for (int y = 0; y < 9; ++y) {
      for (int x = 0; x < 9; ++x) {
        uint8_t cell_number = getNumber(board[y][x]);
        uint8_t cell_display_state = getDisplayState(board[y][x]);
        if (cell_display_state == cell_display_state_closed) {
          DrawRectangleV(Vector2Add(top_left, (Vector2){x * 20.0f + 2.0f,
                                                        y * 20.0f + 2.0f}),
                         (Vector2){16.0f, 16.0f}, BLACK);
        } else if (cell_display_state == cell_display_state_open) {
          DrawRectangleV(Vector2Add(top_left, (Vector2){x * 20.0f + 2.0f,
                                                        y * 20.0f + 2.0f}),
                         (Vector2){16.0f, 16.0f}, LIGHTGRAY);
          if (cell_number != 0) {
            char str[2];
            sprintf(str, "%u", cell_number);
            DrawText(str, top_left.x + x * 20.0f + 5,
                     top_left.y + y * 20.0f + 2, 20,
                     number_colors[cell_number]);
          }
        } else if (cell_display_state == cell_display_state_flagged) {
          DrawRectangleV(Vector2Add(top_left, (Vector2){x * 20.0f + 2.0f,
                                                        y * 20.0f + 2.0f}),
                         (Vector2){16.0f, 16.0f}, BLACK);
          DrawText("F", top_left.x + x * 20.0f + 5, top_left.y + y * 20.0f + 2,
                   20, RED);
        } else if (cell_display_state == cell_display_state_mistake) {
          DrawRectangleV(Vector2Add(top_left, (Vector2){x * 20.0f + 2.0f,
                                                        y * 20.0f + 2.0f}),
                         (Vector2){16.0f, 16.0f}, RED);
          DrawText("M", top_left.x + x * 20.0f + 5, top_left.y + y * 20.0f + 2,
                   20, BLACK);
        } else if (cell_display_state == cell_display_state_mine) {
          DrawRectangleV(Vector2Add(top_left, (Vector2){x * 20.0f + 2.0f,
                                                        y * 20.0f + 2.0f}),
                         (Vector2){16.0f, 16.0f}, LIGHTGRAY);
          DrawText("M", top_left.x + x * 20.0f + 5, top_left.y + y * 20.0f + 2,
                   20, BLACK);
        }
      }
    }

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
