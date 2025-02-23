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

int board_width = 9;
int board_height = 9;
int num_mines = 10;
int mines_left = 10;

// beginner, intermediate, expert
// board_width, board_height, num_mines
int difficulty_nums[] = {9, 9, 10, 16, 16, 40, 30, 16, 99};
char *difficulty_strs[] = {"9", "9", "10", "16", "16", "40", "30", "16", "99"};

int render_width;
int render_height;
float scale = 1.0f;

// clang-format off
Rectangle atlas_rects[] = {
  {0.0f, 0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 20.0f, 20.0f},
  {20.0f, 0.0f, 20.0f, 20.0f},
  {40.0f, 0.0f, 20.0f, 20.0f},
  {60.0f, 0.0f, 20.0f, 20.0f},
  {0.0f, 20.0f, 20.0f, 20.0f},
  {20.0f, 20.0f, 20.0f, 20.0f},
  {40.0f, 20.0f, 20.0f, 20.0f},
  {60.0f, 20.0f, 20.0f, 20.0f},
  {0.0f, 40.0f, 20.0f, 20.0f},
  {20.0f, 40.0f, 20.0f, 20.0f},
  {40.0f, 40.0f, 20.0f, 20.0f},
  {60.0f, 40.0f, 20.0f, 20.0f},
  {0.0f, 60.0f, 20.0f, 20.0f},
  {20.0f, 60.0f, 20.0f, 20.0f},
  {40.0f, 60.0f, 20.0f, 20.0f},
  {60.0f, 60.0f, 20.0f, 20.0f},
  {0.0f, 80.0f, 20.0f, 20.0f},
  {20.0f, 80.0f, 20.0f, 20.0f},
  {40.0f, 80.0f, 20.0f, 20.0f},
  {60.0f, 80.0f, 20.0f, 20.0f},
  {0.0f, 100.0f, 20.0f, 20.0f}
};
// clang-format on

typedef enum CellRects {
  ATLAS_FLAGGED = 9,
  ATLAS_MINE,
  ATLAS_MISTAKE,
  ATLAS_FLAG_MISTAKE,
  ATLAS_CLOSED,
  ATLAS_OPEN,
  ATLAS_BACKGROUND,
  ATLAS_FOREGROUND,
  ATLAS_FACE_SMILE,
  ATLAS_FACE_PRESSED,
  ATLAS_FACE_SCARED,
  ATLAS_FACE_DEAD,
  ATLAS_FACE_WIN
} CellRects;

uint32_t timer = 0;
bool timer_running = false;
double timer_start = 0.0;
bool game_running = true;
bool is_first_open = true;
bool game_over = false;
bool game_won = false;

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
        toggleFlagged(board, x, y);
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
  game_over = false;
  is_first_open = true;
  timer_running = false;
  timer = 0;
}

void resizeBoard(uint8_t **board, int new_width, int new_height, RenderTexture2D *render_target) {
  board_width = new_width;
  board_height = new_height;
  *board = realloc(*board, sizeof(uint8_t) * board_width * board_height);
  render_width = 40 + board_width * 20;
  render_height = 110 + board_height * 20;
  SetWindowSize(render_width * scale, render_height * scale);
  UnloadRenderTexture(*render_target);
  *render_target = LoadRenderTexture(render_width, render_height);
}

