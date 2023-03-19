
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkwidget.h>
#include <gdk/gdkkeysyms.h>

#include "defines.h"
#include "memory.h"
#include "editor.h"
#include "common.h"
#include "palette.h"
#include "tiled.h"
#include "jpg.h"
#include "png.h"
#include "bmp.h"
#include "pcx.h"
#include "tga.h"
#include "prefs.h"
#include "string.h"


struct mem_window memwin;

/* internal functions */
static void memory_clear(GtkWidget *widget, gpointer data);
static void memory_open(GtkWidget *widget, gpointer data);
static void memory_save(GtkWidget *widget, gpointer data);
static void memory_save_as(GtkWidget *widget, gpointer data);
static void memory_revert(GtkWidget *widget, gpointer data);
static void memory_file_save_ok(GtkWidget *widget, gpointer data);
static void memory_file_save_cancel(GtkWidget *widget, gpointer data);
static void memory_file_open_ok(GtkWidget *widget, gpointer data);
static void memory_file_open_cancel(GtkWidget *widget, gpointer data);
static void memory_size_n(GtkWidget *widget, gpointer data);
static void memory_zoom_n(GtkWidget *widget, gpointer data);
static void memory_zoom_change(GtkWidget *widget, gpointer data);
static void memory_view_n(GtkWidget *widget, gpointer data);
static void memory_scale_n(GtkWidget *widget, gpointer data);
static void memory_window_n(GtkWidget *widget, gpointer data);
static void memory_snap(GtkWidget *widget, gpointer data);
static void memory_update_mode(GtkWidget *widget, gpointer data);
static void memory_undo(GtkWidget *widget, gpointer data);
static gint memory_button_press(GtkWidget *widget, GdkEventButton *event);
static gint memory_button_release(GtkWidget *widget, GdkEventButton *event);
static gint memory_motion_notify(GtkWidget *widget, GdkEventButton *event);
static void memory_copy_to_undo(void);
static gint memory_key_press(GtkWidget *widget, GdkEventKey *event);
static gboolean memory_draw_area_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);
static void memory_change_showgrid(GtkWidget *widget, gpointer data);
static void memory_set_gridsize_n(GtkWidget *widget, gpointer data);

GtkItemFactoryEntry memory_menu_items[] = {
  { "/_File",           NULL,         NULL, 0, "<Branch>" },
  { "/File/New",        NULL,         memory_clear, 0, NULL },
  { "/File/sep1",       NULL,         NULL, 0, "<Separator>" },
  { "/File/_Open",      "<control>O", memory_open, 0, NULL },
  { "/File/_Save",      "<control>S", memory_save, 0, NULL },
  { "/File/Save As",    NULL,         memory_save_as, 0, NULL },
  { "/File/Revert",     NULL,         memory_revert, 0, NULL },
  { "/File/sep2",       NULL,         NULL, 0, "<Separator>" },
  { "/File/_Quit",      "<control>Q", common_gtk_exit, 0, NULL },
  { "/_Edit",           NULL,         NULL, 0, "<Branch>" },
  { "/Edit/Undo",       "<control>Z", memory_undo, 0, NULL },
  { "/Edit/sep3",        NULL,        NULL, 0, "<Separator>" },
  { "/Edit/Scale",      NULL,         NULL, 0, "<Branch>" },
  { "/Edit/Scale/50%",  NULL,         memory_scale_n, 2, NULL },
  { "/Edit/Scale/25%",  NULL,         memory_scale_n, 4, NULL },
  { "/Edit/Scale/12.5%",NULL,         memory_scale_n, 8, NULL },
  { "/_Zoom",           NULL,         NULL, 0, "<Branch>" },
  { "/Zoom/Decrease",  "minus",      memory_zoom_change, -1, NULL },
  { "/Zoom/100%",       "1",          memory_zoom_n, 1, NULL },
  { "/Zoom/200%",       NULL,         memory_zoom_n, 2, NULL },
  { "/Zoom/300%",       NULL,         memory_zoom_n, 3, NULL },
  { "/Zoom/400%",       NULL,         memory_zoom_n, 4, NULL },
  { "/Zoom/Increase",  "plus",       memory_zoom_change, 1, NULL },
  { "/_Size",           NULL,         NULL, 0, "<Branch>" },
  { "/Size/64x64",      NULL,         memory_size_n, 64, "<RadioItem>" },
  { "/Size/128x128",    NULL,         memory_size_n, 128, "/Size/64x64" },
  { "/Size/256x256",    NULL,         memory_size_n, 256, "/Size/64x64" },
  { "/Size/512x512",    NULL,         memory_size_n, 512, "/Size/64x64" },
  { "/Size/1024x1024",  NULL,         memory_size_n, 1024, "/Size/64x64" },
  { "/Size/2048x2048",  NULL,         memory_size_n, 2048, "/Size/64x64" },
  { "/_View",           NULL,         NULL, 0, "<Branch>" },
  { "/View/RGBA",       "<alt>6",     memory_view_n, VIEW_MODE_RGBA, "<RadioItem>" },
  { "/View/RGB",        "<alt>7",     memory_view_n, VIEW_MODE_RGB, "/View/RGBA" },
  { "/View/Alpha",      "<alt>8",     memory_view_n, VIEW_MODE_ALPHA, "/View/RGBA" },
  { "/View/sep4",       NULL,         NULL, 0, "<Separator>" },
  { "/View/Snap to Grid", NULL,       memory_snap, 0, "<ToggleItem>" },
  { "/View/Show Grid",  NULL,         memory_change_showgrid, 0, "<ToggleItem>" },
  { "/View/Grid Size",  NULL,         NULL, 0, "<Branch>" },
  { "/View/Grid Size/8x8",      NULL,         memory_set_gridsize_n, 8, "<RadioItem>" },
  { "/View/Grid Size/16x16",    NULL,         memory_set_gridsize_n, 16, "/View/Grid Size/8x8" },
  { "/View/Grid Size/32x32",    NULL,         memory_set_gridsize_n, 32, "/View/Grid Size/8x8" },
  { "/View/Grid Size/64x64",    NULL,         memory_set_gridsize_n, 64, "/View/Grid Size/8x8" },
  { "/View/Grid Size/128x128",  NULL,         memory_set_gridsize_n, 128, "/View/Grid Size/8x8" },

  { "/View/sep5",       NULL,         NULL, 0, "<Separator>" },
  { "/View/Updated in Real-Time",     NULL, memory_update_mode, 0, "<ToggleItem>" },
  { "/_Windows",        NULL,         NULL, 0, "<Branch>" },
  { "/Windows/Tiled View", NULL,      memory_window_n, 0, "<ToggleItem>" },
  { "/Windows/Palette", NULL,         memory_window_n, 1, "<ToggleItem>" },
};


