
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
#include "tiled.h"
#include "png.h"


/* internal functions */
static void tiled_window_set_title(void);
static void tiled_zoom_n(GtkWidget *widget, gpointer data);
static void tiled_zoom_change(GtkWidget *widget, gpointer data);
static void tiled_view_n(GtkWidget *widget, gpointer data);
static void tiled_size_n(GtkWidget *widget, gpointer data);
static gint tiled_hide_window(GtkWidget *widget, gpointer data);
static void tiled_update_mode(GtkWidget *widget, gpointer data);
static gboolean tiled_draw_area_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);


GtkItemFactoryEntry tiled_menu_items[] = {
  { "/_Zoom",       NULL,         NULL, 0, "<Branch>" },
  { "/Zoom/Decrease", "minus",    tiled_zoom_change, -1, NULL },
  { "/Zoom/100%",   "1",          tiled_zoom_n, 1, NULL },
  { "/Zoom/200%",   NULL,         tiled_zoom_n, 2, NULL },
  { "/Zoom/300%",   NULL,         tiled_zoom_n, 3, NULL },
  { "/Zoom/400%",   NULL,         tiled_zoom_n, 4, NULL },
  { "/Zoom/500%",   NULL,         tiled_zoom_n, 5, NULL },
  { "/Zoom/1000%",  NULL,         tiled_zoom_n, 10, NULL },
  { "/Zoom/Increase", "plus",     tiled_zoom_change, 1, NULL },
  { "/_Size",       NULL,         NULL, 0, "<Branch>" },
  { "/Size/1x1",    NULL,         tiled_size_n, 1, "<RadioItem>" },
  { "/Size/3x3",    NULL,         tiled_size_n, 3, "/Size/1x1" },
  { "/Size/5x5",    NULL,         tiled_size_n, 5, "/Size/1x1" },
  { "/Size/7x7",    NULL,         tiled_size_n, 7, "/Size/1x1" },
  { "/Size/9x9",    NULL,         tiled_size_n, 9, "/Size/1x1" },
  { "/_View",       NULL,         NULL, 0, "<Branch>" },
  { "/View/RGBA",   "<alt>6",     tiled_view_n, VIEW_MODE_RGBA, "<RadioItem>" },
  { "/View/RGB",    "<alt>7",     tiled_view_n, VIEW_MODE_RGB, "/View/RGBA" },
  { "/View/Alpha",  "<alt>8",     tiled_view_n, VIEW_MODE_ALPHA, "/View/RGBA" },
  { "/View/sep1",   NULL,         NULL, 0, "<Separator>" },
  { "/View/Updated in Real-Time", NULL, tiled_update_mode, 0, "<ToggleItem>" },
};

void
tiled_struct_init(void)
{
  tilwin.window = NULL;
  tilwin.vbox1 = NULL;
  tilwin.vbox2 = NULL;
  tilwin.hbox1 = NULL;
  tilwin.menubar = NULL;
  tilwin.draw_area = NULL;
  tilwin.file_selection_save = NULL;
  tilwin.file_selection_open = NULL;
  tilwin.accel_group = NULL;
  tilwin.item_factory = NULL;

  tilwin.surf.width = 0;
  tilwin.surf.height = 0;
  tilwin.surf.zoom = 2;
  tilwin.surf.viewmode = VIEW_MODE_RGBA;
  tilwin.surf.tilenum = 0;
  tilwin.surf.data = NULL;
  tilwin.surf.view = NULL;
  tilwin.surf.parent = NULL;
  tilwin.surf.parent_xofs = 0;
  tilwin.surf.parent_yofs = 0;
  tilwin.surf.child = NULL;

  tilwin.tiles_dx = 3;
  tilwin.tiles_dy = 3;
  tilwin.view_status = ON;
  tilwin.real_time = ON;
}

