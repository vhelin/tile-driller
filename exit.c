
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include "defines.h"
#include "memory.h"
#include "common.h"
#include "prefs.h"
#include "tiled.h"


/* exit dialog */
GtkWidget *exit_dialog = NULL;


static void exit_dialog_response(GtkWidget *widget, gint resp) {

  if (resp == GTK_RESPONSE_ACCEPT) {
    /* save the prefs before quitting */
    prefs_save();

    gtk_main_quit();
  }
  else
    gtk_widget_hide(GTK_WIDGET(exit_dialog));
}


int exit_dialog_init(void) {

  GtkWidget *label;


  exit_dialog = gtk_dialog_new_with_buttons("Really quit?",
					    GTK_WINDOW(memwin.window),
					    0,
					    GTK_STOCK_OK,
					    GTK_RESPONSE_ACCEPT,
					    GTK_STOCK_CANCEL,
					    GTK_RESPONSE_REJECT,
					    NULL);

  label = gtk_label_new("Quit without saving?");
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(exit_dialog)->vbox), label);

  g_signal_connect_swapped(GTK_OBJECT(exit_dialog), 
			   "response", 
			   G_CALLBACK(exit_dialog_response),
			   NULL);

  gtk_signal_connect(GTK_OBJECT(exit_dialog), "delete_event", GTK_SIGNAL_FUNC(common_gtk_hide_widget), NULL);
  gtk_signal_connect(GTK_OBJECT(exit_dialog), "destroy", GTK_SIGNAL_FUNC(common_gtk_hide_widget), NULL);

  return SUCCEEDED;
}