void memory_struct_init(void) {
  
  memwin.window = NULL;
  memwin.vbox1 = NULL;
  memwin.menubar = NULL;
  memwin.draw_area = NULL;
  memwin.file_selection_save = NULL;
  memwin.file_selection_open = NULL;
  memwin.scrolled_window = NULL;
  memwin.widget_revert = NULL;
  memwin.accel_group = NULL;
  memwin.item_factory = NULL;

  memwin.surf.width = 128;
  memwin.surf.height = 128;
  memwin.surf.zoom = 2;
  memwin.surf.viewmode = VIEW_MODE_RGBA;
  memwin.surf.tilenum = 0;
  memwin.surf.data = NULL;
  memwin.surf.view = NULL;
  memwin.surf.parent = NULL;
  memwin.surf.parent_xofs = 0;
  memwin.surf.parent_yofs = 0;
  memwin.surf.child = NULL;

  memwin.show_grid = 1;
  memwin.grid_x = 16;
  memwin.grid_y = 16;
  memset(memwin.file_name, 0, 255);
  memset(memwin.file_path, 0, 255);
  memwin.file_status = NO;
  memwin.file_name_status = NO;
  memwin.mouse_button = OFF;
  memwin.snap_to_grid = NO;
  memwin.real_time = NO;
  memwin.undo_stack_max_depth = 10;
}