int tiled_window_init(void) {

  /* copy data from editor window */
  tilwin.surf.width = editwin.surf.width*tilwin.tiles_dx;
  tilwin.surf.height = editwin.surf.height*tilwin.tiles_dy;

  /* window */
  tilwin.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  tiled_window_set_title();

  /* menu */
  tilwin.accel_group = gtk_accel_group_new();
  tilwin.item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", tilwin.accel_group);
  gtk_item_factory_create_items(tilwin.item_factory, sizeof(tiled_menu_items)/sizeof(tiled_menu_items[0]), tiled_menu_items, NULL);
  gtk_window_add_accel_group(GTK_WINDOW(tilwin.window), tilwin.accel_group);
  tilwin.menubar = gtk_item_factory_get_widget(tilwin.item_factory, "<main>");

  /* drawing area */
  tilwin.draw_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(tilwin.draw_area), tilwin.surf.width*tilwin.surf.zoom, tilwin.surf.height*tilwin.surf.zoom);

  /* boxes */
  tilwin.vbox1 = gtk_vbox_new(FALSE, 0);
  tilwin.vbox2 = gtk_vbox_new(FALSE, 0);
  tilwin.hbox1 = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(tilwin.vbox1), 1);
  gtk_container_border_width(GTK_CONTAINER(tilwin.vbox2), 1);
  gtk_container_border_width(GTK_CONTAINER(tilwin.hbox1), 1);

  /* place boxes and widgets */
  gtk_container_add(GTK_CONTAINER(tilwin.window), tilwin.vbox1);
  gtk_box_pack_start(GTK_BOX(tilwin.vbox1), tilwin.menubar, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(tilwin.vbox1), tilwin.hbox1);
  gtk_box_pack_start(GTK_BOX(tilwin.hbox1), tilwin.vbox2, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(tilwin.vbox2), tilwin.draw_area, TRUE, FALSE, 0);

  /* signals */
  gtk_signal_connect(GTK_OBJECT(tilwin.draw_area), "expose_event", GTK_SIGNAL_FUNC(tiled_draw_area_expose), NULL);
  gtk_signal_connect(GTK_OBJECT(tilwin.window), "delete_event", GTK_SIGNAL_FUNC(tiled_hide_window), NULL);
  gtk_signal_connect(GTK_OBJECT(tilwin.window), "destroy", GTK_SIGNAL_FUNC(tiled_hide_window), NULL);

  gtk_window_set_resizable(GTK_WINDOW(tilwin.window), FALSE);

  return SUCCEEDED;
}


void tiled_resize_view(int zoom) {

  tilwin.surf.view = realloc(tilwin.surf.view, tilwin.surf.width*zoom*tilwin.surf.height*zoom*3);
  if (tilwin.surf.view == NULL) {
    fprintf(stderr, "TILED_RESIZE_VIEW: Out of tiled error.\n");
    return;
  }

  tilwin.surf.zoom = zoom;
}


void tiled_resize_data(int dx, int dy) {

  tilwin.surf.data = realloc(tilwin.surf.data, dx*dy*4);
  if (tilwin.surf.data == NULL) {
    fprintf(stderr, "TILED_RESIZE_DATA: Out of tiled error.\n");
    return;
  }

  memset(tilwin.surf.data, 255, dx*dy*4);

  tilwin.surf.width = dx;
  tilwin.surf.height = dy;
}


static void tiled_window_set_title(void) {

  char tmp[512];


  if (tilwin.window == NULL)
    return;

  sprintf(tmp, "Tiled View | %dx%d | ", tilwin.tiles_dx, tilwin.tiles_dy);

  if (tilwin.surf.viewmode == VIEW_MODE_RGBA)
    strcat(tmp, "RGBA");
  else if (tilwin.surf.viewmode == VIEW_MODE_RGB)
    strcat(tmp, "RGB");
  else if (tilwin.surf.viewmode == VIEW_MODE_ALPHA)
    strcat(tmp, "Alpha");

  gtk_window_set_title(GTK_WINDOW(tilwin.window), tmp);
}


static void tiled_zoom_n(GtkWidget *widget, gpointer data) {

  int zoom;


  zoom = (int)data;
  if (tilwin.surf.zoom == zoom)
    return;

  tiled_resize_view(zoom);
  common_copy_data_to_view(&tilwin.surf);

  gtk_drawing_area_size(GTK_DRAWING_AREA(tilwin.draw_area), tilwin.surf.width*tilwin.surf.zoom, tilwin.surf.height*tilwin.surf.zoom);
}

