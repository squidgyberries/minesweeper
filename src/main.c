#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <raylib.h>
#include <raymath.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define BOARD(x, y) (board[(y) * board_width + (x)])

const Color number_colors[] = {LIGHTGRAY, BLUE, GREEN, RED, DARKBLUE, MAROON, SKYBLUE, BLACK, GRAY};

int board_width = 9;
int board_height = 9;
int num_mines = 10;
int mines_left = 10;

int window_width;
int window_height;

uint32_t timer = 0;
bool timer_running = false;
double timer_start = 0.0;
bool game_running = true;
bool is_first_open = true;

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

// -1: mistake
// 0: successful
// 1: didn't open
int openCell(uint8_t *board, int x, int y) {
  if (getDisplayState(BOARD(x, y)) != cell_display_state_closed) {
    return 1;
  }
  if (getNumber(BOARD(x, y)) == 9) {
    BOARD(x, y) = setDisplayState(BOARD(x, y), cell_display_state_mistake);
    return -1;
  }

  BOARD(x, y) = setDisplayState(BOARD(x, y), cell_display_state_open);
  const bool left_edge = x == 0;
  const bool right_edge = x == (board_width - 1);
  const bool top_edge = y == 0;
  const bool bottom_edge = y == (board_height - 1);
  if (getNumber(BOARD(x, y)) == 0) {
    if (!left_edge) {
      openCell(board, x - 1, y);
      if (!top_edge) {
        openCell(board, x - 1, y - 1);
      }
      if (!bottom_edge) {
        openCell(board, x - 1, y + 1);
      }
    }
    if (!right_edge) {
      openCell(board, x + 1, y);
      if (!top_edge) {
        openCell(board, x + 1, y - 1);
      }
      if (!bottom_edge) {
        openCell(board, x + 1, y + 1);
      }
    }
    if (!top_edge) {
      openCell(board, x, y - 1);
    }
    if (!bottom_edge) {
      openCell(board, x, y + 1);
    }
  }
  return 0;
}

void toggleFlagged(uint8_t *board, int x, int y) {
  if (getDisplayState(BOARD(x, y)) != cell_display_state_flagged) {
    BOARD(x, y) = setDisplayState(BOARD(x, y), cell_display_state_flagged);
    --mines_left;
  } else {
    BOARD(x, y) = setDisplayState(BOARD(x, y), cell_display_state_closed);
    ++mines_left;
  }
}

void generateMines(uint8_t *board, int start_x, int start_y) {
  BOARD(start_x, start_y) = setDisplayState(10, cell_display_state_closed);

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
        BOARD(start_x - 1, start_y - 1) = setDisplayState(10, getDisplayState(BOARD(start_x - 1, start_y - 1)));
        ++start_neighbors;
      }
    }
    if (available_start_neighbors-- > 0) {
      BOARD(start_x, start_y - 1) = setDisplayState(10, getDisplayState(BOARD(start_x, start_y - 1)));
      ++start_neighbors;
    }
    if (!start_right_edge) {
      if (available_start_neighbors-- > 0) {
        BOARD(start_x + 1, start_y - 1) = setDisplayState(10, getDisplayState(BOARD(start_x + 1, start_y - 1)));
        ++start_neighbors;
      }
    }
  }
  if (!start_left_edge) {
    if (available_start_neighbors-- > 0) {
      BOARD(start_x - 1, start_y) = setDisplayState(10, getDisplayState(BOARD(start_x - 1, start_y)));
      ++start_neighbors;
    }
  }
  if (!start_right_edge) {
    if (available_start_neighbors-- > 0) {
      BOARD(start_x + 1, start_y) = setDisplayState(10, getDisplayState(BOARD(start_x + 1, start_y)));
      ++start_neighbors;
    }
  }
  if (!start_bottom_edge) {
    if (!start_left_edge) {
      if (available_start_neighbors-- > 0) {
        BOARD(start_x - 1, start_y + 1) = setDisplayState(10, getDisplayState(BOARD(start_x - 1, start_y + 1)));
        ++start_neighbors;
      }
    }
    if (available_start_neighbors-- > 0) {
      BOARD(start_x, start_y + 1) = setDisplayState(10, getDisplayState(BOARD(start_x, start_y + 1)));
      ++start_neighbors;
    }
    if (!start_right_edge) {
      if (available_start_neighbors-- > 0) {
        BOARD(start_x + 1, start_y + 1) = setDisplayState(10, getDisplayState(BOARD(start_x + 1, start_y + 1)));
        ++start_neighbors;
      }
    }
  }

  int available_cells = board_width * board_height - (1 + start_neighbors);
  for (int i = 0; i < num_mines; ++i) {
    int num = randint(available_cells);
    for (int y = 0; y < board_height; ++y) {
      for (int x = 0; x < board_width; ++x) {
        if (getNumber(BOARD(x, y)) == 0) {
          if (num-- == 0) {
            BOARD(x, y) = setDisplayState(9, getDisplayState(BOARD(x, y)));
          }
        }
      }
    }
    --available_cells;
  }
  for (int y = 0; y < board_height; ++y) {
    for (int x = 0; x < board_width; ++x) {
      if (getNumber(BOARD(x, y)) == 9) {
        continue;
      }
      const bool left_edge = x == 0;
      const bool right_edge = x == (board_width - 1);
      const bool top_edge = y == 0;
      const bool bottom_edge = y == (board_height - 1);
      // clang-format off
      const uint8_t neighbors = (!left_edge && !top_edge && getNumber(BOARD(x - 1, y - 1)) == 9) +
                                (!left_edge && getNumber(BOARD(x - 1, y)) == 9) +
                                (!left_edge && !bottom_edge && getNumber(BOARD(x - 1, y + 1)) == 9) +

                                (!right_edge && !top_edge && getNumber(BOARD(x + 1, y - 1)) == 9) +
                                (!right_edge && getNumber(BOARD(x + 1, y)) == 9) +
                                (!right_edge && !bottom_edge && getNumber(BOARD(x + 1, y + 1)) == 9) +

                                (!top_edge && getNumber(BOARD(x, y - 1)) == 9) +
                                (!bottom_edge && getNumber(BOARD(x, y + 1)) == 9);
      // clang-format on
      BOARD(x, y) = setDisplayState(neighbors, getDisplayState(BOARD(x, y)));
    }
  }
}

