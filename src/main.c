#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <raylib.h>
#include <raymath.h>

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
  --n;
  if (n == RAND_MAX) {
    return rand();
  } else {
    assert(n <= RAND_MAX);

    int end = RAND_MAX / n;
    assert(end > 0);
    end *= n;

    int r;
    while ((r = rand()) >= end)
      ;

    return r % n;
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

  uint8_t board[9][9] = {0};
  for (int i = 0; i < 10; ++i) {
    int x, y;
    do {
      x = randint(9);
      y = randint(9);
    } while (getNumber(board[y][x]) == 9);
    board[y][x] = setDisplayState(9, cell_display_state_closed);
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
      for (int y = 0; y < 9; ++y) {
        for (int x = 0; x < 9; ++x) {
          if (CheckCollisionPointRec(mouse_pos,
                                     (Rectangle){top_left.x + x * 20.0f + 2.0f,
                                                 top_left.y + y * 20.0f + 2.0f,
                                                 16.0f, 16.0f})) {
            if (getNumber(board[y][x]) != 9) {
              board[y][x] = setDisplayState(board[y][x], cell_display_state_open);
            } else {
              board[y][x] = setDisplayState(board[y][x], cell_display_state_mistake);
            }
          }
        }
      }
    }
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
      for (int y = 0; y < 9; ++y) {
        for (int x = 0; x < 9; ++x) {
          if (CheckCollisionPointRec(mouse_pos,
                                     (Rectangle){top_left.x + x * 20.0f + 2.0f,
                                                 top_left.y + y * 20.0f + 2.0f,
                                                 16.0f, 16.0f})) {
            if (getDisplayState(board[y][x]) != cell_display_state_flagged) {
              board[y][x] = setDisplayState(board[y][x], cell_display_state_flagged);
            } else {
              board[y][x] = setDisplayState(board[y][x], cell_display_state_closed);
            }
          }
        }
      }
    }

    BeginDrawing();

    ClearBackground(WHITE);

    for (int y = 0; y < 9; ++y) {
      for (int x = 0; x < 9; ++x) {
        if (getDisplayState(board[y][x]) == cell_display_state_closed) {
          DrawRectangleV(Vector2Add(top_left, (Vector2){x * 20.0f + 2.0f,
                                                        y * 20.0f + 2.0f}),
                         (Vector2){16.0f, 16.0f}, BLACK);
        } else if (getDisplayState(board[y][x]) == cell_display_state_open) {
          DrawRectangleV(Vector2Add(top_left, (Vector2){x * 20.0f + 2.0f,
                                                        y * 20.0f + 2.0f}),
                         (Vector2){16.0f, 16.0f}, LIGHTGRAY);
          if (getNumber(board[y][x]) != 0) {
            char str[2];
            sprintf(str, "%u", getNumber(board[y][x]));
            Color color = BLUE;
            switch (getNumber(board[y][x])) {
              case 2:
                color = GREEN;
                break;
              case 3:
                color = RED;
                break;
              case 4:
                color = DARKBLUE;
                break;
              case 5:
                color = MAROON;
                break;
              case 6:
                color = SKYBLUE;
                break;
              case 7:
                color = BLACK;
                break;
              case 8:
                color = GRAY;
                break;
            }
            DrawText(str, top_left.x + x * 20.0f + 5,
                     top_left.y + y * 20.0f + 2, 20, color);
          }
        } else if (getDisplayState(board[y][x]) == cell_display_state_flagged) {
          DrawRectangleV(Vector2Add(top_left, (Vector2){x * 20.0f + 2.0f,
                                                        y * 20.0f + 2.0f}),
                         (Vector2){16.0f, 16.0f}, BLACK);
          DrawText("F", top_left.x + x * 20.0f + 5, top_left.y + y * 20.0f + 2,
                   20, RED);
        } else if (getDisplayState(board[y][x]) == cell_display_state_mistake) {
          DrawRectangleV(Vector2Add(top_left, (Vector2){x * 20.0f + 2.0f,
                                                        y * 20.0f + 2.0f}),
                         (Vector2){16.0f, 16.0f}, RED);
          DrawText("M", top_left.x + x * 20.0f + 5, top_left.y + y * 20.0f + 2,
                   20, BLACK);
        } else if (getDisplayState(board[y][x]) == cell_display_state_mine) {
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
