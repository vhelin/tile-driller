
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "defines.h"
#include "memory.h"
#include "editor.h"
#include "tiled.h"
#include "palette.h"
#include "common.h"


char prefs_file_name[] = "tiledriller.conf";

/* startup attributes */
int memory_window_dx = 256, memory_window_dy = 256;
int memory_window_x = 40, memory_window_y = 40;
int editor_window_x = 70, editor_window_y = 70;
int tiled_window_x = 100, tiled_window_y = 100;
int palette_window_x = 140, palette_window_y = 140;

/* prefs status */
int _prefs_saved = NO;


static void _prefs_get_file_name(char *c) {

#ifndef WIN32
  char *e;


  e = getenv("HOME");
  if (e == NULL)
    strcpy(c, prefs_file_name);
  else {
    strcpy(c, e);
    strcat(c, "/.");
    strcat(c, prefs_file_name);
  }
#else
  strcpy(c, prefs_file_name);
#endif
}


int prefs_read(void) {

  char tmp[256];
  FILE *f;
  int i, a, b;


  /* init the file paths */
  editwin.file_path[0] = 0;
  memwin.file_path[0] = 0;

  _prefs_get_file_name(tmp);

  f = fopen(tmp, "rb");
  if (f == NULL)
    return FAILED;

  while (fscanf(f, "%255s", tmp) != EOF) {
    if (strcmp("Version", tmp) == 0) {
      fscanf(f, "%d", &i);
      if (i != 1) {
        fprintf(stderr, "PREFS_LOAD: Configuration file version %d is not supported.\n", i);
        fclose(f);
        return FAILED;
      }
    }
    else if (strcmp("TiledViewWindow", tmp) == 0) {
      fscanf(f, "%d", &tilwin.view_status);
    }
    else if (strcmp("PaletteWindow", tmp) == 0) {
      fscanf(f, "%d", &palwin.status);
    }
    else if (strcmp("MemorySnapToGrid", tmp) == 0) {
      fscanf(f, "%d", &memwin.snap_to_grid);
    }
    else if (strcmp("MemoryShowGrid", tmp) == 0) {
      fscanf(f, "%d", &memwin.show_grid);
    }
    else if (strcmp("EditorShowGrid", tmp) == 0) {
      fscanf(f, "%d", &editwin.show_grid);
    }
    else if (strcmp("MemoryUpdatedInRealTime", tmp) == 0) {
      fscanf(f, "%d", &memwin.real_time);
    }
    else if (strcmp("TiledViewUpdatedInRealTime", tmp) == 0) {
      fscanf(f, "%d", &tilwin.real_time);
    }
    else if (strcmp("TiledViewZoom", tmp) == 0) {
      fscanf(f, "%d", &tilwin.surf.zoom);
      if (tilwin.surf.zoom > MAX_ZOOM_TILED) tilwin.surf.zoom = MAX_ZOOM_TILED;
      else if (tilwin.surf.zoom < 1) tilwin.surf.zoom = 1;
    }
    else if (strcmp("MemoryZoom", tmp) == 0) {
      fscanf(f, "%d", &memwin.surf.zoom);
      if (memwin.surf.zoom > MAX_ZOOM_MEMORY) memwin.surf.zoom = MAX_ZOOM_MEMORY;
      else if (memwin.surf.zoom < 1) memwin.surf.zoom = 1;
    }
    else if (strcmp("EditorZoom", tmp) == 0) {
      fscanf(f, "%d", &editwin.surf.zoom);
      if (editwin.surf.zoom > MAX_ZOOM_EDITOR) editwin.surf.zoom = MAX_ZOOM_EDITOR;
      else if (editwin.surf.zoom < 1) editwin.surf.zoom = 1;
    }
    else if (strcmp("EditorUndoStackMaxDepth", tmp) == 0) {
      fscanf(f, "%d", &editwin.undo_stack_max_depth);
      if (editwin.undo_stack_max_depth < 1) editwin.undo_stack_max_depth = 1;
    }
    else if (strcmp("MemoryUndoStackMaxDepth", tmp) == 0) {
      fscanf(f, "%d", &memwin.undo_stack_max_depth);
      if (memwin.undo_stack_max_depth < 1) memwin.undo_stack_max_depth = 1;
    }
    else if (strcmp("EditorToolMode", tmp) == 0) {
      fscanf(f, "%d", &editwin.tool_mode);
    }
    else if (strcmp("TiledViewSize", tmp) == 0) {
      fscanf(f, "%d %d", &tilwin.tiles_dx, &tilwin.tiles_dy);
    }
    else if (strcmp("MemorySize", tmp) == 0) {
      fscanf(f, "%d %d", &memwin.surf.width, &memwin.surf.height);
    }
    else if (strcmp("MemoryGridSize", tmp) == 0) {
      fscanf(f, "%d %d", &memwin.grid_x, &memwin.grid_y);
    }
    else if (strcmp("EditorSize", tmp) == 0) {
      fscanf(f, "%d %d", &editwin.surf.width, &editwin.surf.height);
    }
    else if (strcmp("MemoryWindowSize", tmp) == 0) {
      fscanf(f, "%d %d", &a, &b);
      if (a >= 0 && b >= 0) {
        memory_window_dx = a;
        memory_window_dy = b;
      }
    }
    else if (strcmp("TiledViewViewMode", tmp) == 0) {
      fscanf(f, "%d", &tilwin.surf.viewmode);
    }
    else if (strcmp("MemoryViewMode", tmp) == 0) {
      fscanf(f, "%d", &memwin.surf.viewmode);
    }
    else if (strcmp("EditorViewMode", tmp) == 0) {
      fscanf(f, "%d", &editwin.surf.viewmode);
    }
    else if (strcmp("MemoryWindowOrigin", tmp) == 0) {
      fscanf(f, "%d %d", &a, &b);
      if (a >= 0 && b >= 0) {
        memory_window_x = a;
        memory_window_y = b;
      }
    }
    else if (strcmp("EditorWindowOrigin", tmp) == 0) {
      fscanf(f, "%d %d", &a, &b);
      if (a >= 0 && b >= 0) {
        editor_window_x = a;
        editor_window_y = b;
      }
    }
    else if (strcmp("TiledViewWindowOrigin", tmp) == 0) {
      fscanf(f, "%d %d", &a, &b);
      if (a >= 0 && b >= 0) {
        tiled_window_x = a;
        tiled_window_y = b;
      }
    }
    else if (strcmp("PaletteWindowOrigin", tmp) == 0) {
      fscanf(f, "%d %d", &a, &b);
      if (a >= 0 && b >= 0) {
        palette_window_x = a;
        palette_window_y = b;
      }
    }
    else if (strcmp("PaletteWindowBGColor", tmp) == 0) {
      fscanf(f, "%d", &i);
      editwin.pen_bg.r = i;
      fscanf(f, "%d", &i);
      editwin.pen_bg.g = i;
      fscanf(f, "%d", &i);
      editwin.pen_bg.b = i;
      fscanf(f, "%d", &i);
      editwin.pen_bg.a = i;
    }
    else if (strcmp("PaletteWindowFGColor", tmp) == 0) {
      fscanf(f, "%d", &i);
      editwin.pen_fg.r = i;
      fscanf(f, "%d", &i);
      editwin.pen_fg.g = i;
      fscanf(f, "%d", &i);
      editwin.pen_fg.b = i;
      fscanf(f, "%d", &i);
      editwin.pen_fg.a = i;
    }
    else if (strcmp("EditorFilePath", tmp) == 0) {
      while (fscanf(f, "%c", tmp) != EOF) {
        if (tmp[0] == '"')
          break;
      }
      for (i = 0; i < 255; i++) {
        if (fscanf(f, "%c", tmp) == EOF)
          break;
        if (tmp[0] == '"')
          break;
        editwin.file_path[i] = tmp[0];
      }
      editwin.file_path[i] = 0;
    }
    else if (strcmp("MemoryFilePath", tmp) == 0) {
      while (fscanf(f, "%c", tmp) != EOF) {
        if (tmp[0] == '"')
          break;
      }
      for (i = 0; i < 255; i++) {
        if (fscanf(f, "%c", tmp) == EOF)
          break;
        if (tmp[0] == '"')
          break;
        memwin.file_path[i] = tmp[0];
      }
      memwin.file_path[i] = 0;
    }
  }

  fclose(f);

  return SUCCEEDED;
}