void revealMines(uint8_t *board) {
  for (int y = 0; y < board_height; ++y) {
    for (int x = 0; x < board_width; ++x) {
      const uint8_t display_state = getDisplayState(BOARD(x, y));
      const uint8_t number = getNumber(BOARD(x, y));
      if (number == 9 && display_state != cell_display_state_mistake && display_state != cell_display_state_flagged) {
        BOARD(x, y) = setDisplayState(number, cell_display_state_mine);
      }
      if (display_state == cell_display_state_flagged && number != 9) {
        BOARD(x, y) = setDisplayState(number, cell_display_state_flag_mistake);
      }
    }
  }
}

void revealFlags(uint8_t *board) {
  for (int y = 0; y < board_height; ++y) {
    for (int x = 0; x < board_width; ++x) {
      if (getNumber(BOARD(x, y)) == 9 && getDisplayState(BOARD(x, y)) != cell_display_state_flagged) {
        BOARD(x, y) = setDisplayState(BOARD(x, y), cell_display_state_flagged);
      }
    }
  }
}

bool checkWin(uint8_t *board) {
  for (int y = 0; y < board_height; ++y) {
    for (int x = 0; x < board_width; ++x) {
      if (getNumber(BOARD(x, y)) != 9 && getDisplayState(BOARD(x, y)) != cell_display_state_open) {
        return false;
      }
    }
  }
  return true;
}

void resetGame(uint8_t *board) {
  memset(board, 0, sizeof(uint8_t) * board_width * board_height);
  mines_left = num_mines;
  game_running = true;
  is_first_open = true;
  timer_running = false;
  timer = 0;
}