static void tiled_zoom_change(GtkWidget *widget, gpointer data) {

  int zoom;


  zoom = tilwin.surf.zoom + (int)data;
  if (zoom < 1) zoom = 1;
  else if (zoom > MAX_ZOOM_TILED) zoom = MAX_ZOOM_TILED;

  tiled_resize_view(zoom);
  common_copy_data_to_view(&tilwin.surf);

  gtk_drawing_area_size(GTK_DRAWING_AREA(tilwin.draw_area), tilwin.surf.width*tilwin.surf.zoom, tilwin.surf.height*tilwin.surf.zoom);
}


static void tiled_view_n(GtkWidget *widget, gpointer data) {

  int i;


  i = (int)data;
  if (tilwin.surf.viewmode == i)
    return;

  tilwin.surf.viewmode = i;
  tiled_window_refresh();
  tiled_window_set_title();
}


int tiled_window_import_data(void) {

  int x, y, a, b, i, n, m, o, p;


  for (y = 0; y < tilwin.tiles_dy; y++) {
    n = (y*editwin.surf.height*tilwin.surf.width)<<2;
    for (x = 0; x < tilwin.tiles_dx; x++) {
      m = ((x*editwin.surf.width)<<2) + n;
      for (i = 0, a = 0; a < editwin.surf.height; a++) {
	o = ((a*tilwin.surf.width)<<2) + m;
	p = o;
	for (b = 0; b < editwin.surf.width; b++) {
	  tilwin.surf.data[p++] = editwin.surf.data[i++];
	  tilwin.surf.data[p++] = editwin.surf.data[i++];
	  tilwin.surf.data[p++] = editwin.surf.data[i++];
	  tilwin.surf.data[p++] = editwin.surf.data[i++];
	}
      }
    }
  }

  return SUCCEEDED;
}


int tiled_window_refresh(void) {

  if (tilwin.surf.data == NULL || tilwin.surf.view == NULL)
    return FAILED;

  if (tilwin.view_status == OFF)
    return FAILED;

  /* tiled window */
  if (tilwin.surf.width != editwin.surf.width*tilwin.tiles_dx || tilwin.surf.height != editwin.surf.height*tilwin.tiles_dy) {
    tiled_resize_data(editwin.surf.width*tilwin.tiles_dx, editwin.surf.height*tilwin.tiles_dy);
    tiled_resize_view(tilwin.surf.zoom);
    tiled_window_import_data();
    common_copy_data_to_view(&tilwin.surf);
    gtk_drawing_area_size(GTK_DRAWING_AREA(tilwin.draw_area), tilwin.surf.width*tilwin.surf.zoom, tilwin.surf.height*tilwin.surf.zoom);
    return SUCCEEDED;
  }

  tiled_window_import_data();
  common_copy_data_to_view(&tilwin.surf);

  /* refresh the tiled draw area */
  gtk_widget_queue_draw(tilwin.draw_area);

  return SUCCEEDED;
}


static void tiled_size_n(GtkWidget *widget, gpointer data) {

  int i;


  i = (int)data;
  if (tilwin.tiles_dx == i && tilwin.tiles_dy == i)
    return;

  tilwin.tiles_dx = i;
  tilwin.tiles_dy = i;

  tiled_window_refresh();
  tiled_window_set_title();
}


static gint tiled_hide_window(GtkWidget *widget, gpointer data) {

  gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Windows/Tiled View"), FALSE);

  return TRUE;
}


static void tiled_update_mode(GtkWidget *widget, gpointer data) {

  if (tilwin.real_time == NO)
    tilwin.real_time = YES;
  else
    tilwin.real_time = NO;
}


static gboolean tiled_draw_area_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {

  if (tilwin.surf.view != NULL) {
    gdk_draw_rgb_image(widget->window, widget->style->fg_gc[GTK_STATE_NORMAL], 0, 0,
		       tilwin.surf.width*tilwin.surf.zoom, tilwin.surf.height*tilwin.surf.zoom, GDK_RGB_DITHER_MAX, tilwin.surf.view, tilwin.surf.width*tilwin.surf.zoom*3);
  }

  return FALSE;
}