int memory_window_init(void) {

  /* window */
  memwin.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  memory_window_set_title();

  /* menu */
  memwin.accel_group = gtk_accel_group_new();
  memwin.item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", memwin.accel_group);
  gtk_item_factory_create_items(memwin.item_factory, sizeof(memory_menu_items)/sizeof(memory_menu_items[0]), memory_menu_items, NULL);
  gtk_window_add_accel_group(GTK_WINDOW(memwin.window), memwin.accel_group);
  memwin.menubar = gtk_item_factory_get_widget(memwin.item_factory, "<main>");

  /* drawing area */
  memwin.draw_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(memwin.draw_area), memwin.surf.width*memwin.surf.zoom, memwin.surf.height*memwin.surf.zoom);
  gtk_widget_add_events(memwin.draw_area,
                        GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_POINTER_MOTION_MASK);

  /* scrolled window */
  memwin.scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(memwin.scrolled_window), 4);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(memwin.scrolled_window),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  /* boxes */
  memwin.vbox1 = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(memwin.vbox1), 1);

  /* place boxes and widgets */
  gtk_container_add(GTK_CONTAINER(memwin.window), memwin.vbox1);
  gtk_box_pack_start(GTK_BOX(memwin.vbox1), memwin.menubar, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(memwin.vbox1), memwin.scrolled_window);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(memwin.scrolled_window), memwin.draw_area);

  /* file selections */
  memwin.file_selection_save = gtk_file_selection_new("Save PNG Image");
  memwin.file_selection_open = gtk_file_selection_new("Open BMP/JPG/PCX/PNG/TGA Image");

  /* signals */
  gtk_signal_connect(GTK_OBJECT(memwin.draw_area), "expose_event", GTK_SIGNAL_FUNC(memory_draw_area_expose), NULL);
  gtk_signal_connect(GTK_OBJECT(memwin.draw_area), "button_press_event", GTK_SIGNAL_FUNC(memory_button_press), NULL);
  gtk_signal_connect(GTK_OBJECT(memwin.draw_area), "button_release_event", GTK_SIGNAL_FUNC(memory_button_release), NULL);
  gtk_signal_connect(GTK_OBJECT(memwin.draw_area), "motion_notify_event", GTK_SIGNAL_FUNC(memory_motion_notify), NULL);
  gtk_signal_connect(GTK_OBJECT(memwin.window), "key_press_event", GTK_SIGNAL_FUNC(memory_key_press), NULL);
  gtk_signal_connect(GTK_OBJECT(memwin.window), "delete_event", GTK_SIGNAL_FUNC(common_gtk_exit), NULL);
  gtk_signal_connect(GTK_OBJECT(memwin.window), "destroy", GTK_SIGNAL_FUNC(common_gtk_exit), NULL);

  gtk_signal_connect(GTK_OBJECT(memwin.file_selection_save), "delete_event", GTK_SIGNAL_FUNC(common_gtk_hide_widget), NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(memwin.file_selection_save)->ok_button), "clicked", GTK_SIGNAL_FUNC(memory_file_save_ok), NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(memwin.file_selection_save)->cancel_button), "clicked", GTK_SIGNAL_FUNC(memory_file_save_cancel), NULL);

  gtk_signal_connect(GTK_OBJECT(memwin.file_selection_open), "delete_event", GTK_SIGNAL_FUNC(common_gtk_hide_widget), NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(memwin.file_selection_open)->ok_button), "clicked", GTK_SIGNAL_FUNC(memory_file_open_ok), NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(memwin.file_selection_open)->cancel_button), "clicked", GTK_SIGNAL_FUNC(memory_file_open_cancel), NULL);

  gtk_window_set_resizable(GTK_WINDOW(memwin.window), TRUE);
  gtk_widget_set_size_request(memwin.window, 256, 256);

  memwin.file_name[0] = 0;

  undo_init(memwin.undo_stack_max_depth, &memwin.undo_data);
  memwin.surf.parent = NULL;
  memwin.surf.child = &editwin.surf;

  /* get some widgets */
  memwin.widget_revert = gtk_item_factory_get_widget(memwin.item_factory, "/File/Revert");

  return SUCCEEDED;
}


static gint memory_key_press(GtkWidget *widget, GdkEventKey *event) {

  if (event->keyval == GDK_Right) {
    /* move one tile right */
    if (memwin.surf.child->parent_xofs < memwin.surf.width - memwin.surf.child->width) {
      memwin.surf.child->parent_xofs += memwin.surf.child->width;
      if (common_compute_tile_position(&memwin.surf) == SUCCEEDED) {
        editor_window_set_title();
        common_copy_data_to_child(&memwin.surf);
        editor_window_refresh();
        memory_window_refresh();
        tiled_window_refresh();
      }
    }
  }
  else if (event->keyval == GDK_Left) {
    /* move one tile left */
    if (memwin.surf.child->parent_xofs >= memwin.surf.child->width) {
      memwin.surf.child->parent_xofs -= memwin.surf.child->width;
      if (common_compute_tile_position(&memwin.surf) == SUCCEEDED) {
        editor_window_set_title();
        common_copy_data_to_child(&memwin.surf);
        editor_window_refresh();
        memory_window_refresh();
        tiled_window_refresh();
      }
    }
  }
  else if (event->keyval == GDK_Up) {
    /* move one tile up */
    if (memwin.surf.child->parent_yofs >= memwin.surf.child->height) {
      memwin.surf.child->parent_yofs -= memwin.surf.child->height;
      if (common_compute_tile_position(&memwin.surf) == SUCCEEDED) {
        editor_window_set_title();
        common_copy_data_to_child(&memwin.surf);
        editor_window_refresh();
        memory_window_refresh();
        tiled_window_refresh();
      }
    }
  }
  else if (event->keyval == GDK_Down) {
    /* move one tile down */
    if (memwin.surf.child->parent_yofs < memwin.surf.height - memwin.surf.child->height) {
      memwin.surf.child->parent_yofs += memwin.surf.child->height;
      if (common_compute_tile_position(&memwin.surf) == SUCCEEDED) {
        editor_window_set_title();
        common_copy_data_to_child(&memwin.surf);
        editor_window_refresh();
        memory_window_refresh();
        tiled_window_refresh();
      }
    }
  }

  return FALSE;
}