int prefs_save(void) {

  char tmp[256];
  FILE *f;


  if (_prefs_saved == YES)
    return FAILED;

  _prefs_get_file_name(tmp);

  f = fopen(tmp, "wb");
  if (f == NULL) {
    fprintf(stderr, "PREFS_SAVE: Could not open file \"%s\" for writing.\n", tmp);
    return FAILED;
  }

  gdk_window_get_size(memwin.window->window, &memory_window_dx, &memory_window_dy);
  gtk_window_get_position(GTK_WINDOW(memwin.window), &memory_window_x, &memory_window_y);
  gtk_window_get_position(GTK_WINDOW(editwin.window), &editor_window_x, &editor_window_y);
  if (tilwin.view_status == ON)
    gtk_window_get_position(GTK_WINDOW(tilwin.window), &tiled_window_x, &tiled_window_y);
  if (palwin.status == ON)
    gtk_window_get_position(GTK_WINDOW(palwin.window), &palette_window_x, &palette_window_y);

  fprintf(f, "\n;\n; Tile Driller Configuration File\n;\n\n");
  fprintf(f, "Version 1\n");
  fprintf(f, "\n");
  fprintf(f, "MemoryFilePath \"%s\"\n", memwin.file_path);
  fprintf(f, "MemorySize %d %d\n", memwin.surf.width, memwin.surf.height);
  fprintf(f, "MemorySnapToGrid %d\n", memwin.snap_to_grid);
  fprintf(f, "MemoryShowGrid %d\n", memwin.show_grid);
  fprintf(f, "MemoryGridSize %d %d\n", memwin.grid_x, memwin.grid_y);
  fprintf(f, "MemoryUpdatedInRealTime %d\n", memwin.real_time);
  fprintf(f, "MemoryViewMode %d\n", memwin.surf.viewmode);
  fprintf(f, "MemoryWindowOrigin %d %d\n", memory_window_x, memory_window_y);
  fprintf(f, "MemoryWindowSize %d %d\n", memory_window_dx, memory_window_dy);
  fprintf(f, "MemoryZoom %d\n", memwin.surf.zoom);
  fprintf(f, "MemoryUndoStackMaxDepth %d\n", memwin.undo_stack_max_depth);
  fprintf(f, "\n");
  fprintf(f, "EditorFilePath \"%s\"\n", editwin.file_path);
  fprintf(f, "EditorSize %d %d\n", editwin.surf.width, editwin.surf.height);
  fprintf(f, "EditorToolMode %d\n", editwin.tool_mode);
  fprintf(f, "EditorViewMode %d\n", editwin.surf.viewmode);
  fprintf(f, "EditorWindowOrigin %d %d\n", editor_window_x, editor_window_y);
  fprintf(f, "EditorZoom %d\n", editwin.surf.zoom);
  fprintf(f, "EditorShowGrid %d\n", editwin.show_grid);
  fprintf(f, "EditorUndoStackMaxDepth %d\n", editwin.undo_stack_max_depth);
  fprintf(f, "\n");
  fprintf(f, "TiledViewSize %d %d\n", tilwin.tiles_dx, tilwin.tiles_dy);
  fprintf(f, "TiledViewUpdatedInRealTime %d\n", tilwin.real_time);
  fprintf(f, "TiledViewViewMode %d\n", tilwin.surf.viewmode);
  fprintf(f, "TiledViewWindow %d\n", tilwin.view_status);
  fprintf(f, "TiledViewWindowOrigin %d %d\n", tiled_window_x, tiled_window_y);
  fprintf(f, "TiledViewZoom %d\n", tilwin.surf.zoom);
  fprintf(f, "\n");
  fprintf(f, "PaletteWindow %d\n", palwin.status);
  fprintf(f, "PaletteWindowBGColor %d %d %d %d\n", editwin.pen_bg.r, editwin.pen_bg.g, editwin.pen_bg.b, editwin.pen_bg.a);
  fprintf(f, "PaletteWindowFGColor %d %d %d %d\n", editwin.pen_fg.r, editwin.pen_fg.g, editwin.pen_fg.b, editwin.pen_fg.a);
  fprintf(f, "PaletteWindowOrigin %d %d\n", palette_window_x, palette_window_y);
  fprintf(f, "\n");

  fclose(f);

  /* save only once */
  _prefs_saved = YES;

  return SUCCEEDED;
}


