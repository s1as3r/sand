// clang-format off
#include <stdlib.h>
#include <string.h>

#include <raylib.h>

#include "base.h"
// clang-format on

#define NUM_COLORS 5
// clang-format off
global const Color g_colors[NUM_COLORS] = {
  {  0, 228,  48, 255},
  {255, 203,   0, 255},
  {255, 109, 194, 255},
  {102, 191, 255, 255},
  {135,  60, 190, 255},
};
// clang-format on
global const Color g_color_empty = {0, 0, 0, 255};

typedef enum { CELL_STATE_EMPTY = 0, CELL_STATE_FILLED } CellState;

typedef struct {
  CellState state;
  u32 color_idx; // valid only if filled
} Cell;

global const Cell g_empty_cell = {.state = CELL_STATE_EMPTY, .color_idx = 0};

typedef struct {
  Cell *write;
  Cell *read;
  i32 nx, ny;
  i32 w, h;
} Grid;

inline Cell *_cell_at(Cell *cells, i32 nx, i32 x, i32 y) {
  return (cells + (y * nx) + x);
}

inline Cell get_cell(Grid *grid, i32 x, i32 y) {
  return *(_cell_at(grid->read, grid->nx, x, y));
}

inline void set_cell(Grid *grid, i32 x, i32 y, Cell cell) {
  *_cell_at(grid->write, grid->nx, x, y) = cell;
}

Grid new_grid(i32 nx, i32 ny, i32 w, i32 h) {
  Grid grid = {.nx = nx, .ny = ny, .h = h, .w = w};
  u32 n_bytes = sizeof(Cell) * (u32)nx * (u32)ny;
  grid.read = malloc(n_bytes);
  grid.write = malloc(n_bytes);

  for (i32 y = 0; y < ny; y++) {
    for (i32 x = 0; x < nx; x++) {
      *_cell_at(grid.read, grid.nx, x, y) = g_empty_cell;
      *_cell_at(grid.write, grid.nx, x, y) = g_empty_cell;
    }
  }
  return grid;
}

void cleanup_grid(Grid *grid) {
  free(grid->read);
  free(grid->write);
}

void draw_grid(Color *pixels, Grid *grid) {
  for (i32 y = 0; y < grid->ny; y++) {
    for (i32 x = 0; x < grid->nx; x++) {
      Cell cell = *(grid->read + (grid->nx * y + x));
      *(pixels + (grid->nx * y + x)) = cell.state == CELL_STATE_EMPTY
                                           ? g_color_empty
                                           : g_colors[cell.color_idx];
    }
  }
}

void act_on_grid(Grid *grid, i32 radius, u32 current_color) {
  i32 center_x = GetMouseX() / grid->w;
  i32 center_y = GetMouseY() / grid->h;

  if (center_x < 0 || center_x >= grid->nx || center_y < 0 ||
      center_y >= grid->ny) {
    return;
  }

  Cell cell = {.state = CELL_STATE_EMPTY, .color_idx = current_color};
  bool set = false;
  if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    cell.state = CELL_STATE_FILLED;
    set = true;
  }
  if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
    cell.state = CELL_STATE_EMPTY;
    set = true;
  }

  if (!set) {
    return;
  }

  for (i32 y = -radius; y < radius + 1; y++) {
    for (i32 x = -radius; x < radius + 1; x++) {
      if (x * x + y * y <= radius * radius) {
        i32 px = center_x + x;
        i32 py = center_y + y;
        if (px >= 0 && px < grid->nx && py >= 0 && py < grid->ny) {
          // *_cell_at(grid->read, grid->nx, center_x + x, center_y + y) = cell;
          *_cell_at(grid->write, grid->nx, center_x + x, center_y + y) = cell;
        }
      }
    }
  }
}