int memory_draw_editor_rectangle(void) {

  int x, y;

  common_putpixel_setmode(PUTPIX_NEGATE|PUTPIX_ZOOM|PUTPIX_VIEW);

  /* top */
  for (x = 0; x < memwin.surf.child->width; x++)
    common_putpixel(memwin.surf.child->parent_xofs + x, memwin.surf.child->parent_yofs, NULL, &memwin.surf);

  /* bottom */
  for (x = 0; x < memwin.surf.child->width; x++)
    common_putpixel(memwin.surf.child->parent_xofs + x, memwin.surf.child->parent_yofs + memwin.surf.child->height-1, NULL, &memwin.surf);

  /* left */
  for (y = 1; y < memwin.surf.child->height-1; y++)
    common_putpixel(memwin.surf.child->parent_xofs, memwin.surf.child->parent_yofs + y, NULL, &memwin.surf);

  /* right */
  for (y = 1; y < memwin.surf.child->height-1; y++)
    common_putpixel(memwin.surf.child->parent_xofs + memwin.surf.child->width-1, memwin.surf.child->parent_yofs + y, NULL, &memwin.surf);

  return SUCCEEDED;
}


void memory_title_reset(int ents) {

  if (editwin.need_to_save == ents)
    return;

  editwin.need_to_save = ents;
  memory_window_set_title();
}

static void memory_change_showgrid(GtkWidget *widget, gpointer data) {

  if (memwin.show_grid) memwin.show_grid = 0;
  else memwin.show_grid = 1;

  editor_window_refresh();
  memory_window_refresh();
}


void memory_window_set_title(void) {

  char tmp[512];
  int i;


  if (memwin.window == NULL)
    return;

  if (memwin.file_status == YES) {
    for (i = strlen(memwin.file_name)-1; i >= 0; i--) {
#ifdef WIN32
      if (memwin.file_name[i] == '\\')
#else
        if (memwin.file_name[i] == '/')
#endif
          break;
    }

    if (editwin.need_to_save == NO) {
      sprintf(tmp, "%s | %s | %dx%d | ", version_string, &memwin.file_name[i+1], memwin.surf.width, memwin.surf.height);
      if (memwin.widget_revert != NULL)
        gtk_widget_set_sensitive(memwin.widget_revert, FALSE);
    }
    else {
      sprintf(tmp, "%s | *%s | %dx%d | ", version_string, &memwin.file_name[i+1], memwin.surf.width, memwin.surf.height);
      if (memwin.widget_revert != NULL)
        gtk_widget_set_sensitive(memwin.widget_revert, TRUE);
    }
  }
  else {
    sprintf(tmp, "%s | ? | %dx%d | ", version_string, memwin.surf.width, memwin.surf.height);
    if (memwin.widget_revert != NULL)
      gtk_widget_set_sensitive(memwin.widget_revert, FALSE);
  }

  if (memwin.surf.viewmode == VIEW_MODE_RGBA)
    strcat(tmp, "RGBA");
  else if (memwin.surf.viewmode == VIEW_MODE_RGB)
    strcat(tmp, "RGB");
  else if (memwin.surf.viewmode == VIEW_MODE_ALPHA)
    strcat(tmp, "Alpha");

  gtk_window_set_title(GTK_WINDOW(memwin.window), tmp);
}


void memory_resize_view(int zoom) {

  memwin.surf.view = realloc(memwin.surf.view, memwin.surf.width*zoom*memwin.surf.height*zoom*3);
  if (memwin.surf.view == NULL) {
    fprintf(stderr, "MEMORY_RESIZE_VIEW: Out of memory error.\n");
    return;
  }

  memwin.surf.zoom = zoom;
}