int prefs_apply(void) {

  /* init the memory window data structures */
  common_resize_data(memwin.surf.width, memwin.surf.height, &editwin.pen_bg, &memwin.surf);
  memory_resize_view(memwin.surf.zoom);
  common_copy_data_to_view(&memwin.surf);

  /* init the data structures */
  editor_resize_data(editwin.surf.width, editwin.surf.height);
  editor_resize_view(editwin.surf.zoom);
  common_copy_data_to_child(&editwin.surf);

  common_copy_data_to_view(&editwin.surf);

  /* init the tiled view data structures */
  tiled_resize_data(tilwin.surf.width, tilwin.surf.height);
  tiled_resize_view(tilwin.surf.zoom);
  tiled_window_import_data();
  common_copy_data_to_view(&tilwin.surf);

  /* tiled updated-in-real-time */
  if (tilwin.real_time == YES) {
    tilwin.real_time = NO;
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(tilwin.item_factory, "/View/Updated in Real-Time"), TRUE);
  }

  /* memory updated-in-real-time */
  if (memwin.real_time == YES) {
    memwin.real_time = NO;
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/View/Updated in Real-Time"), TRUE);
  }

  /* memory snap-to-grid */
  if (memwin.snap_to_grid == YES) {
    memwin.snap_to_grid = NO;
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/View/Snap to Grid"), TRUE);
  }

  /* memory show grid */
  if (memwin.show_grid == YES) {
    memwin.show_grid = NO;
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/View/Show Grid"), TRUE);
  }

  /* editor show grid */
  if (editwin.show_grid == YES) {
    editwin.show_grid = NO;
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/View/Show Grid"), TRUE);
  }

  /* memory window attributes */
  gtk_window_move(GTK_WINDOW(memwin.window), memory_window_x, memory_window_y);
  gtk_window_resize(GTK_WINDOW(memwin.window), memory_window_dx, memory_window_dy);

  /* editor window attributes */
  gtk_window_move(GTK_WINDOW(editwin.window), editor_window_x, editor_window_y);

  /* palette window attributes */
  gtk_window_move(GTK_WINDOW(palwin.window), palette_window_x, palette_window_y);

  /* tiled view window attributes */
  gtk_window_move(GTK_WINDOW(tilwin.window), tiled_window_x, tiled_window_y);

  /* memory file paths */
  if (memwin.file_path[0] != 0) {
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(memwin.file_selection_save), memwin.file_path);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(memwin.file_selection_open), memwin.file_path);
  }

  /* editor file paths */
  if (editwin.file_path[0] != 0) {
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(editwin.file_selection_save), editwin.file_path);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(editwin.file_selection_open), editwin.file_path);
  }

  /* editor tool mode */
  if (editwin.tool_mode == TOOL_MODE_BLUR_GAUSSIAN)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Tools/Blur/Gaussian"), TRUE);
  else if (editwin.tool_mode == TOOL_MODE_BLUR_NORMAL)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Tools/Blur/Normal"), TRUE);
  else if (editwin.tool_mode == TOOL_MODE_FILL)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Tools/Fill"), TRUE);
  else if (editwin.tool_mode == TOOL_MODE_PICK)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Tools/Pick"), TRUE);
  else if (editwin.tool_mode == TOOL_MODE_PEN_1X1)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Tools/Pen 1x1"), TRUE);
  else if (editwin.tool_mode == TOOL_MODE_PEN_3X3)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Tools/Pen 3x3"), TRUE);
  else if (editwin.tool_mode == TOOL_MODE_PEN_5X5)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Tools/Pen 5x5"), TRUE);
  else if (editwin.tool_mode == TOOL_MODE_PEN_7X7)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Tools/Pen 7x7"), TRUE);

  /* editor size */
  if (editwin.surf.width == 8)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Size/8x8"), TRUE);
  else if (editwin.surf.width == 16)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Size/16x16"), TRUE);
  else if (editwin.surf.width == 32)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Size/32x32"), TRUE);
  else if (editwin.surf.width == 64)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Size/64x64"), TRUE);
  else if (editwin.surf.width == 128)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Size/128x128"), TRUE);
  else if (editwin.surf.width == 256)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Size/256x256"), TRUE);
  else if (editwin.surf.width == 512)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Size/512x512"), TRUE);
  else if (editwin.surf.width == 1024)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/Size/1024x1024"), TRUE);

  /* tiled view size */
  if (tilwin.tiles_dx == 3)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(tilwin.item_factory, "/Size/3x3"), TRUE);
  else if (tilwin.tiles_dx == 5)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(tilwin.item_factory, "/Size/5x5"), TRUE);
  else if (tilwin.tiles_dx == 7)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(tilwin.item_factory, "/Size/7x7"), TRUE);
  else if (tilwin.tiles_dx == 9)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(tilwin.item_factory, "/Size/9x9"), TRUE);

  /* memory size */
  if (memwin.surf.width == 64)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Size/64x64"), TRUE);
  else if (memwin.surf.width == 128)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Size/128x128"), TRUE);
  else if (memwin.surf.width == 256)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Size/256x256"), TRUE);
  else if (memwin.surf.width == 512)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Size/512x512"), TRUE);
  else if (memwin.surf.width == 1024)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Size/1024x1024"), TRUE);
  else if (memwin.surf.width == 2048)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Size/2048x2048"), TRUE);

  /* memory view mode */
  if (memwin.surf.viewmode == VIEW_MODE_RGBA)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/View/RGBA"), TRUE);
  else if (memwin.surf.viewmode == VIEW_MODE_RGB)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/View/RGB"), TRUE);
  else if (memwin.surf.viewmode == VIEW_MODE_ALPHA)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/View/Alpha"), TRUE);

  /* editor view mode */
  if (editwin.surf.viewmode == VIEW_MODE_RGBA)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/View/RGBA"), TRUE);
  else if (editwin.surf.viewmode == VIEW_MODE_RGB)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/View/RGB"), TRUE);
  else if (editwin.surf.viewmode == VIEW_MODE_ALPHA)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(editwin.item_factory, "/View/Alpha"), TRUE);

  /* tiled view view mode */
  if (tilwin.surf.viewmode == VIEW_MODE_RGBA)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(tilwin.item_factory, "/View/RGBA"), TRUE);
  else if (tilwin.surf.viewmode == VIEW_MODE_RGB)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(tilwin.item_factory, "/View/RGB"), TRUE);
  else if (tilwin.surf.viewmode == VIEW_MODE_ALPHA)
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(tilwin.item_factory, "/View/Alpha"), TRUE);

  /* display the windows */
  gtk_widget_show_all(memwin.window);
  gtk_widget_show_all(editwin.window);

  /* palette window */
  if (palwin.status == ON) {
    palwin.status = OFF;
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Windows/Palette"), TRUE);
  }

  /* tiled view window */
  if (tilwin.view_status == ON) {
    tilwin.view_status = OFF;
    gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Windows/Tiled View"), TRUE);
  }

  /* update the color selector */
  palette_window_refresh();

  return SUCCEEDED;
}