int main(void) {
  SetConfigFlags(FLAG_VSYNC_HINT);
  render_width = 40 + board_width * 20;
  render_height = 110 + board_height * 20;
  InitWindow(render_width * scale, render_height * scale, "minesweeper");

  srand(time(NULL));

  uint8_t *board = malloc(sizeof(uint8_t) * board_width * board_height);
  if (num_mines > board_width * board_height - 1) {
    num_mines = board_width * board_height - 1;
  }
  resetGame(board);

  int last_press_x = 0;
  int last_press_y = 0;

  int mines_text_length = snprintf(NULL, 0, "%d", mines_left) + 1;
  char *mines_text = malloc(sizeof(char) * mines_text_length);
  snprintf(mines_text, mines_text_length, "%d", mines_left);
  int timer_text_length = snprintf(NULL, 0, "%u", timer) + 1;
  char *timer_text = malloc(sizeof(char) * timer_text_length);
  snprintf(timer_text, timer_text_length, "%u", timer);

  Image texture_atlas_image = LoadImage("resources/texture_atlas.png");
  const Color background_color = GetImageColor(texture_atlas_image, atlas_rects[ATLAS_BACKGROUND].x, atlas_rects[ATLAS_BACKGROUND].y);
  const Color foreground_color = GetImageColor(texture_atlas_image, atlas_rects[ATLAS_FOREGROUND].x, atlas_rects[ATLAS_FOREGROUND].y);
  Texture2D texture_atlas = LoadTextureFromImage(texture_atlas_image);
  SetTextureFilter(texture_atlas, TEXTURE_FILTER_POINT);
  UnloadImage(texture_atlas_image);

  RenderTexture2D render_target = LoadRenderTexture(render_width, render_height);
  SetTextureFilter(render_target.texture, TEXTURE_FILTER_POINT);

  while (!WindowShouldClose()) {
    Vector2 top_left = (Vector2){20.0f, 90.0f};

    // Vector2 real_mouse_pos = GetMousePosition();
    // Vector2 mouse_pos = Vector2Scale(real_mouse_pos, 1.0f / scale);
    
    // Apply the same transformation as the virtual mouse to the real mouse (i.e. to work with raygui)
    // SetMouseOffset(-(GetScreenWidth() - (gameScreenWidth*scale))*0.5f, -(GetScreenHeight() - (gameScreenHeight*scale))*0.5f);
    SetMouseScale(1.0f / scale, 1.0f / scale);
    Vector2 mouse_pos = GetMousePosition();

    int mouse_cell_x, mouse_cell_y;
    const bool mouse_is_on_cell = findCollisionCell(mouse_pos, top_left, &mouse_cell_x, &mouse_cell_y);
    if (game_running) {
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
            game_won = false;
            game_over = true;
            revealMines(board);
          } else if (open_result == 0) {
            if (checkWin(board)) {
              timer_running = false;
              game_running = false;
              game_won = true;
              game_over = true;
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

    // BeginDrawing();
    BeginTextureMode(render_target);

    ClearBackground(background_color);

    // Draw board
    for (int y = 0; y < board_height; ++y) {
      for (int x = 0; x < board_width; ++x) {
        const uint8_t cell_number = getNumber(BOARD(x, y));
        const uint8_t cell_display_state = getDisplayState(BOARD(x, y));
        const Vector2 cell_top_left = Vector2Add(top_left, (Vector2){x * 20.0f, y * 20.0f});
        if (cell_display_state == cell_display_state_closed) {
          DrawTextureRec(texture_atlas, atlas_rects[ATLAS_CLOSED], cell_top_left, WHITE);
        } else if (cell_display_state == cell_display_state_open) {
          DrawTextureRec(texture_atlas, atlas_rects[ATLAS_OPEN], cell_top_left, WHITE);
          if (cell_number > 0 && cell_number < 9) {
            DrawTextureRec(texture_atlas, atlas_rects[cell_number], cell_top_left, WHITE);
          }
        } else if (cell_display_state == cell_display_state_flagged) {
          DrawTextureRec(texture_atlas, atlas_rects[ATLAS_CLOSED], cell_top_left, WHITE);
          DrawTextureRec(texture_atlas, atlas_rects[ATLAS_FLAGGED], cell_top_left, WHITE);
        } else if (cell_display_state == cell_display_state_mine) {
          DrawTextureRec(texture_atlas, atlas_rects[ATLAS_OPEN], cell_top_left, WHITE);
          DrawTextureRec(texture_atlas, atlas_rects[ATLAS_MINE], cell_top_left, WHITE);
        } else if (cell_display_state == cell_display_state_mistake) {
          DrawTextureRec(texture_atlas, atlas_rects[ATLAS_OPEN], cell_top_left, WHITE);
          DrawTextureRec(texture_atlas, atlas_rects[ATLAS_MISTAKE], cell_top_left, WHITE);
        } else if (cell_display_state == cell_display_state_flag_mistake) {
          DrawTextureRec(texture_atlas, atlas_rects[ATLAS_OPEN], cell_top_left, WHITE);
          DrawTextureRec(texture_atlas, atlas_rects[ATLAS_FLAG_MISTAKE], cell_top_left, WHITE);
        } else if (cell_display_state == cell_display_state_press) {
          DrawTextureRec(texture_atlas, atlas_rects[ATLAS_OPEN], cell_top_left, WHITE);
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
    DrawText(mines_text, 20, 40, 30, foreground_color);

    // Timer
    if (timer_running) {
      timer = (int)(GetTime() - timer_start);
    }
    if ((new_text_length = snprintf(NULL, 0, "%u", timer) + 1) > timer_text_length) {
      timer_text_length = new_text_length;
      timer_text = realloc(timer_text, sizeof(char) * timer_text_length);
    }
    snprintf(timer_text, timer_text_length, "%u", timer);
    DrawText(timer_text, render_width - 20 - MeasureText(timer_text, 30), 40, 30, foreground_color);

    // Button
    Rectangle button = {render_width * 0.5f - 20.0f, 35.0f, 40.0f, 40.0f};
    if (CheckCollisionPointRec(mouse_pos, button) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      DrawTexturePro(texture_atlas, atlas_rects[ATLAS_FACE_PRESSED], button, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    } else {
      DrawTexturePro(texture_atlas, atlas_rects[ATLAS_FACE_SMILE], button, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
      if (game_running) {
        if (game_over && game_won) {
          DrawTexturePro(texture_atlas, atlas_rects[ATLAS_FACE_WIN], button, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        } else if (game_over && !game_won) {
          DrawTexturePro(texture_atlas, atlas_rects[ATLAS_FACE_DEAD], button, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        } else if (mouse_is_on_cell && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
          DrawTexturePro(texture_atlas, atlas_rects[ATLAS_FACE_SCARED], button, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        }
      }
      if (CheckCollisionPointRec(mouse_pos, button) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        resetGame(board);
      }
    }

    // Dialogue buttons
    const float ui_height = 20.0f;
    static bool show_game_dialog = false;
    if (GuiButton((Rectangle){0.0f, 0.0f, (float)render_width * 0.5f, ui_height}, "Game Options")) {
      show_game_dialog = true;
      game_running = false;
      timer_running = false;
    }

    static bool show_display_dialog = false;
    static bool previous_timer_running = false;
    if (GuiButton((Rectangle){(float)render_width * 0.5f, 0.0f, (float)render_width * 0.5f, ui_height}, "Display Options")) {
      show_display_dialog = true;
      game_running = false;
      previous_timer_running = timer_running;
      timer_running = false;
    }

    // Middle of the board
    if (show_game_dialog) {
      game_running = false; // Make sure can't start game with dialogue open

      Rectangle dialog_bounds = {(float)render_width * 0.5f - 90.0f, 10.0f * (float)board_height, 180.0f, 180.0f};
      const int result = GuiWindowBox(dialog_bounds, "Game Options");

      Rectangle inner_bounds = {dialog_bounds.x + 10.0f, dialog_bounds.y + 28.0f, 160.0f, 142.0f};
      GuiLabel((Rectangle){inner_bounds.x, inner_bounds.y, inner_bounds.width, ui_height}, "Mode:");
      static int active = 0;
      static bool dropdown_active = false;
      GuiLabel((Rectangle){inner_bounds.x, inner_bounds.y + 45.0f, inner_bounds.width, ui_height}, "Width:");
      GuiLabel((Rectangle){inner_bounds.x, inner_bounds.y + 70.0f, inner_bounds.width, ui_height}, "Height:");
      GuiLabel((Rectangle){inner_bounds.x, inner_bounds.y + 95.0f, inner_bounds.width, ui_height}, "Mines:");

      static int custom_board_width = 30;
      static int custom_board_height = 30;
      static int custom_mines = 150;
      if (active != 3) {
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
        GuiLabel((Rectangle){inner_bounds.x, inner_bounds.y + 45.0f, inner_bounds.width, ui_height}, difficulty_strs[active * 3 + 0]);
        GuiLabel((Rectangle){inner_bounds.x, inner_bounds.y + 70.0f, inner_bounds.width, ui_height}, difficulty_strs[active * 3 + 1]);
        GuiLabel((Rectangle){inner_bounds.x, inner_bounds.y + 95.0f, inner_bounds.width, ui_height}, difficulty_strs[active * 3 + 2]);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
      } else {
        static bool width_edit_mode = false;
        if (GuiValueBox((Rectangle){inner_bounds.x + 80.0f, inner_bounds.y + 45.0f, inner_bounds.width * 0.5f, ui_height}, NULL,
                        &custom_board_width, 1, 1000, width_edit_mode)) {
          width_edit_mode = !width_edit_mode;
        }

        static bool height_edit_mode = false;
        if (GuiValueBox((Rectangle){inner_bounds.x + 80.0f, inner_bounds.y + 70.0f, inner_bounds.width * 0.5f, ui_height}, NULL,
                        &custom_board_height, 1, 1000, height_edit_mode)) {
          height_edit_mode = !height_edit_mode;
        }

        static bool mines_edit_mode = false;
        if (GuiValueBox((Rectangle){inner_bounds.x + 80.0f, inner_bounds.y + 95.0f, inner_bounds.width * 0.5f, ui_height}, NULL,
                        &custom_mines, 1, 1000, mines_edit_mode)) {
          mines_edit_mode = !mines_edit_mode;
        }
      }

      if (GuiButton((Rectangle){inner_bounds.x, dialog_bounds.y + 150.0f, inner_bounds.width, ui_height}, "Start Game")) {
        show_game_dialog = false;
        game_running = true;
        timer_running = false;
        if (active != 3) {
          resizeBoard(&board, difficulty_nums[active * 3 + 0], difficulty_nums[active * 3 + 1], &render_target);
          num_mines = difficulty_nums[active * 3 + 2];
        } else {
          resizeBoard(&board, custom_board_width, custom_board_height, &render_target);
          if (custom_mines > board_width * board_height - 1) {
            custom_mines = board_width * board_height - 1;
          }
          num_mines = custom_mines;
        }
        resetGame(board);
      }

      if (GuiDropdownBox((Rectangle){inner_bounds.x, inner_bounds.y + 20.0f, inner_bounds.width, ui_height},
                         "Beginner;Intermediate;Expert;Custom", &active, dropdown_active)) {
        dropdown_active = !dropdown_active;
      }

      if (result) {
        show_game_dialog = false;
        game_running = true;
        timer_running = false;
      }
    }

    if (show_display_dialog) {
      game_running = false; // Make sure can't start game with dialogue open

      Rectangle dialog_bounds = {(float)render_width * 0.5f - 90.0f, 10.0f * (float)board_height, 180.0f, 180.0f};
      const int result = GuiWindowBox(dialog_bounds, "Display Options");

      Rectangle inner_bounds = {dialog_bounds.x + 10.0f, dialog_bounds.y + 28.0f, 160.0f, 142.0f};
      GuiLabel((Rectangle){inner_bounds.x, inner_bounds.y, inner_bounds.width, ui_height}, "Display Scale:");
      static int active = 0;
      static bool dropdown_active = false;

      if (GuiButton((Rectangle){inner_bounds.x, dialog_bounds.y + 150.0f, inner_bounds.width, ui_height}, "Apply")) {
        show_display_dialog = false;
        game_running = true;
        timer_running = previous_timer_running;
        if (active == 0) {
          scale = 1.0f;
        } else {
          scale = 2.0f;
        }
        SetWindowSize(render_width * scale, render_height * scale);
      }

      if (GuiDropdownBox((Rectangle){inner_bounds.x, inner_bounds.y + 20.0f, inner_bounds.width, ui_height},
                         "1;2", &active, dropdown_active)) {
        dropdown_active = !dropdown_active;
      }

      if (result) {
        show_display_dialog = false;
        game_running = true;
        timer_running = previous_timer_running;
      }
    }

    // EndDrawing();
    EndTextureMode();

    BeginDrawing();

    DrawTexturePro(render_target.texture, (Rectangle){0.0f, 0.0f, (float)render_width, (float)-render_height},
                   (Rectangle){0.0f, 0.0f, (float)render_width * scale, (float)render_height * scale}, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    
    EndDrawing();
  }

  UnloadTexture(texture_atlas);

  UnloadRenderTexture(render_target);

  CloseWindow();

  free(mines_text);
  free(timer_text);
  free(board);

  return 0;
}