void memory_draw_grid() {
  
  if (memwin.show_grid) {
    int x, y;

    for (x = memwin.grid_x; x < memwin.surf.width; x += memwin.grid_x)
      common_line_dotted_vert(x * memwin.surf.zoom, &memwin.surf);
    for (y = memwin.grid_y; y < memwin.surf.height; y += memwin.grid_y)
      common_line_dotted_horiz(y * memwin.surf.zoom, &memwin.surf);
  }
}


static void memory_clear(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  memory_copy_to_undo();

  memory_title_reset(YES);

  common_putpixel_setmode(PUTPIX_NORMAL);
  common_clear(&editwin.pen_bg, &memwin.surf);

  common_copy_data_to_view(&memwin.surf);
  memory_draw_editor_rectangle();

  /* update the window title */
  memwin.file_status = NO;
  memwin.file_name_status = NO;
  memory_window_set_title();

  /* refresh the memory draw area */
  gtk_widget_queue_draw(memwin.draw_area);

  /* refresh the windows */
  common_copy_data_to_child(&memwin.surf);

  editor_window_refresh();
  tiled_window_refresh();
}


static void memory_scale_n(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  memory_copy_to_undo();

  memory_title_reset(YES);

  /* scale! */
  common_scale((int)data, &memwin.surf);

  /* refresh the memory draw area */
  common_copy_data_to_view(&memwin.surf);
  memory_draw_editor_rectangle();
  gtk_widget_queue_draw(memwin.draw_area);

  /* refresh the windows */
  common_copy_data_to_child(&memwin.surf);

  editor_window_refresh();
  tiled_window_refresh();
}


static void memory_copy_to_undo(void) {

  undo_push(&memwin.surf, &memwin.undo_data);
}


static void memory_undo(GtkWidget *widget, gpointer data) {

  undo_pop(&memwin.surf, &memwin.undo_data);

  common_copy_data_to_view(&memwin.surf);
  memory_draw_editor_rectangle();
  gtk_widget_queue_draw(memwin.draw_area);

  common_copy_data_to_child(&memwin.surf);

  editor_window_refresh();
  tiled_window_refresh();

}


int memory_window_refresh(void) {

  if (memwin.surf.data == NULL || memwin.surf.view == NULL)
    return FAILED;

  common_copy_data_to_view(&memwin.surf);

  memory_draw_editor_rectangle();

  /* refresh the memory draw area */
  gtk_widget_queue_draw(memwin.draw_area);

  return SUCCEEDED;
}


int memory_window_block_refresh(void) {

  if (memwin.surf.data == NULL || memwin.surf.view == NULL)
    return FAILED;

  common_copy_block_to_view(memwin.surf.child->parent_xofs, memwin.surf.child->parent_yofs, memwin.surf.child->width, memwin.surf.child->height, &memwin.surf);

  memory_draw_editor_rectangle();

  memory_draw_grid();

  /* refresh the memory draw area */
  gdk_draw_rgb_image(memwin.draw_area->window, memwin.draw_area->style->fg_gc[GTK_STATE_NORMAL], memwin.surf.child->parent_xofs*memwin.surf.zoom, memwin.surf.child->parent_yofs*memwin.surf.zoom,
                     memwin.surf.child->width*memwin.surf.zoom, memwin.surf.child->height*memwin.surf.zoom, GDK_RGB_DITHER_MAX, memwin.surf.view + memwin.surf.child->parent_yofs*memwin.surf.zoom*memwin.surf.width*3*memwin.surf.zoom + memwin.surf.child->parent_xofs*3*memwin.surf.zoom, memwin.surf.width*memwin.surf.zoom*3);

  return SUCCEEDED;
}


int memory_window_plain_block_refresh(void) {

  if (memwin.surf.data == NULL || memwin.surf.view == NULL)
    return FAILED;

  common_copy_block_to_view(memwin.surf.child->parent_xofs, memwin.surf.child->parent_yofs, memwin.surf.child->width, memwin.surf.child->height, &memwin.surf);

  memory_draw_grid();

  /* refresh the memory draw area */
  gdk_draw_rgb_image(memwin.draw_area->window, memwin.draw_area->style->fg_gc[GTK_STATE_NORMAL], memwin.surf.child->parent_xofs*memwin.surf.zoom, memwin.surf.child->parent_yofs*memwin.surf.zoom,
                     memwin.surf.child->width*memwin.surf.zoom, memwin.surf.child->height*memwin.surf.zoom, GDK_RGB_DITHER_MAX, memwin.surf.view + memwin.surf.child->parent_yofs*memwin.surf.zoom*memwin.surf.width*3*memwin.surf.zoom + memwin.surf.child->parent_xofs*3*memwin.surf.zoom, memwin.surf.width*memwin.surf.zoom*3);

  return SUCCEEDED;
}