int main(void) {
  SetConfigFlags(FLAG_VSYNC_HINT);
  window_width = 40 + board_width * 20;
  window_height = 110 + board_height * 20;
  InitWindow(window_width, window_height, "minesweeper");

  srand(time(NULL));

  // Generate board
  // clang-format off
  // uint8_t (*board)[board_width] = calloc(1, sizeof(uint8_t[board_height][board_width]));
  // uint8_t (*board)[board_width] = calloc(1, sizeof(uint8_t * board_width * board_height));
  // uint8_t (*board)[board_width] = calloc(board_height, sizeof(*board));
  // clang-format on
  // uint8_t(*board)[board_width] = calloc(board_width * board_height, sizeof(uint8_t));
  if (num_mines > board_width * board_height - 1) {
    num_mines = board_width * board_height - 1;
    mines_left = num_mines;
  }
  uint8_t *board = malloc(sizeof(uint8_t) * board_width * board_height);
  resetGame(board);

  int last_press_x = 0;
  int last_press_y = 0;

  int mines_text_length = snprintf(NULL, 0, "%d", mines_left) + 1;
  char *mines_text = malloc(sizeof(char) * mines_text_length);
  snprintf(mines_text, mines_text_length, "%d", mines_left);
  int timer_text_length = snprintf(NULL, 0, "%u", timer) + 1;
  char *timer_text = malloc(sizeof(char) * timer_text_length);
  snprintf(timer_text, timer_text_length, "%u", timer);
  while (!WindowShouldClose()) {
    Vector2 top_left = (Vector2){20.0f, 90.0f};

    Vector2 mouse_pos = GetMousePosition();

    if (game_running) {
      int mouse_cell_x, mouse_cell_y;
      const bool mouse_is_on_cell = findCollisionCell(mouse_pos, top_left, &mouse_cell_x, &mouse_cell_y);
      const uint8_t mouse_display_state = getDisplayState(BOARD(mouse_cell_x, mouse_cell_y));
      if (getDisplayState(BOARD(last_press_x, last_press_y)) == cell_display_state_press) {
        BOARD(last_press_x, last_press_y) = setDisplayState(BOARD(last_press_x, last_press_y), cell_display_state_closed);
      }
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (mouse_is_on_cell && mouse_display_state == cell_display_state_closed) {
          BOARD(mouse_cell_x, mouse_cell_y) = setDisplayState(BOARD(mouse_cell_x, mouse_cell_y), cell_display_state_press);
          last_press_x = mouse_cell_x;
          last_press_y = mouse_cell_y;
        }
      }
      if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (mouse_is_on_cell) {
          if (is_first_open) {
            generateMines(board, mouse_cell_x, mouse_cell_y);
            is_first_open = false;
            timer_running = true;
            timer_start = GetTime();
          }
          int open_result = openCell(board, mouse_cell_x, mouse_cell_y);
          if (open_result == -1) {
            // GAME OVER
            timer_running = false;
            game_running = false;
            revealMines(board);
          } else if (open_result == 0) {
            if (checkWin(board)) {
              timer_running = false;
              game_running = false;
              revealFlags(board);
            }
          }
        }
      }
      if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        if (mouse_is_on_cell) {
          if (mouse_display_state == cell_display_state_closed || mouse_display_state == cell_display_state_flagged) {
            toggleFlagged(board, mouse_cell_x, mouse_cell_y);
          }
        }
      }
    }

    BeginDrawing();

    ClearBackground(WHITE);

    // Draw board
    for (int y = 0; y < board_height; ++y) {
      for (int x = 0; x < board_width; ++x) {
        const uint8_t cell_number = getNumber(BOARD(x, y));
        const uint8_t cell_display_state = getDisplayState(BOARD(x, y));
        const Vector2 cell_top_left = Vector2Add(top_left, (Vector2){x * 20.0f + 1.0f, y * 20.0f + 1.0f});
        const Vector2 cell_size = {18.0f, 18.0f};
        if (cell_display_state == cell_display_state_closed) {
          DrawRectangleV(cell_top_left, cell_size, BLACK);
        } else if (cell_display_state == cell_display_state_open) {
          DrawRectangleV(cell_top_left, cell_size, LIGHTGRAY);
          if (cell_number != 0) {
            char str[2];
            snprintf(str, 2, "%u", cell_number);
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
    // Mines counter
    int new_text_length;
    if ((new_text_length = snprintf(NULL, 0, "%d", mines_left) + 1) > mines_text_length) {
      mines_text_length = new_text_length;
      mines_text = realloc(mines_text, sizeof(char) * mines_text_length);
    }
    snprintf(mines_text, mines_text_length, "%d", mines_left);
    DrawText(mines_text, 20, 40, 30, BLACK);

    // Timer
    if (timer_running) {
      timer = (int)(GetTime() - timer_start);
    }
    if ((new_text_length = snprintf(NULL, 0, "%u", timer) + 1) > timer_text_length) {
      timer_text_length = new_text_length;
      timer_text = realloc(timer_text, sizeof(char) * timer_text_length);
    }
    snprintf(timer_text, timer_text_length, "%u", timer);
    DrawText(timer_text, window_width - 20 - MeasureText(timer_text, 30), 40, 30, BLACK);

    // Button
    Rectangle button = {window_width / 2.0f - 15.0f, 40.0f, 30.0f, 30.0f};
    if (CheckCollisionPointRec(mouse_pos, button) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      DrawRectangleRec(button, LIGHTGRAY);
    } else {
      DrawRectangleRec(button, BLACK);
      if (CheckCollisionPointRec(mouse_pos, button) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        resetGame(board);
      }
    }

    // Dialogue
    static bool show_game_dialog = false;
    if (GuiButton((Rectangle){0.0f, 0.0f, (float)window_width, 20.0f}, "Game")) {
      show_game_dialog = true;
      game_running = false;
    }

    Vector2 dialog_top_left = {(float)window_width / 2.0f - 90.0f, 10.0f * (float)board_height};
    if (show_game_dialog) {
      int result = GuiWindowBox((Rectangle){dialog_top_left.x, dialog_top_left.y, 180.0f, 180.0f}, "Game");

      GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 28.0f, 160.0f, 20.0f}, "Mode:");
      static int active = 0;
      static bool dropdown_active = false;
      GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 73.0f, 160.0f, 20.0f}, "Width:");
      GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 98.0f, 160.0f, 20.0f}, "Height:");
      GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 123.0f, 160.0f, 20.0f}, "Mines:");

      static int custom_board_width = 30;
      static int custom_board_height = 30;
      static int custom_mines = 150;
      if (active == 0) {
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
        GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 73.0f, 160.0f, 20.0f}, "9");
        GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 98.0f, 160.0f, 20.0f}, "9");
        GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 123.0f, 160.0f, 20.0f}, "10");
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
      } else if (active == 1) {
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
        GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 73.0f, 160.0f, 20.0f}, "16");
        GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 98.0f, 160.0f, 20.0f}, "16");
        GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 123.0f, 160.0f, 20.0f}, "40");
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
      } else if (active == 2) {
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
        GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 73.0f, 160.0f, 20.0f}, "30");
        GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 98.0f, 160.0f, 20.0f}, "16");
        GuiLabel((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 123.0f, 160.0f, 20.0f}, "99");
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
      } else if (active == 3) {
        static bool width_edit_mode = false;
        if (GuiValueBox((Rectangle){dialog_top_left.x + 90.0f, dialog_top_left.y + 73.0f, 80.0f, 20.0f}, NULL, &custom_board_width, 1, 1000,
                        width_edit_mode)) {
          width_edit_mode = !width_edit_mode;
        }

        static bool height_edit_mode = false;
        if (GuiValueBox((Rectangle){dialog_top_left.x + 90.0f, dialog_top_left.y + 98.0f, 80.0f, 20.0f}, NULL, &custom_board_height, 1, 1000,
                        height_edit_mode)) {
          height_edit_mode = !height_edit_mode;
        }

        static bool mines_edit_mode = false;
        if (GuiValueBox((Rectangle){dialog_top_left.x + 90.0f, dialog_top_left.y + 123.0f, 80.0f, 20.0f}, NULL, &custom_mines, 1, 1000,
                        mines_edit_mode)) {
          mines_edit_mode = !mines_edit_mode;
        }
      }

      if (GuiButton((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 150.0f, 160.0f, 20.0f}, "Start Game")) {
        show_game_dialog = false;
        game_running = true;
        if (active == 0) {
          board_width = 9;
          board_height = 9;
          num_mines = 10;
        } else if (active == 1) {
          board_width = 16;
          board_height = 16;
          num_mines = 40;
        } else if (active == 2) {
          board_width = 30;
          board_height = 16;
          num_mines = 99;
        } else if (active == 3) {
          board_width = custom_board_width;
          board_height = custom_board_height;
          num_mines = custom_mines;
        }
        board = realloc(board, sizeof(uint8_t) * board_width * board_height);
        window_width = 40 + board_width * 20;
        window_height = 110 + board_height * 20;
        SetWindowSize(window_width, window_height);
        resetGame(board);
      }

      if (GuiDropdownBox((Rectangle){dialog_top_left.x + 10.0f, dialog_top_left.y + 48.0f, 160.0f, 20.0f}, "Beginner;Intermediate;Expert;Custom", &active,
                         dropdown_active)) {
        dropdown_active = !dropdown_active;
      }

      if (result) {
        show_game_dialog = false;
        game_running = true;
      }
    }

    EndDrawing();
  }

  CloseWindow();

  free(mines_text);
  free(timer_text);
  free(board);

  return 0;
}
