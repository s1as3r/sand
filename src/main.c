// clang-format off
#include <stdlib.h>
#include <string.h>

#include <raylib.h>

#include "base.h"
// clang-format on

#define NUM_COLORS 5
#define MAX_BRUSH_RADIUS 20

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
  Color text_color;
  Font font;
  f32 font_size;
  f32 font_spacing;
  f32 padding;
} UiConfig;

void draw_text(UiConfig *ui, const char *text, Vector2 pos) {
  DrawTextEx(ui->font, text, pos, ui->font_size, ui->font_spacing,
             ui->text_color);
}

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

inline Cell *cell_at(Cell *cells, i32 nx, i32 x, i32 y) {
  return (cells + (y * nx) + x);
}

Grid new_grid(i32 nx, i32 ny, i32 w, i32 h) {
  Grid grid = {.nx = nx, .ny = ny, .h = h, .w = w};
  u32 n_bytes = sizeof(Cell) * (u32)nx * (u32)ny;
  grid.read = malloc(n_bytes);
  grid.write = malloc(n_bytes);

  for (i32 y = 0; y < ny; y++) {
    for (i32 x = 0; x < nx; x++) {
      *cell_at(grid.read, grid.nx, x, y) = g_empty_cell;
      *cell_at(grid.write, grid.nx, x, y) = g_empty_cell;
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
          *cell_at(grid->write, grid->nx, center_x + x, center_y + y) = cell;
        }
      }
    }
  }
}

bool _left_update_grid(Grid *grid, i32 x, i32 y, u32 color_idx) {
  if (x - 1 >= 0 &&
      cell_at(grid->read, grid->nx, x - 1, y + 1)->state == CELL_STATE_EMPTY) {
    *cell_at(grid->write, grid->nx, x, y) = g_empty_cell;
    *cell_at(grid->write, grid->nx, x - 1, y + 1) =
        (Cell){.color_idx = color_idx, .state = CELL_STATE_FILLED};
    return true;
  }
  return false;
}

bool _right_update_grid(Grid *grid, i32 x, i32 y, u32 color_idx) {
  if (x + 1 < grid->nx &&
      cell_at(grid->read, grid->nx, x + 1, y + 1)->state == CELL_STATE_EMPTY) {
    *cell_at(grid->write, grid->nx, x, y) = g_empty_cell;
    *cell_at(grid->write, grid->nx, x + 1, y + 1) =
        (Cell){.color_idx = color_idx, .state = CELL_STATE_FILLED};
    return true;
  }
  return false;
}

void update_grid(Grid *grid) {
  Cell cell;
  for (i32 y = grid->ny - 2; y >= 0; y--) {
    for (i32 x = 0; x < grid->nx; x++) {
      cell = *cell_at(grid->read, grid->nx, x, y);
      if (cell.state == CELL_STATE_EMPTY) {
        continue;
      }

      // insted of checking the cell in the read buffer, we check it in the
      // write buffer since it will already be processsed. we basically want to
      // avoid the issues with cells right on top of each other. if we check the
      // read buffer, it would try to move the cell diagonally which we dont
      // want.
      if (cell_at(grid->write, grid->nx, x, y + 1)->state == CELL_STATE_EMPTY) {
        *cell_at(grid->write, grid->nx, x, y) = g_empty_cell;
        *cell_at(grid->write, grid->nx, x, y + 1) = cell;
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

  UiConfig ui = {
      .font = LoadFont("assets/Fake Receipt.otf"),
      .font_size = 20,
      .font_spacing = 1,
      .text_color = WHITE,
      .padding = 10,
  };

  SetExitKey(KEY_ESCAPE);
  i32 cell_h = 2, cell_w = 2;
  i32 brush_radius = 6;
  i32 target_fps = 240;
  i32 brush_show_frames = target_fps / 2;

  // clang-format off
  #if defined(PLATFORM_WEB)
    rr = 60
    cell_h = 4;
    cell_w = 4;
    brush_radius = 3;
  #endif
  // clang-format on
  SetTargetFPS(target_fps);

  i32 nx = window_width / cell_w;
  i32 ny = window_height / cell_h;
  Grid grid = new_grid(nx, ny, cell_w, cell_h);
  u32 color_idx = 0;
  TraceLog(LOG_INFO, "[init] nx: %d | ny: %d | w: %d | h: %d | ar: %d", nx, ny,
           cell_w, cell_h, brush_radius);

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

  const char *ins_brush = "<- | ->:   inc/dec brush radius";
  const char *ins_pause = "space  : (un)pause grid updates";
  const char *ins_color = "c      :     change brush color";
  const char *ins_erase = "mouse2 :                  erase";

  const Vector2 ins_brush_w =
      MeasureTextEx(ui.font, ins_brush, ui.font_size, ui.font_spacing);
  const Vector2 ins_pause_w =
      MeasureTextEx(ui.font, ins_pause, ui.font_size, ui.font_spacing);
  const Vector2 ins_color_w =
      MeasureTextEx(ui.font, ins_color, ui.font_size, ui.font_spacing);
  const Vector2 ins_erase_w =
      MeasureTextEx(ui.font, ins_erase, ui.font_size, ui.font_spacing);

  bool pause_update = false;
  i32 show_brush_for = brush_show_frames;
  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_C)) {
      color_idx = (color_idx + 1) % NUM_COLORS;
      show_brush_for = brush_show_frames;
    }

    if (IsKeyPressed(KEY_SPACE)) {
      pause_update = !pause_update;
    }

    if (IsKeyPressed(KEY_RIGHT) && brush_radius < MAX_BRUSH_RADIUS) {
      brush_radius++;
      show_brush_for = brush_show_frames;
    }
    if (IsKeyPressed(KEY_LEFT) && brush_radius > 0) {
      brush_radius--;
      show_brush_for = brush_show_frames;
    }

    memcpy(grid.write, grid.read, sizeof(Cell) * (u32)nx * (u32)ny);

    act_on_grid(&grid, brush_radius, color_idx);
    if (!pause_update) {
      update_grid(&grid);
    }

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
      if (show_brush_for > 0) {
        DrawCircle(GetMouseX(), GetMouseY(), (f32)(brush_radius * cell_w),
                   g_colors[color_idx]);
        show_brush_for--;
      }
      draw_text(&ui,
                TextFormat("brush radius: %d %s", brush_radius,
                           pause_update ? "| updates paused" : ""),
                (Vector2){ui.padding, ui.padding});

      draw_text(&ui, ins_brush,
                (Vector2){window_width - ins_brush_w.x - ui.padding,
                          ui.padding + 0 * ins_brush_w.y});
      draw_text(&ui, ins_color,
                (Vector2){window_width - ins_color_w.x - ui.padding,
                          ui.padding + 1 * ins_brush_w.y});
      draw_text(&ui, ins_pause,
                (Vector2){window_width - ins_pause_w.x - ui.padding,
                          ui.padding + 2 * ins_color_w.y});
      draw_text(&ui, ins_erase,
                (Vector2){window_width - ins_erase_w.x - ui.padding,
                          ui.padding + 3 * ins_pause_w.y});
    }
    EndDrawing();
  }

  cleanup_grid(&grid);
  UnloadTexture(texture);
  UnloadImage(img);
  return 0;
}