static void memory_save(GtkWidget *widget, gpointer data) {

  if (memwin.file_name_status == NO) {
    memory_save_as(widget, data);
    return;
  }

  png_save(memwin.file_name, memwin.surf.width, memwin.surf.height, memwin.surf.data);

  editwin.need_to_save = NO;
  memory_window_set_title();
}


static void memory_save_as(GtkWidget *widget, gpointer data) {

  gtk_widget_show(memwin.file_selection_save);
}


static void memory_open(GtkWidget *widget, gpointer data) {

  gtk_widget_show(memwin.file_selection_open);
}


static void memory_file_save_ok(GtkWidget *widget, gpointer data) {

  char tmp[512];
  gchar *n;


  editwin.need_to_save = NO;
  memory_window_set_title();

  n = (gchar *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(memwin.file_selection_save));
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(memwin.file_selection_open), n);

  /* hide the file selection window */
  gtk_widget_hide(memwin.file_selection_save);

  png_save(n, memwin.surf.width, memwin.surf.height, memwin.surf.data);

  strcpy(memwin.file_name, n);
  memwin.file_status = YES;
  memwin.file_name_status = YES;

  /* strip the filename from the whole path */
  common_get_path(n, tmp);
  strcpy(memwin.file_path, tmp);

  memory_window_set_title();
}


static void memory_revert(GtkWidget *widget, gpointer data) {

  if (memwin.file_status == NO)
    return;

  /* backup the old data */
  memory_copy_to_undo();

  memory_file_read(memwin.file_name);
}


static void memory_file_open_ok(GtkWidget *widget, gpointer data) {

  gchar *n;


  /* backup the old data */
  memory_copy_to_undo();

  n = (gchar *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(memwin.file_selection_open));

  /* hide the file selection window */
  gtk_widget_hide(memwin.file_selection_open);

  memory_file_read((char *)n);
}


void memory_file_read(char *n) {

  unsigned char *d;
  int dx, dy, bpp, i, l;
  char tmp[512];


  /* find the file type */
  for (i = strlen(n)-1; i >= 0; i--) {
    if (n[i] == '.')
      break;
  }

  l = 0;
  if (string_compare_nocase(&n[i+1], "png") == 0)
    l |= 1;
  else if (string_compare_nocase(&n[i+1], "jpg") == 0 || string_compare_nocase(&n[i+1], "jpeg") == 0)
    l |= 2;
  else if (string_compare_nocase(&n[i+1], "bmp") == 0)
    l |= 4;
  else if (string_compare_nocase(&n[i+1], "pcx") == 0)
    l |= 8;
  else if (string_compare_nocase(&n[i+1], "tga") == 0)
    l |= 16;
  else
    l |= 31;

  i = FAILED;
  if (l & 1)
    i = png_load(n, &dx, &dy, &bpp, &d);
  if (l & 2 && i == FAILED)
    i = jpg_load(n, &dx, &dy, &bpp, &d);
  if (l & 4 && i == FAILED)
    i = bmp_load(n, &dx, &dy, &bpp, &d);
  if (l & 8 && i == FAILED)
    i = pcx_load(n, &dx, &dy, &bpp, &d);
  if (l & 16 && i == FAILED)
    i = tga_load(n, &dx, &dy, &bpp, &d);

  if (i == FAILED)
    return;

  /* reformat the image so it is of suitable format */
  common_reformat_image(&dx, &dy, &bpp, &d);

  strcpy(memwin.file_name, n);
  memwin.file_status = YES;

  /* strip the filename from the whole path */
  common_get_path(n, tmp);
  strcpy(memwin.file_path, tmp);

  /* the filename can be used later only if the file was a png file */
  if (l == 1) {
    memwin.file_name_status = YES;

    /* set the whole filename with path to the "save as" file selector */
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(memwin.file_selection_save), n);
  }
  else {
    memwin.file_name_status = NO;

    /* set the path to the "save as" file selector */
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(memwin.file_selection_save), tmp);
  }

  if (dx != memwin.surf.width) {
    if (dx == 64)
      gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Size/64x64"), TRUE);
    else if (dx == 128)
      gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Size/128x128"), TRUE);
    else if (dx == 256)
      gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Size/256x256"), TRUE);
    else if (dx == 512)
      gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Size/512x512"), TRUE);
    else if (dx == 1024)
      gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Size/1024x1024"), TRUE);
    else
      gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Size/2048x2048"), TRUE);

    memwin.surf.width = dx;
    memwin.surf.height = dy;
  }

  if (memwin.surf.data != NULL)
    free(memwin.surf.data);
  memwin.surf.data = d;

  memory_resize_view(memwin.surf.zoom);
  common_copy_data_to_view(&memwin.surf);
  memory_draw_editor_rectangle();

  gtk_drawing_area_size(GTK_DRAWING_AREA(memwin.draw_area), memwin.surf.width*memwin.surf.zoom, memwin.surf.height*memwin.surf.zoom);

  /* refresh the windows */
  common_copy_data_to_child(&memwin.surf);

  editor_window_refresh();
  tiled_window_refresh();

  editwin.need_to_save = NO;
  memory_window_set_title();
}


