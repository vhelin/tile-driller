
#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

struct mem_window {
  GtkWidget *window;
  GtkWidget *vbox1;
  GtkWidget *menubar;
  GtkWidget *draw_area;
  GtkWidget *file_selection_save, *file_selection_open;
  GtkWidget *scrolled_window;
  GtkWidget *widget_revert;

  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;

  struct td_surf surf;

  int show_grid;
  int grid_x;
  int grid_y;

  char file_name[256], file_path[256];
  int file_status, file_name_status;

  int mouse_button;
  int snap_to_grid;

  int real_time;

  struct _undo undo_data;
  int undo_stack_max_depth;
};

struct mem_window memwin;

/* functions */
void memory_struct_init(void);
int memory_window_init(void);
int memory_window_refresh(void);
int memory_window_block_refresh(void);
int memory_window_plain_block_refresh(void);
int memory_copy_data_out(unsigned char *out, int x, int y, int dx, int dy);
int memory_copy_data_in(unsigned char *in, int x, int y, int dx, int dy);
int memory_draw_editor_rectangle(void);
int memory_compute_tile_position(int mx, int my);

void memory_resize_data(int dx, int dy);
void memory_resize_view(int zoom);
void memory_window_set_title(void);
void memory_title_reset(int ents);

#endif
