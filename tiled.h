
#ifndef TILED_H
#define TILED_H

struct tiled_window {
  GtkWidget *window;
  GtkWidget *vbox1, *vbox2, *hbox1;
  GtkWidget *menubar;
  GtkWidget *draw_area;
  GtkWidget *file_selection_save, *file_selection_open;

  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;

  struct td_surf surf;

  int tiles_dx, tiles_dy;

  int view_status;

  int real_time;
};

extern struct tiled_window tilwin;

/* functions */
void tiled_struct_init(void);
int tiled_window_init(void);
int tiled_window_refresh(void);
int tiled_window_import_data(void);

void tiled_resize_data(int dx, int dy);
void tiled_resize_view(int zoom);

#endif