static void memory_file_save_cancel(GtkWidget *widget, gpointer data) {

  gtk_widget_hide(memwin.file_selection_save);
}


static void memory_file_open_cancel(GtkWidget *widget, gpointer data) {

  gtk_widget_hide(memwin.file_selection_open);
}


static void memory_zoom_n(GtkWidget *widget, gpointer data) {

  int zoom;


  zoom = (int)data;
  if (memwin.surf.zoom == zoom)
    return;

  memory_resize_view(zoom);
  common_copy_data_to_view(&memwin.surf);
  memory_draw_editor_rectangle();

  gtk_drawing_area_size(GTK_DRAWING_AREA(memwin.draw_area), memwin.surf.width*memwin.surf.zoom, memwin.surf.height*memwin.surf.zoom);
}


static void memory_zoom_change(GtkWidget *widget, gpointer data) {

  int zoom;


  zoom = memwin.surf.zoom + (int)data;
  if (zoom < 1) zoom = 1;
  else if (zoom > MAX_ZOOM_MEMORY) zoom = MAX_ZOOM_MEMORY;

  memory_resize_view(zoom);
  common_copy_data_to_view(&memwin.surf);
  memory_draw_editor_rectangle();

  gtk_drawing_area_size(GTK_DRAWING_AREA(memwin.draw_area), memwin.surf.width*memwin.surf.zoom, memwin.surf.height*memwin.surf.zoom);
}


static void memory_size_n(GtkWidget *widget, gpointer data) {

  int i;


  i = (int)data;
  if (memwin.surf.width == i && memwin.surf.height == i)
    return;

  /* memory window cannot be smaller than the tile size */
  if (i < memwin.surf.child->width || i < memwin.surf.child->height) {
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
    return;
  }

  /* move the tile pointer to (0, 0) */
  common_set_tile_position(0, 0, &memwin.surf);
  editor_window_set_title();

  common_resize_data(i, i, &editwin.pen_bg, &memwin.surf);
  memory_resize_view(memwin.surf.zoom);
  common_copy_data_to_view(&memwin.surf);
  memory_draw_editor_rectangle();

  gtk_drawing_area_size(GTK_DRAWING_AREA(memwin.draw_area), memwin.surf.width*memwin.surf.zoom, memwin.surf.height*memwin.surf.zoom);

  /* refresh the windows */
  common_copy_data_to_child(&memwin.surf);

  editor_window_refresh();
  tiled_window_refresh();

  editwin.need_to_save = YES;
  memory_window_set_title();
}


static void memory_set_gridsize_n(GtkWidget *widget, gpointer data) {

  int i;

  i = (int)data;
  if (memwin.grid_x == i && memwin.grid_y == i)
    return;

  memwin.grid_x = memwin.grid_y = i;

  editor_window_set_title();

  /* refresh the windows */
  editor_window_refresh();
  memory_window_refresh();
  tiled_window_refresh();

  memory_window_set_title();
}


static void memory_view_n(GtkWidget *widget, gpointer data) {

  int i;

  i = (int)data;
  if (memwin.surf.viewmode == i)
    return;

  memwin.surf.viewmode = i;
  memory_window_refresh();

  memory_window_set_title();
}


/* editor rectangle, snap to grid */
void memory_snaptogrid_editor_rect(int ex, int ey) {

  int x, y, a, b;

  if (memwin.surf.child == NULL)
    return;

  x = memwin.surf.child->parent_xofs;
  y = memwin.surf.child->parent_yofs;
  common_set_tile_position(ex, ey, &memwin.surf);

  if (memwin.surf.child->parent_xofs != x || memwin.surf.child->parent_yofs != y) {
    a = memwin.surf.child->parent_xofs;
    b = memwin.surf.child->parent_yofs;
    memwin.surf.child->parent_xofs = x;
    memwin.surf.child->parent_yofs = y;
    memory_window_plain_block_refresh();
    memwin.surf.child->parent_xofs = a;
    memwin.surf.child->parent_yofs = b;
    memory_window_block_refresh();
  }
}