bool _left_update_grid(Grid *grid, i32 x, i32 y, u32 color_idx) {
  if (x - 1 >= 0 && get_cell(grid, x - 1, y + 1).state == CELL_STATE_EMPTY) {
    set_cell(grid, x, y, g_empty_cell);
    set_cell(grid, x - 1, y + 1,
             (Cell){.color_idx = color_idx, .state = CELL_STATE_FILLED});
    return true;
  }
  return false;
}

bool _right_update_grid(Grid *grid, i32 x, i32 y, u32 color_idx) {
  if (x + 1 < grid->nx &&
      get_cell(grid, x + 1, y + 1).state == CELL_STATE_EMPTY) {
    set_cell(grid, x, y, g_empty_cell);
    set_cell(grid, x + 1, y + 1,
             (Cell){.color_idx = color_idx, .state = CELL_STATE_FILLED});
    return true;
  }
  return false;
}

void update_grid(Grid *grid) {
  Cell cell;
  for (i32 y = grid->ny - 2; y >= 0; y--) {
    for (i32 x = 0; x < grid->nx; x++) {
      cell = get_cell(grid, x, y);
      if (cell.state == CELL_STATE_EMPTY) {
        continue;
      }

      if (get_cell(grid, x, y + 1).state == CELL_STATE_EMPTY) {
        set_cell(grid, x, y, g_empty_cell);
        set_cell(grid, x, y + 1, cell);
        continue;
      }

      if (rand() < RAND_MAX / 2) {
        if (_left_update_grid(grid, x, y, cell.color_idx)) {
          continue;
        }
        _right_update_grid(grid, x, y, cell.color_idx);
      } else {
        if (_right_update_grid(grid, x, y, cell.color_idx)) {
          continue;
        }
        _left_update_grid(grid, x, y, cell.color_idx);
      }
    }
  }
}

i32 main(void) {
  const i32 window_height = 800, window_width = 1200;
  InitWindow(window_width, window_height, "fallingsand");
  SetExitKey(KEY_ESCAPE);
  // i32 refresh_rate = GetMonitorRefreshRate(GetCurrentMonitor());
  i32 cell_h = 2, cell_w = 2;
  #if defined(PLATFORM_WEB)
    SetTargetFPS(60);
    cell_h = 4;
    cell_w = 4;
  #else
    SetTargetFPS(240);
  #endif

  i32 nx = window_width / cell_w;
  i32 ny = window_height / cell_h;
  Grid grid = new_grid(nx, ny, cell_w, cell_h);
  i32 action_radius = 4;
  u32 color_idx = 0;
  TraceLog(LOG_INFO, "[init] nx: %d | ny: %d | w: %d | h: %d | ar: %d", nx, ny,
           cell_w, cell_h, action_radius);

  Cell *temp;
  Color *pixels = malloc((u32)nx * (u32)ny * sizeof(Color));
  Image img = {.data = pixels,
               .width = nx,
               .height = ny,
               .mipmaps = 1,
               .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  Texture2D texture = LoadTextureFromImage(img);

  const Rectangle source_rect = {
      .x = 0.0f, .y = 0.0f, .width = (f32)nx, .height = (f32)ny};
  const Rectangle dest_rect = {.x = 0.0f,
                               .y = 0.0f,
                               .width = (f32)window_width,
                               .height = (f32)window_height};
  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_SPACE)) {
      color_idx = (color_idx + 1) % NUM_COLORS;
    }

    memcpy(grid.write, grid.read, sizeof(Cell) * (u32)nx * (u32)ny);

    act_on_grid(&grid, action_radius, color_idx);
    update_grid(&grid);

    temp = grid.read;
    grid.read = grid.write;
    grid.write = temp;
    draw_grid(pixels, &grid);
    UpdateTexture(texture, pixels);

    BeginDrawing();
    {
      ClearBackground(BLACK);
      DrawTexturePro(texture, source_rect, dest_rect, (Vector2){0, 0}, 0,
                     WHITE);
      DrawFPS(10, 10);
    }
    EndDrawing();
  }

  cleanup_grid(&grid);
  UnloadTexture(texture);
  UnloadImage(img);
  return 0;
}
