
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
#include "png.h"


/* internal functions */
static void palette_fg_button_clicked(void);
static void palette_bg_button_clicked(void);
static void palette_selection_change(GtkWidget *widget, gpointer data);
static gint palette_hide_window(GtkWidget *widget, gpointer data);


void palette_struct_init(void) {
  
  palwin.window = NULL;
  palwin.vbox1 = NULL;
  palwin.mode_hbox1 = NULL;
  palwin.selection = NULL;
  palwin.fg_button = NULL;
  palwin.bg_button = NULL;
  palwin.rgroup = NULL;
  palwin.status = ON;
  palwin.select_mode = FG;
}


int palette_window_init(void) {

  char tmp[512];


  /* window */
  palwin.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  sprintf(tmp, "Color Selection");
  gtk_window_set_title(GTK_WINDOW(palwin.window), tmp);

  /* boxes */
  palwin.vbox1 = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(palwin.vbox1), 4);
  
  palwin.mode_hbox1 = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(palwin.mode_hbox1), 4);

  palwin.fg_button = gtk_radio_button_new_with_label(palwin.rgroup, "Foreground");
  palwin.rgroup = gtk_radio_button_group (GTK_RADIO_BUTTON(palwin.fg_button));
  palwin.bg_button = gtk_radio_button_new_with_label(palwin.rgroup, "Background");

  /* color selection widget */
  palwin.selection = gtk_color_selection_new();
  gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION(palwin.selection), TRUE);

  gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(palwin.selection), TRUE);

  /* place boxes and widgets */
  gtk_container_add(GTK_CONTAINER(palwin.window), palwin.vbox1);
  gtk_box_pack_start(GTK_BOX(palwin.vbox1), palwin.selection, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(palwin.vbox1), palwin.mode_hbox1);
  
  gtk_box_pack_start(GTK_BOX(palwin.mode_hbox1), palwin.fg_button, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(palwin.mode_hbox1), palwin.bg_button);

  /* signals */
  gtk_signal_connect(GTK_OBJECT(palwin.window), "delete_event", GTK_SIGNAL_FUNC(palette_hide_window), NULL);
  gtk_signal_connect(GTK_OBJECT(palwin.window), "destroy", GTK_SIGNAL_FUNC(palette_hide_window), NULL);
  gtk_signal_connect(GTK_OBJECT(palwin.selection), "color_changed", GTK_SIGNAL_FUNC(palette_selection_change), NULL);
  
  gtk_signal_connect(GTK_OBJECT(palwin.fg_button), "clicked", GTK_SIGNAL_FUNC(palette_fg_button_clicked), NULL);
  gtk_signal_connect(GTK_OBJECT(palwin.bg_button), "clicked", GTK_SIGNAL_FUNC(palette_bg_button_clicked), NULL);

  gtk_window_set_resizable(GTK_WINDOW(palwin.window), FALSE);

  /* use the default color */
  palette_window_refresh();

  return SUCCEEDED;
}


static void palette_fg_button_clicked(void) {

  palwin.select_mode = FG;
  palette_window_refresh();
}


static void palette_bg_button_clicked(void) {

  palwin.select_mode = BG;
  palette_window_refresh();
}


static void palette_selection_change(GtkWidget *widget, gpointer data) {

  GdkColor col;
  guint16 alpha;

  gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(palwin.selection), &col);
  alpha = gtk_color_selection_get_current_alpha(GTK_COLOR_SELECTION(palwin.selection));

  if (palwin.select_mode == FG) {
    editwin.pen_fg.r = col.red / 256;
    editwin.pen_fg.g = col.green / 256;
    editwin.pen_fg.b = col.blue / 256;
    editwin.pen_fg.a = alpha / 256;
  }
  else {
    editwin.pen_bg.r = col.red / 256;
    editwin.pen_bg.g = col.green / 256;
    editwin.pen_bg.b = col.blue / 256;
    editwin.pen_bg.a = alpha / 256;
  }
}


int palette_window_refresh(void) {

  GdkColor col;
  guint16 alpha;

  if (palwin.select_mode == FG) {
    col.red   = ((gdouble)editwin.pen_fg.r)*256;
    col.green = ((gdouble)editwin.pen_fg.g)*256;
    col.blue  = ((gdouble)editwin.pen_fg.b)*256;
    alpha     = ((gdouble)editwin.pen_fg.a)*256;
  }
  else {
    col.red   = ((gdouble)editwin.pen_bg.r)*256;
    col.green = ((gdouble)editwin.pen_bg.g)*256;
    col.blue  = ((gdouble)editwin.pen_bg.b)*256;
    alpha     = ((gdouble)editwin.pen_bg.a)*256;
  }

  gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(palwin.selection), &col);
  gtk_color_selection_set_current_alpha(GTK_COLOR_SELECTION(palwin.selection), alpha);

  gtk_color_selection_set_previous_color(GTK_COLOR_SELECTION(palwin.selection), &col);
  gtk_color_selection_set_previous_alpha(GTK_COLOR_SELECTION(palwin.selection), alpha);

  return SUCCEEDED;
}


static gint palette_hide_window(GtkWidget *widget, gpointer data) {

  gtk_check_menu_item_set_active((GtkCheckMenuItem *)gtk_item_factory_get_widget(memwin.item_factory, "/Windows/Palette"), FALSE);

  return TRUE;
}