/* editor rectangle, free movement */
void memory_freemove_editor_rect(int ex, int ey) {
  
  int x, y;

  x = (int)ex/memwin.surf.zoom;
  y = (int)ey/memwin.surf.zoom;

  x -= memwin.surf.child->width/2;
  y -= memwin.surf.child->height/2;

  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;
  if (x >= memwin.surf.width - memwin.surf.child->width)
    x = memwin.surf.width - memwin.surf.child->width;
  if (y >= memwin.surf.height - memwin.surf.child->height)
    y = memwin.surf.height - memwin.surf.child->height;

  if (x != memwin.surf.child->parent_xofs || y != memwin.surf.child->parent_yofs) {
    memory_window_plain_block_refresh();
    memwin.surf.child->parent_xofs = x;
    memwin.surf.child->parent_yofs = y;
    memory_window_block_refresh();
  }
}


static gint memory_button_press(GtkWidget *widget, GdkEventButton *event) {

  memwin.mouse_button = ON;

  if (memwin.snap_to_grid == YES) {
    memory_snaptogrid_editor_rect(event->x, event->y);
  }
  else {
    memory_freemove_editor_rect(event->x, event->y);
  }

  return FALSE;
}


static gint memory_button_release(GtkWidget *widget, GdkEventButton *event) {

  memwin.mouse_button = OFF;

  editor_window_set_title();
  common_copy_data_to_child(&memwin.surf);

  editor_window_refresh();
  tiled_window_refresh();

  return FALSE;
}


static gint memory_motion_notify(GtkWidget *widget, GdkEventButton *event) {

  if (memwin.mouse_button == OFF)
    return FALSE;

  if (memwin.snap_to_grid == YES) {
    memory_snaptogrid_editor_rect(event->x, event->y);
  }
  else {
    memory_freemove_editor_rect(event->x, event->y);
  }

  return FALSE;
}


static void memory_snap(GtkWidget *widget, gpointer data) {

  if (memwin.snap_to_grid == YES)
    memwin.snap_to_grid = NO;
  else
    memwin.snap_to_grid = YES;

  if (memwin.snap_to_grid == YES) {
    /* adjust the coordinates */
    common_compute_tile_position(&memwin.surf);

    memory_window_refresh();
    common_copy_data_to_child(&memwin.surf);

    editor_window_refresh();
    tiled_window_refresh();
  }

  editor_window_set_title();
}


static void memory_window_n(GtkWidget *widget, gpointer data) {

  int i;

  i = (int)data;
  if (i == 0) {
    /* tiled view */
    if (tilwin.window == NULL)
      return;

    if (tilwin.view_status == ON) {
      /* before hiding the window, get its origin */
      gtk_window_get_position(GTK_WINDOW(tilwin.window), &tiled_window_x, &tiled_window_y);
      tilwin.view_status = OFF;
      gtk_widget_hide(tilwin.window);
    }
    else {
      tilwin.view_status = ON;
      gtk_widget_show_all(tilwin.window);
      tiled_window_refresh();
    }
  }
  else if (i == 1) {
    /* palette */
    if (palwin.window == NULL)
      return;

    if (palwin.status == ON) {
      /* before hiding the window, get its origin */
      gtk_window_get_position(GTK_WINDOW(palwin.window), &palette_window_x, &palette_window_y);
      palwin.status = OFF;
      gtk_widget_hide(palwin.window);
    }
    else {
      palwin.status = ON;
      gtk_widget_show_all(palwin.window);
    }
  }
}


static void memory_update_mode(GtkWidget *widget, gpointer data) {

  if (memwin.real_time == NO)
    memwin.real_time = YES;
  else
    memwin.real_time = NO;
}


static gboolean memory_draw_area_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {

  if (memwin.surf.view != NULL) {
    memory_draw_grid();

    gdk_draw_rgb_image(widget->window, widget->style->fg_gc[GTK_STATE_NORMAL], 0, 0,
                       memwin.surf.width*memwin.surf.zoom, memwin.surf.height*memwin.surf.zoom, GDK_RGB_DITHER_MAX, memwin.surf.view, memwin.surf.width*memwin.surf.zoom*3);
  }

  return FALSE;
}
