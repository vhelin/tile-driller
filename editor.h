
#ifndef EDITOR_H
#define EDITOR_H

#include "common.h"
#include "memory.h"

#define TOOL_MODE_BLUR_GAUSSIAN   0
#define TOOL_MODE_BLUR_NORMAL     1
#define TOOL_MODE_PICK            2
#define TOOL_MODE_FILL            3
#define TOOL_MODE_AGGRESSIVE_FILL 4
#define TOOL_MODE_GRADIENT_FILL   5
#define TOOL_MODE_PEN_1X1         6
#define TOOL_MODE_PEN_3X3         7
#define TOOL_MODE_PEN_5X5         8
#define TOOL_MODE_PEN_7X7         9

#define MK_SHIFT	  1
#define MK_ALT		  2
#define MK_CONTROL	4

struct edit_window {
  GtkItemFactory *item_factory;
  GtkWidget *window;
  GtkWidget *file_selection_save, *file_selection_open;

  GtkWidget *vbox1, *vbox2, *hbox1;
  GtkWidget *menubar;
  GtkWidget *draw_area;
  GtkWidget *statuslabel;

  GtkAccelGroup *accel_group;

  struct td_surf surf;

  int tool_mode;

  char file_name[256];
  char file_path[256];
  int  file_name_status;

  struct td_color pen_fg;
  struct td_color pen_bg;

  int need_to_save;

  int show_grid;

  struct _undo undo_data;
  int undo_stack_max_depth;

  int editor_copy_dx, editor_copy_dy;
  unsigned char *editor_copy_data;

  int mouse_button;
  int mouse_button_mode;

  int mouse_old_x, mouse_old_y;
  int editor_prev_x;
  int editor_prev_y;
  int editor_curr_x;
  int editor_curr_y;
  int editor_next_x;
  int editor_next_y;
  int editor_modkey;
  struct td_color *linecolor;
};

extern struct edit_window editwin;

/* functions */
void editor_struct_init(void);
void editor_file_open(gchar *n);
int editor_window_init(void);
int editor_window_refresh(void);
int editor_window_set_title(void);

void editor_resize_data(int dx, int dy);
void editor_resize_view(int zoom);

#endif
