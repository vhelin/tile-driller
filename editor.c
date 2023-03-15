
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
#include "common.h"
#include "editor.h"
#include "palette.h"
#include "tiled.h"
#include "jpg.h"
#include "png.h"
#include "bmp.h"
#include "pcx.h"
#include "tga.h"
#include "string.h"


struct edit_window editwin;

/* internal functions */
static void editor_open(GtkWidget *widget, gpointer data);
static void editor_save(GtkWidget *widget, gpointer data);
static void editor_save_as(GtkWidget *widget, gpointer data);
static void editor_file_save_ok(GtkWidget *widget, gpointer data);
static void editor_file_save_cancel(GtkWidget *widget, gpointer data);
static void editor_file_open_ok(GtkWidget *widget, gpointer data);
static void editor_file_open_cancel(GtkWidget *widget, gpointer data);
static void editor_draw_pick(int x, int y);
static void editor_draw_pen(int x, int y);
static void editor_draw_blur_gaussian(unsigned int x, unsigned int y);
static void editor_draw_blur_normal(unsigned int x, unsigned int y);
static int  editor_need_to_draw(int x, int y);
static void editor_draw_cursor(int x, int y, GtkWidget *widget);
static void editor_undo(GtkWidget *widget, gpointer data);
static void editor_clear(GtkWidget *widget, gpointer data);
static void editor_filters_negate(GtkWidget *widget, gpointer data);
static void editor_filters_blur_normal(GtkWidget *widget, gpointer data);
static void editor_filters_blur_gaussian(GtkWidget *widget, gpointer data);
static void editor_filters_build_alpha(GtkWidget *widget, gpointer data);
static void editor_mirror_lr(GtkWidget *widget, gpointer data);
static void editor_mirror_tb(GtkWidget *widget, gpointer data);
static void editor_copy(GtkWidget *widget, gpointer data);
static void editor_paste(GtkWidget *widget, gpointer data);
static void editor_erosion(GtkWidget *widget, gpointer data);
static void editor_flip_x(GtkWidget *widget, gpointer data);
static void editor_flip_y(GtkWidget *widget, gpointer data);
static void editor_mirror_and_blend_lin_x(GtkWidget *widget, gpointer data);
static void editor_mirror_and_blend_lin_y(GtkWidget *widget, gpointer data);
static void editor_mirror_and_blend_sto_x(GtkWidget *widget, gpointer data);
static void editor_mirror_and_blend_sto_y(GtkWidget *widget, gpointer data);
static void editor_rotate_left(GtkWidget *widget, gpointer data);
static void editor_rotate_right(GtkWidget *widget, gpointer data);
static void editor_zoom_change(GtkWidget *widget, gpointer data);
static void editor_zoom_n(GtkWidget *widget, gpointer data);
static void editor_size_n(GtkWidget *widget, gpointer data);
static void editor_view_n(GtkWidget *widget, gpointer data);
static void editor_scale_n(GtkWidget *widget, gpointer data);
static void editor_tools_n(GtkWidget *widget, gpointer data);
static void editor_shift_left(GtkWidget *widget, gpointer data);
static void editor_shift_right(GtkWidget *widget, gpointer data);
static void editor_shift_up(GtkWidget *widget, gpointer data);
static void editor_shift_down(GtkWidget *widget, gpointer data);
static gint editor_key_press(GtkWidget *widget, GdkEventKey *event);
static gint editor_key_release(GtkWidget *widget, GdkEventKey *event);
static gint editor_button_press(GtkWidget *widget, GdkEventButton *event);
static gint editor_button_release(GtkWidget *widget, GdkEventButton *event);
static gint editor_motion_notify(GtkWidget *widget, GdkEventButton *event);
static gint editor_leave_notify(GtkWidget *widget, GdkEventButton *event);
static gboolean editor_draw_area_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);
static const char *editor_statusline(void);
void editor_draw_grid(void);
static void editor_change_showgrid(GtkWidget *widget, gpointer data);


GtkItemFactoryEntry editor_menu_items[] = {
  { "/_File",           NULL,          NULL, 0, "<Branch>" },
  { "/File/_Open",      "<control>O",  editor_open, 0, NULL },
  { "/File/_Save",      "<control>S",  editor_save, 0, NULL },
  { "/File/Save As",    NULL,          editor_save_as, 0, NULL },
  { "/_Tools",           NULL,         NULL, 0, "<Branch>" },
  { "/Tools/Blur",       NULL,         NULL, 0, "<Branch>" },
  { "/Tools/Blur/Gaussian", "<alt>1",editor_tools_n, TOOL_MODE_BLUR_GAUSSIAN, "<RadioItem>" },
  { "/Tools/Blur/Normal", "<alt>2",  editor_tools_n, TOOL_MODE_BLUR_NORMAL, "/Tools/Blur/Gaussian" },
  { "/Tools/Fill",       "<alt>3",     editor_tools_n, TOOL_MODE_FILL, "/Tools/Blur/Gaussian" },
  { "/Tools/Aggressive fill", "<alt>4",     editor_tools_n, TOOL_MODE_AGGRESSIVE_FILL, "/Tools/Blur/Gaussian" },
  { "/Tools/Gradient fill", "<alt>5",     editor_tools_n, TOOL_MODE_GRADIENT_FILL, "/Tools/Blur/Gaussian" },
  { "/Tools/Pick",       "<alt>6",     editor_tools_n, TOOL_MODE_PICK, "/Tools/Blur/Gaussian" },
  { "/Tools/Pen 1x1",    "<alt>7",     editor_tools_n, TOOL_MODE_PEN_1X1, "/Tools/Blur/Gaussian" },
  { "/Tools/Pen 3x3",    "<alt>8",     editor_tools_n, TOOL_MODE_PEN_3X3, "/Tools/Blur/Gaussian" },
  { "/Tools/Pen 5x5",    "<alt>9",     editor_tools_n, TOOL_MODE_PEN_5X5, "/Tools/Blur/Gaussian" },
  { "/Tools/Pen 7x7",    "<alt>0",     editor_tools_n, TOOL_MODE_PEN_7X7, "/Tools/Blur/Gaussian" },
  { "/_Edit",            NULL,         NULL, 0, "<Branch>" },
  { "/Edit/Undo",        "<control>Z", editor_undo, 0, NULL },
  { "/Edit/sep1",        NULL,         NULL, 0, "<Separator>" },
  { "/Edit/_Copy",       "<control>C", editor_copy, 0, NULL },
  { "/Edit/Paste",       "<control>V", editor_paste, 0, NULL },
  { "/Edit/sep2",        NULL,         NULL, 0, "<Separator>" },
  { "/Edit/Erosion",     NULL,         editor_erosion, 0, NULL },
  { "/Edit/Flip",        NULL,         NULL, 0, "<Branch>" },
  { "/Edit/Flip/X-Axis", "x",          editor_flip_x, 0, NULL },
  { "/Edit/Flip/Y-Axis", "y",          editor_flip_y, 0, NULL },
  { "/Edit/Mirror",      NULL,         NULL, 0, "<Branch>" },
  { "/Edit/Mirror/Left->Right",   NULL, editor_mirror_lr, 0, NULL },
  { "/Edit/Mirror/Top->Bottom",   NULL, editor_mirror_tb, 0, NULL },
  { "/Edit/Mirror & Blend",           NULL,         NULL, 0, "<Branch>" },
  { "/Edit/Mirror & Blend/Left->Right (Lin)", NULL, editor_mirror_and_blend_lin_x, 0, NULL },
  { "/Edit/Mirror & Blend/Left->Right (Sto)", NULL, editor_mirror_and_blend_sto_x, 0, NULL },
  { "/Edit/Mirror & Blend/Top->Bottom (Lin)", NULL, editor_mirror_and_blend_lin_y, 0, NULL },
  { "/Edit/Mirror & Blend/Top->Bottom (Sto)", NULL, editor_mirror_and_blend_sto_y, 0, NULL },
  { "/Edit/Rotate",      NULL,         NULL, 0, "<Branch>" },
  { "/Edit/Rotate/Left", "q",          editor_rotate_left, 0, NULL },
  { "/Edit/Rotate/Right","e",          editor_rotate_right, 0, NULL },
  { "/Edit/Scale",       NULL,         NULL, 0, "<Branch>" },
  { "/Edit/Scale/50%",   NULL,         editor_scale_n, 2, NULL },
  { "/Edit/Scale/25%",   NULL,         editor_scale_n, 4, NULL },
  { "/Edit/Scale/12.5%", NULL,         editor_scale_n, 8, NULL },
  { "/Edit/Shift",       NULL,         NULL, 0, "<Branch>" },
  { "/Edit/Shift/Left",  "a",          editor_shift_left, 0, NULL },
  { "/Edit/Shift/Right", "d",          editor_shift_right, 0, NULL },
  { "/Edit/Shift/Up",    "w",          editor_shift_up, 0, NULL },
  { "/Edit/Shift/Down",  "s",          editor_shift_down, 0, NULL },
  { "/Edit/sep8",        NULL,         NULL, 0, "<Separator>" },
  { "/Edit/Clear",       "<control>K", editor_clear, 0, NULL },
  { "/_Filters",         NULL,         NULL, 0, "<Branch>" },
  { "/Filters/Blur",     NULL,         NULL, 0, "<Branch>" },
  { "/Filters/Blur/Gaussian", NULL,  editor_filters_blur_gaussian, 0, NULL },
  { "/Filters/Blur/Normal",   NULL,  editor_filters_blur_normal, 0, NULL },
  { "/Filters/Build Alpha", NULL,      editor_filters_build_alpha, 0, NULL },
  { "/Filters/Negate",   NULL,         editor_filters_negate, 0, NULL },
  { "/_Zoom",            NULL,         NULL, 0, "<Branch>" },
  { "/Zoom/Decrease",    "minus",      editor_zoom_change, -1, NULL },
  { "/Zoom/100%",        "1",          editor_zoom_n, 1,  NULL },
  { "/Zoom/200%",        NULL,         editor_zoom_n, 2,  NULL },
  { "/Zoom/400%",        NULL,         editor_zoom_n, 4,  NULL },
  { "/Zoom/600%",        NULL,         editor_zoom_n, 6,  NULL },
  { "/Zoom/1000%",       "0",          editor_zoom_n, 10, NULL },
  { "/Zoom/1500%",       NULL,         editor_zoom_n, 15, NULL },
  { "/Zoom/2000%",       NULL,         editor_zoom_n, 20, NULL },
  { "/Zoom/Increase",    "plus",       editor_zoom_change, 1, NULL },
  { "/_Size",            NULL,         NULL, 0, "<Branch>" },
  { "/Size/8x8",         NULL,         editor_size_n, 8, "<RadioItem>" },
  { "/Size/16x16",       NULL,         editor_size_n, 16, "/Size/8x8" },
  { "/Size/32x32",       NULL,         editor_size_n, 32, "/Size/8x8" },
  { "/Size/64x64",       NULL,         editor_size_n, 64, "/Size/8x8" },
  { "/Size/128x128",     NULL,         editor_size_n, 128, "/Size/8x8" },
  { "/Size/256x256",     NULL,         editor_size_n, 256, "/Size/8x8" },
  { "/Size/512x512",     NULL,         editor_size_n, 512, "/Size/8x8" },
  { "/Size/1024x1024",   NULL,         editor_size_n, 1024, "/Size/8x8" },
  { "/_View",            NULL,         NULL, 0, "<Branch>" },
  { "/View/RGBA",        NULL,         editor_view_n, VIEW_MODE_RGBA, "<RadioItem>" },
  { "/View/RGB",         NULL,         editor_view_n, VIEW_MODE_RGB, "/View/RGBA" },
  { "/View/Alpha",       NULL,         editor_view_n, VIEW_MODE_ALPHA, "/View/RGBA" },
  { "/View/sep3",        NULL,         NULL, 0, "<Separator>" },
  { "/View/Show Grid",   NULL,         editor_change_showgrid, 0, "<ToggleItem>" },

};


void editor_struct_init(void) {

  editwin.surf.width = 16;
  editwin.surf.height = 16;
  editwin.surf.zoom = 10;
  editwin.surf.viewmode = VIEW_MODE_RGBA;
  editwin.surf.tilenum = 0;
  editwin.surf.data = NULL;
  editwin.surf.view = NULL;
  editwin.surf.parent = NULL;
  editwin.surf.parent_xofs = 0;
  editwin.surf.parent_yofs = 0;
  editwin.surf.child = NULL;

  editwin.show_grid = 1;
  editwin.editor_copy_dx = 0;
  editwin.editor_copy_dy = 0;
  editwin.editor_copy_data = NULL;
  editwin.mouse_button = OFF;
  editwin.mouse_button_mode = FG;

  editwin.pen_fg.r = 0;
  editwin.pen_fg.g = 0;
  editwin.pen_fg.b = 0;
  editwin.pen_fg.a = 255;

  editwin.pen_bg.r = 255;
  editwin.pen_bg.g = 255;
  editwin.pen_bg.b = 255;
  editwin.pen_bg.a = 255;
  editwin.tool_mode = TOOL_MODE_PEN_1X1;
  editwin.undo_stack_max_depth = 100;

  editwin.editor_prev_x = -1;
  editwin.editor_prev_y = -1;
  editwin.editor_curr_x = -1;
  editwin.editor_curr_y = -1;
  editwin.editor_next_x = -1;
  editwin.editor_next_y = -1;
  editwin.editor_modkey = 0;
  editwin.linecolor = &editwin.pen_fg;
  memset(editwin.file_name, 0, 255);
  memset(editwin.file_path, 0, 255);
  editwin.file_name_status = NO;
  editwin.need_to_save = NO;
  editwin.mouse_old_x = -1;
  editwin.mouse_old_y = -1;
}


int editor_window_init(void) {

  /* window */
  editwin.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  editor_window_set_title();

  /* menu */
  editwin.accel_group = gtk_accel_group_new();
  editwin.item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", editwin.accel_group);
  gtk_item_factory_create_items(editwin.item_factory, sizeof(editor_menu_items)/sizeof(editor_menu_items[0]), editor_menu_items, NULL);
  gtk_window_add_accel_group(GTK_WINDOW(editwin.window), editwin.accel_group);
  editwin.menubar = gtk_item_factory_get_widget(editwin.item_factory, "<main>");

  /* drawing area */
  editwin.draw_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(editwin.draw_area), editwin.surf.width*editwin.surf.zoom, editwin.surf.height*editwin.surf.zoom);
  gtk_widget_add_events(editwin.draw_area,
                        GDK_KEY_RELEASE_MASK |
                        GDK_KEY_PRESS_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_RELEASE_MASK | GDK_KEY_RELEASE_MASK | GDK_POINTER_MOTION_MASK);

  /* boxes */
  editwin.vbox1 = gtk_vbox_new(FALSE, 0);
  editwin.vbox2 = gtk_vbox_new(FALSE, 0);
  editwin.hbox1 = gtk_hbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(editwin.vbox1), 1);
  gtk_container_border_width(GTK_CONTAINER(editwin.vbox2), 1);
  gtk_container_border_width(GTK_CONTAINER(editwin.hbox1), 1);

  /* place boxes and widgets */
  gtk_container_add(GTK_CONTAINER(editwin.window), editwin.vbox1);
  gtk_box_pack_start(GTK_BOX(editwin.vbox1), editwin.menubar, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(editwin.vbox1), editwin.hbox1);
  gtk_box_pack_start(GTK_BOX(editwin.hbox1), editwin.vbox2, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(editwin.vbox2), editwin.draw_area, TRUE, FALSE, 0);

  editwin.statuslabel = gtk_label_new(editor_statusline());
  gtk_container_add(GTK_CONTAINER(editwin.vbox1), editwin.statuslabel);

  /* file selections */
  editwin.file_selection_save = gtk_file_selection_new("Save PNG Image");
  editwin.file_selection_open = gtk_file_selection_new("Open BMP/JPG/PCX/PNG/TGA Image");

  /* signals */
  gtk_signal_connect(GTK_OBJECT(editwin.draw_area), "expose_event", GTK_SIGNAL_FUNC(editor_draw_area_expose), NULL);
  gtk_signal_connect(GTK_OBJECT(editwin.draw_area), "button_press_event", GTK_SIGNAL_FUNC(editor_button_press), NULL);
  gtk_signal_connect(GTK_OBJECT(editwin.draw_area), "button_release_event", GTK_SIGNAL_FUNC(editor_button_release), NULL);
  gtk_signal_connect(GTK_OBJECT(editwin.draw_area), "motion_notify_event", GTK_SIGNAL_FUNC(editor_motion_notify), NULL);
  gtk_signal_connect(GTK_OBJECT(editwin.draw_area), "leave_notify_event", GTK_SIGNAL_FUNC(editor_leave_notify), NULL);
  gtk_signal_connect(GTK_OBJECT(editwin.window), "key_press_event", GTK_SIGNAL_FUNC(editor_key_press), NULL);
  gtk_signal_connect(GTK_OBJECT(editwin.window), "key_release_event", GTK_SIGNAL_FUNC(editor_key_release), NULL);
  gtk_signal_connect(GTK_OBJECT(editwin.window), "delete_event", GTK_SIGNAL_FUNC(common_gtk_exit), NULL);
  gtk_signal_connect(GTK_OBJECT(editwin.window), "destroy", GTK_SIGNAL_FUNC(common_gtk_exit), NULL);

  gtk_signal_connect(GTK_OBJECT(editwin.file_selection_save), "delete_event", GTK_SIGNAL_FUNC(common_gtk_hide_widget), NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(editwin.file_selection_save)->ok_button), "clicked", GTK_SIGNAL_FUNC(editor_file_save_ok), NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(editwin.file_selection_save)->cancel_button), "clicked", GTK_SIGNAL_FUNC(editor_file_save_cancel), NULL);

  gtk_signal_connect(GTK_OBJECT(editwin.file_selection_open), "delete_event", GTK_SIGNAL_FUNC(common_gtk_hide_widget), NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(editwin.file_selection_open)->ok_button), "clicked", GTK_SIGNAL_FUNC(editor_file_open_ok), NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(editwin.file_selection_open)->cancel_button), "clicked", GTK_SIGNAL_FUNC(editor_file_open_cancel), NULL);

  gtk_window_set_resizable(GTK_WINDOW(editwin.window), FALSE);

  editwin.file_name[0] = 0;

  undo_init(editwin.undo_stack_max_depth, &editwin.undo_data);
  editwin.surf.parent = &memwin.surf;
  editwin.surf.child = NULL;

  return SUCCEEDED;
}


void editor_set_linedraw_color() {
  
  if (editwin.mouse_button_mode == BG)
    editwin.linecolor = &editwin.pen_bg;
  else
    editwin.linecolor = &editwin.pen_fg;
}


void editor_gdk_refresh_rgbimage(int x, int y, int w, int h, struct td_surf *s, GtkWidget *widget) {

  /* refresh the editor draw area */
  if (w <= 0)
    w = s->width;
  if (h <= 0)
    h = s->height;

  if (y + h >= editwin.surf.height)
    h = editwin.surf.height - y;
  if (x + w >= editwin.surf.width)
    w = editwin.surf.width - x;

  gdk_draw_rgb_image(widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
                     x*s->zoom, y*s->zoom,
                     w*s->zoom, h*s->zoom,
                     GDK_RGB_DITHER_MAX,
                     s->view + y * s->zoom * s->width * 3 * s->zoom + x * 3 * s->zoom,
                     s->width * s->zoom * 3);
}


static gint editor_key_release(GtkWidget *widget, GdkEventKey *event) {
  
  if (event->keyval == GDK_Shift_L || event->keyval == GDK_Shift_R) {
    if ((editwin.editor_modkey&MK_SHIFT)) {
      common_copy_data_to_view(&editwin.surf);
      gtk_widget_queue_draw(editwin.draw_area);
    }
    editwin.editor_modkey &= ~MK_SHIFT;
  }

  if (event->keyval == GDK_Control_L || event->keyval == GDK_Control_R) {
    editwin.editor_modkey &= ~MK_CONTROL;
  }

  return FALSE;
}


static gint editor_key_press(GtkWidget *widget, GdkEventKey *event) {

  if (editwin.surf.parent == NULL)
    return FALSE;

  if (event->keyval == GDK_Right) {
    /* move one tile right */
    if (editwin.surf.parent_xofs < editwin.surf.parent->width - editwin.surf.width) {
      memory_window_plain_block_refresh();
      editwin.surf.parent_xofs += editwin.surf.width;
      if (common_compute_tile_position(&editwin.surf) == SUCCEEDED) {
        editor_window_set_title();
        common_copy_data_to_child(&editwin.surf);
        editor_window_refresh();
        memory_window_block_refresh();
        tiled_window_refresh();
      }
    }
  }
  else if (event->keyval == GDK_Left) {
    /* move one tile left */
    if (editwin.surf.parent_xofs >= editwin.surf.width) {
      memory_window_plain_block_refresh();
      editwin.surf.parent_xofs -= editwin.surf.width;
      if (common_compute_tile_position(&editwin.surf) == SUCCEEDED) {
        editor_window_set_title();
        common_copy_data_to_child(&editwin.surf);

        editor_window_refresh();
        memory_window_block_refresh();
        tiled_window_refresh();
      }
    }
  }
  else if (event->keyval == GDK_Up) {
    /* move one tile up */
    if (editwin.surf.parent_yofs >= editwin.surf.height) {
      memory_window_plain_block_refresh();
      editwin.surf.parent_yofs -= editwin.surf.height;
      if (common_compute_tile_position(&editwin.surf) == SUCCEEDED) {
        editor_window_set_title();
        common_copy_data_to_child(&editwin.surf);
        editor_window_refresh();
        memory_window_block_refresh();
        tiled_window_refresh();
      }
    }
  }
  else if (event->keyval == GDK_Down) {
    /* move one tile down */
    if (editwin.surf.parent_yofs < editwin.surf.parent->height - editwin.surf.height) {
      memory_window_plain_block_refresh();
      editwin.surf.parent_yofs += editwin.surf.height;
      if (common_compute_tile_position(&editwin.surf) == SUCCEEDED) {
        editor_window_set_title();
        common_copy_data_to_child(&editwin.surf);
        editor_window_refresh();
        memory_window_block_refresh();
        tiled_window_refresh();
      }
    }
  }

  if (event->keyval == GDK_Control_L || event->keyval == GDK_Control_R) {
    editwin.editor_modkey |= MK_CONTROL;
  }

  if (event->keyval == GDK_Shift_L || event->keyval == GDK_Shift_R) {
    editwin.editor_modkey |= MK_SHIFT;
    /*
      if (editwin.tool_mode == TOOL_MODE_CIRCLE) {
      if ((editwin.editor_curr_x > -1 && editwin.editor_curr_y > -1) &&
      (editwin.editor_next_x > -1 && editwin.editor_next_y > -1)) {
      common_copy_data_to_view(&editwin.surf);
      editor_set_linedraw_color();

      common_putpixel_setmode(PUTPIX_NORMAL|PUTPIX_VIEW|PUTPIX_ZOOM);
      common_putcircle(editwin.editor_curr_x,editwin.editor_curr_y, abs(editwin.editor_curr_x-editwin.editor_next_x),abs(editwin.editor_curr_y-editwin.editor_next_y), editwin.linecolor, &editwin.surf, (editwin.editor_modkey&MK_CONTROL));
      gtk_widget_queue_draw(editwin.draw_area);
      }
      }
      else if (editwin.tool_mode == TOOL_MODE_RECT) {
      if ((editwin.editor_curr_x > -1 && editwin.editor_curr_y > -1) &&
      (editwin.editor_next_x > -1 && editwin.editor_next_y > -1)) {
      common_copy_data_to_view(&editwin.surf);
      editor_set_linedraw_color();
                                
      common_putpixel_setmode(PUTPIX_NORMAL|PUTPIX_VIEW|PUTPIX_ZOOM);
      common_putrect(editwin.editor_curr_x,editwin.editor_curr_y, editwin.editor_next_x,editwin.editor_next_y, editwin.linecolor, &editwin.surf, (editwin.editor_modkey&MK_CONTROL));
      gtk_widget_queue_draw(editwin.draw_area);
      }
      }
      else if ((editwin.tool_mode == TOOL_MODE_PEN) || (editwin.tool_mode == TOOL_MODE_LINE)) {
      if ((editwin.editor_curr_x > -1 && editwin.editor_curr_y > -1) &&
      (editwin.editor_next_x > -1 && editwin.editor_next_y > -1)) {
      common_copy_data_to_view(&editwin.surf);
      editor_set_linedraw_color();

      common_putpixel_setmode(PUTPIX_NORMAL|PUTPIX_VIEW|PUTPIX_ZOOM);
      if (editwin.editor_modkey&MK_CONTROL) {
      if (abs(editwin.editor_next_x - editwin.editor_curr_x) > abs(editwin.editor_next_y - editwin.editor_curr_y)) {
      common_putline(editwin.editor_curr_x,editwin.editor_curr_y, editwin.editor_next_x,editwin.editor_curr_y, editwin.linecolor, &editwin.surf);
      }
      else {
      common_putline(editwin.editor_curr_x,editwin.editor_curr_y, editwin.editor_curr_x,editwin.editor_next_y, editwin.linecolor, &editwin.surf);
      }
      }
      else
      common_putline(editwin.editor_curr_x,editwin.editor_curr_y, editwin.editor_next_x,editwin.editor_next_y, editwin.linecolor, &editwin.surf);
      gtk_widget_queue_draw(editwin.draw_area);
      }
      }
    */
  }

  return FALSE;
}


int editor_window_refresh(void) {

  if (editwin.surf.data == NULL || editwin.surf.view == NULL)
    return FAILED;

  common_copy_data_to_view(&editwin.surf);

  /* refresh the editor draw area */
  gtk_widget_queue_draw(editwin.draw_area);

  return SUCCEEDED;
}


static void editor_copy_to_undo(void) {

  undo_push(&editwin.surf, &editwin.undo_data);
}


static void editor_undo(GtkWidget *widget, gpointer data) {

  memory_window_plain_block_refresh();

  undo_pop(&editwin.surf, &editwin.undo_data);

  editor_window_set_title();
  common_copy_data_to_parent(&editwin.surf);
  common_copy_data_to_view(&editwin.surf);
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
  gtk_drawing_area_size(GTK_DRAWING_AREA(editwin.draw_area), editwin.surf.width*editwin.surf.zoom, editwin.surf.height*editwin.surf.zoom);
}


static void editor_clear(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_putpixel_setmode(PUTPIX_NORMAL);
  common_clear(&editwin.pen_bg, &editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_filters_blur_gaussian(GtkWidget *widget, gpointer data) {

  float filter[3*3] = { 1.0/36.0, 1.0/9.0, 1.0/36.0,
    1.0/ 9.0, 4.0/9.0, 1.0/ 9.0,
    1.0/36.0, 1.0/9.0, 1.0/36.0 };

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_editor_filter_3x3(filter, &editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_filters_blur_normal(GtkWidget *widget, gpointer data) {

  float filter[3*3] = { 1.0/9.0, 1.0/9.0, 1.0/9.0,
    1.0/9.0, 1.0/9.0, 1.0/9.0,
    1.0/9.0, 1.0/9.0, 1.0/9.0 };

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_editor_filter_3x3(filter, &editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_filters_negate(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_filters_negate(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_filters_build_alpha(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_filters_build_alpha(&editwin.pen_fg, &editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_mirror_lr(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_mirror_lr(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_mirror_tb(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_mirror_tb(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_copy(GtkWidget *widget, gpointer data) {

  if (editwin.editor_copy_dx != editwin.surf.width || editwin.editor_copy_dy != editwin.surf.height) {
    editwin.editor_copy_data = realloc(editwin.editor_copy_data, editwin.surf.width*editwin.surf.height*4);
    if (editwin.editor_copy_data == NULL) {
      fprintf(stderr, "EDITOR_COPY: Out of memory error.\n");
      return;
    }

    editwin.editor_copy_dx = editwin.surf.width;
    editwin.editor_copy_dy = editwin.surf.height;
  }

  /* copy the data to the copy buffer */
  memcpy(editwin.editor_copy_data, editwin.surf.data, editwin.surf.width*editwin.surf.height*4);
}


static void editor_paste(GtkWidget *widget, gpointer data) {

  int i;

  if (editwin.editor_copy_dx != editwin.surf.width || editwin.editor_copy_dy != editwin.surf.height)
    return;

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  /* copy the data from the copy buffer */
  if (editwin.surf.viewmode == VIEW_MODE_RGBA) {
    for (i = 0; i < editwin.surf.width*editwin.surf.height*4; i += 4) {
      editwin.surf.data[i + 0] = editwin.editor_copy_data[i + 0];
      editwin.surf.data[i + 1] = editwin.editor_copy_data[i + 1];
      editwin.surf.data[i + 2] = editwin.editor_copy_data[i + 2];
      editwin.surf.data[i + 3] = editwin.editor_copy_data[i + 3];
    }
  }
  else if (editwin.surf.viewmode == VIEW_MODE_RGB) {
    for (i = 0; i < editwin.surf.width*editwin.surf.height*4; i += 4) {
      editwin.surf.data[i + 0] = editwin.editor_copy_data[i + 0];
      editwin.surf.data[i + 1] = editwin.editor_copy_data[i + 1];
      editwin.surf.data[i + 2] = editwin.editor_copy_data[i + 2];
    }
  }
  else if (editwin.surf.viewmode == VIEW_MODE_ALPHA) {
    for (i = 0; i < editwin.surf.width*editwin.surf.height*4; i += 4) {
      editwin.surf.data[i + 0] = editwin.editor_copy_data[i + 3];
    }
  }

  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_rotate_right(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_rotate_right(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_rotate_left(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_rotate_left(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_mirror_and_blend_lin_x(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_mirror_and_blend_lin_x(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_mirror_and_blend_lin_y(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_mirror_and_blend_lin_y(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_mirror_and_blend_sto_x(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_mirror_and_blend_sto_x(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_mirror_and_blend_sto_y(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_mirror_and_blend_sto_y(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_erosion(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_erosion(&editwin.pen_fg, &editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_flip_x(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_flip_x(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_flip_y(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_flip_y(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_shift_left(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_shift_left(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_shift_right(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_shift_right(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_shift_up(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_shift_up(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_shift_down(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  common_shift_down(&editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_scale_n(GtkWidget *widget, gpointer data) {

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  /* scale! */
  common_scale((int)data, &editwin.surf);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static const char *editor_statusline(void)
{
  static char tmp[512];
  char buf[50];

  /* display the tile number only when snap-to-grid is enabled */
  if (memwin.snap_to_grid == YES)
    sprintf(tmp, "TILE: %d | %dx%d", editwin.surf.tilenum, editwin.surf.width, editwin.surf.height);
  else
    sprintf(tmp, "TILE: ? | %dx%d", editwin.surf.width, editwin.surf.height);

  if (editwin.surf.viewmode == VIEW_MODE_RGBA)
    strcat(tmp, " | RGBA");
  else if (editwin.surf.viewmode == VIEW_MODE_RGB)
    strcat(tmp, " | RGB");
  else if (editwin.surf.viewmode == VIEW_MODE_ALPHA)
    strcat(tmp, " | Alpha");

  if (editwin.tool_mode == TOOL_MODE_BLUR_GAUSSIAN)
    strcat(tmp, " | Blur (Gaussian)");
  else if (editwin.tool_mode == TOOL_MODE_BLUR_NORMAL)
    strcat(tmp, " | Blur (Normal)");
  /*
    else if ((editwin.tool_mode == TOOL_MODE_LINE) ||
    ((editwin.tool_mode == TOOL_MODE_PEN) && (editwin.editor_modkey&MK_SHIFT)))
    strcat(tmp, " | Line");
  */
  else if (editwin.tool_mode == TOOL_MODE_PEN_1X1)
    strcat(tmp, " | Pen 1x1");
  else if (editwin.tool_mode == TOOL_MODE_PEN_3X3)
    strcat(tmp, " | Pen 3x3");
  else if (editwin.tool_mode == TOOL_MODE_PEN_5X5)
    strcat(tmp, " | Pen 5x5");
  else if (editwin.tool_mode == TOOL_MODE_PEN_7X7)
    strcat(tmp, " | Pen 7x7");
  else if (editwin.tool_mode == TOOL_MODE_PICK)
    strcat(tmp, " | Pick");
  else if (editwin.tool_mode == TOOL_MODE_FILL)
    strcat(tmp, " | Fill");
  else if (editwin.tool_mode == TOOL_MODE_AGGRESSIVE_FILL)
    strcat(tmp, " | Aggressive fill");
  else if (editwin.tool_mode == TOOL_MODE_GRADIENT_FILL)
    strcat(tmp, " | Gradient fill");
  /*
    else if (editwin.tool_mode == TOOL_MODE_RECT)
    strcat(tmp, " | Rect");
    else if (editwin.tool_mode == TOOL_MODE_CIRCLE)
    strcat(tmp, " | Circle");
  */

  sprintf(buf, " | Zoom: %i%%", editwin.surf.zoom*100);
  strcat(tmp, buf);

  sprintf(buf, " | Undo: %i", editwin.undo_data.stack_depth);
  strcat(tmp, buf);

  return tmp;
}


int editor_window_set_title(void) {

  if (editwin.window == NULL)
    return FAILED;

  gtk_window_set_title(GTK_WINDOW(editwin.window), editor_statusline());

  return SUCCEEDED;
}


void editor_resize_view(int zoom) {

  editwin.surf.view = realloc(editwin.surf.view, editwin.surf.width*zoom*editwin.surf.height*zoom*3);
  if (editwin.surf.view == NULL) {
    fprintf(stderr, "EDITOR_RESIZE_VIEW: Out of memory error.\n");
    return;
  }

  editwin.surf.zoom = zoom;
}


void editor_resize_data(int dx, int dy) {

  editwin.surf.data = realloc(editwin.surf.data, dx*dy*4);
  if (editwin.surf.data == NULL) {
    fprintf(stderr, "EDITOR_RESIZE_DATA: Out of memory error.\n");
    return;
  }

  memset(editwin.surf.data, 255, dx*dy*4);

  editwin.surf.width = dx;
  editwin.surf.height = dy;
}


static void editor_zoom_n(GtkWidget *widget, gpointer data) {

  int zoom;


  zoom = (int)data;
  if (editwin.surf.zoom == zoom)
    return;

  editor_resize_view(zoom);
  common_copy_data_to_view(&editwin.surf);

  gtk_drawing_area_size(GTK_DRAWING_AREA(editwin.draw_area), editwin.surf.width*editwin.surf.zoom, editwin.surf.height*editwin.surf.zoom);
}


static void editor_zoom_change(GtkWidget *widget, gpointer data) {

  int zoom;


  zoom = editwin.surf.zoom + (int)data;
  if (zoom < 1) zoom = 1;
  else if (zoom > MAX_ZOOM_EDITOR) zoom = MAX_ZOOM_EDITOR;

  editor_resize_view(zoom);
  common_copy_data_to_view(&editwin.surf);

  gtk_drawing_area_size(GTK_DRAWING_AREA(editwin.draw_area), editwin.surf.width*editwin.surf.zoom, editwin.surf.height*editwin.surf.zoom);
}


static void editor_change_showgrid(GtkWidget *widget, gpointer data) {

  if (editwin.show_grid) editwin.show_grid = 0;
  else editwin.show_grid = 1;

  editor_window_refresh();
}


static void editor_size_n(GtkWidget *widget, gpointer data) {

  int i;

  i = (int)data;
  if (editwin.surf.width == i && editwin.surf.height == i)
    return;

  /* tile size cannot be larger than the memory size */
  if (editwin.surf.parent->width < i || editwin.surf.parent->height < i) {
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
    return;
  }

  editor_resize_data(i, i);
  editor_resize_view(editwin.surf.zoom);

  if (editwin.surf.parent_xofs + editwin.surf.width > editwin.surf.parent->width)
    editwin.surf.parent_xofs = editwin.surf.parent->width - editwin.surf.width;
  if (editwin.surf.parent_yofs + editwin.surf.height > editwin.surf.parent->height)
    editwin.surf.parent_yofs = editwin.surf.parent->height - editwin.surf.height;

  if (memwin.snap_to_grid == YES)
    common_compute_tile_position(&editwin.surf);

  common_copy_data_to_child(&editwin.surf);
  common_copy_data_to_view(&editwin.surf);
  memory_window_refresh();
  editor_window_set_title();
  tiled_window_refresh();

  gtk_drawing_area_size(GTK_DRAWING_AREA(editwin.draw_area), editwin.surf.width*editwin.surf.zoom, editwin.surf.height*editwin.surf.zoom);
}


static void editor_view_n(GtkWidget *widget, gpointer data) {

  int i;

  i = (int)data;
  if (editwin.surf.viewmode == i)
    return;

  editwin.surf.viewmode = i;
  editor_window_refresh();
  editor_window_set_title();
}


static void editor_save(GtkWidget *widget, gpointer data) {

  if (editwin.file_name_status == NO) {
    editor_save_as(widget, data);
    return;
  }

  png_save(editwin.file_name, editwin.surf.width, editwin.surf.height, editwin.surf.data);
}


static void editor_save_as(GtkWidget *widget, gpointer data) {

  gtk_widget_show(editwin.file_selection_save);
}


static void editor_open(GtkWidget *widget, gpointer data) {

  gtk_widget_show(editwin.file_selection_open);
}


static void editor_file_save_ok(GtkWidget *widget, gpointer data) {

  char tmp[512];
  gchar *n;

  n = (gchar *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(editwin.file_selection_save));
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(editwin.file_selection_open), n);

  /* hide the file selection window */
  gtk_widget_hide(editwin.file_selection_save);

  png_save(n, editwin.surf.width, editwin.surf.height, editwin.surf.data);

  strcpy(editwin.file_name, n);
  /*  editwin.editor_file_status = YES;*/
  editwin.file_name_status = YES;

  /* strip the filename from the whole path */
  common_get_path(n, tmp);
  strcpy(editwin.file_path, tmp);
}


static void editor_file_open_ok(GtkWidget *widget, gpointer data) {

  unsigned char *d;
  int dx, dy, bpp, i, l;
  gchar *n, tmp[512];

  /* backup the old data */
  editor_copy_to_undo();

  memory_title_reset(YES);

  n = (gchar *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(editwin.file_selection_open));

  /* hide the file selection window */
  gtk_widget_hide(editwin.file_selection_open);

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

  strcpy(editwin.file_name, n);
  /*  editwin.editor_file_status = YES;*/

  /* strip the filename from the whole path */
  common_get_path(n, tmp);
  strcpy(editwin.file_path, tmp);

  /* the filename can be used later only if the file was a png file */
  if (l == 1) {
    editwin.file_name_status = YES;

    /* set the whole filename with path to the "save as" file selector */
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(editwin.file_selection_save), n);
  }
  else {
    editwin.file_name_status = NO;

    /* set the path to the "save as" file selector */
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(editwin.file_selection_save), tmp);
  }

  /* copy it, or part of it, to the editor */
  common_copy_image(editwin.surf.data, editwin.surf.width, editwin.surf.height, d, dx, dy);
  free(d);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* refresh the draw areas */
  editor_window_refresh();
  memory_window_block_refresh();
  tiled_window_refresh();
}


static void editor_file_save_cancel(GtkWidget *widget, gpointer data) {

  gtk_widget_hide(editwin.file_selection_save);
}


static void editor_file_open_cancel(GtkWidget *widget, gpointer data) {

  gtk_widget_hide(editwin.file_selection_open);
}

static void editor_draw_pick(int x, int y) {

  if (editwin.mouse_button_mode == BG) {
    common_getpixel(x,y, &editwin.pen_bg, &editwin.surf);
  }
  else {
    common_getpixel(x,y, &editwin.pen_fg, &editwin.surf);
  }

  palette_window_refresh();
}


static void editor_draw_pen(int x, int y) {

  memory_title_reset(YES);

  common_putpixel_setmode(PUTPIX_NORMAL);

  if (editwin.mouse_button_mode == BG) {
    common_putpixel(x,y, &editwin.pen_bg, &editwin.surf);
  }
  else {
    common_putpixel(x,y, &editwin.pen_fg, &editwin.surf);
  }
}


static void editor_draw_blur_normal(unsigned int x, unsigned int y) {

  memory_title_reset(YES);

  common_draw_blur_normal(x,y, &editwin.surf);
}


static void editor_draw_blur_gaussian(unsigned int x, unsigned int y) {

  memory_title_reset(YES);

  common_draw_blur_gaussian(x,y, &editwin.surf);
}

static void editor_draw_cursor(int x, int y, GtkWidget *widget) {

  /* backup the old pixel */
  struct td_color col;

  common_getpixel(x, y, &col, &editwin.surf);

  common_putpixel_setmode(PUTPIX_NORMAL);

  /* highlight the pixel */
  common_putpixel(x,y, &editwin.pen_fg, &editwin.surf);

  /* display it */
  common_copy_pixel_to_view(x, y, &editwin.surf);

  /* return the old value */
  common_putpixel(x,y, &col, &editwin.surf);

  editor_draw_grid();

  /* refresh the editor draw area */
  editor_gdk_refresh_rgbimage(x,y, 1,1, &editwin.surf, widget);
}


static int editor_need_to_draw(int x, int y) {

  struct td_color c, d;

  if (y < 0 || y >= editwin.surf.height || x < 0 || x >= editwin.surf.width)
    return NO;

  if (editwin.mouse_button_mode == BG)
    d = editwin.pen_bg;
  else
    d = editwin.pen_fg;

  c = d;

  common_getpixel(x, y, &c, &editwin.surf);

  if (c.r == d.r && c.g == d.g && c.b == d.b && c.a == d.a)
    return NO;

  return YES;
}


/* old mouse x and y */
/*static int mouse_old_x = -1, mouse_old_y = -1;*/


static gint editor_button_press(GtkWidget *widget, GdkEventButton *event) {

  int x, y, i, j;

  editwin.mouse_button = ON;

  if (event->button == 3)
    editwin.mouse_button_mode = BG;
  else
    editwin.mouse_button_mode = FG;

  x = event->x/editwin.surf.zoom;
  y = event->y/editwin.surf.zoom;

  if (y < 0 || y >= editwin.surf.height || x < 0 || x >= editwin.surf.height)
    return FALSE;

  editwin.mouse_old_x = x;
  editwin.mouse_old_y = y;

  editwin.editor_prev_x = editwin.editor_curr_x;
  editwin.editor_prev_y = editwin.editor_curr_y;

  editwin.editor_curr_x = x;
  editwin.editor_curr_y = y;

  if (editwin.tool_mode == TOOL_MODE_FILL || editwin.tool_mode == TOOL_MODE_AGGRESSIVE_FILL || editwin.tool_mode == TOOL_MODE_GRADIENT_FILL) {
    if (editor_need_to_draw(x, y) == NO) {
      /* refresh the pointer */
      editor_draw_cursor(x, y, widget);

      return FALSE;
    }

    /* backup the old data */
    editor_copy_to_undo();

    memory_title_reset(YES);

    if (editwin.tool_mode == TOOL_MODE_AGGRESSIVE_FILL) {
      if (editwin.mouse_button_mode == BG)
        common_draw_aggressive_fill(x, y, &editwin.pen_bg, &editwin.surf);
      else
        common_draw_aggressive_fill(x, y, &editwin.pen_fg, &editwin.surf);
    }
    else if (editwin.tool_mode == TOOL_MODE_GRADIENT_FILL) {
      if (editwin.mouse_button_mode == BG)
        common_draw_gradient_fill(x, y, &editwin.pen_bg, &editwin.surf);
      else
        common_draw_gradient_fill(x, y, &editwin.pen_fg, &editwin.surf);
    }
    else {
      if (editwin.mouse_button_mode == BG)
        common_draw_fill(x, y, &editwin.pen_bg, &editwin.surf);
      else
        common_draw_fill(x, y, &editwin.pen_fg, &editwin.surf);
    }

    /* copy to memory */
    common_copy_data_to_parent(&editwin.surf);

    /* refresh the draw areas */
    editor_window_refresh();
    memory_window_block_refresh();
    tiled_window_refresh();

    /* refresh the pointer */
    editor_draw_cursor(x, y, widget);
  }
  else if (editwin.tool_mode == TOOL_MODE_BLUR_GAUSSIAN || editwin.tool_mode == TOOL_MODE_BLUR_NORMAL) {
    /* backup the old data */
    editor_copy_to_undo();

    if (editwin.tool_mode == TOOL_MODE_BLUR_GAUSSIAN)
      editor_draw_blur_gaussian((unsigned int)x, (unsigned int)y);
    else
      editor_draw_blur_normal((unsigned int)x, (unsigned int)y);

    /* refresh the editor draw area */
    common_copy_pixel_to_view(x, y, &editwin.surf);

    /* update the other windows, too, if required */
    if (tilwin.real_time == YES)
      tiled_window_refresh();
    if (memwin.real_time == YES) {
      /* copy to memory */
      common_copy_data_to_parent(&editwin.surf);

      memory_window_block_refresh();
    }

    gtk_widget_queue_draw(editwin.draw_area);
  }
  /*
    else if (editwin.tool_mode == TOOL_MODE_CIRCLE) {
    if ((editwin.editor_prev_x > -1 && editwin.editor_prev_y > -1) &&
    (editwin.editor_curr_x > -1 && editwin.editor_curr_y > -1) && (editwin.editor_modkey&MK_SHIFT)) {
    editor_copy_to_undo();
    editor_set_linedraw_color();
    common_putpixel_setmode(PUTPIX_NORMAL);
    common_putcircle(editwin.editor_prev_x,editwin.editor_prev_y, abs(editwin.editor_prev_x-editwin.editor_curr_x),abs(editwin.editor_prev_y-editwin.editor_curr_y), editwin.linecolor, &editwin.surf, (editwin.editor_modkey&MK_CONTROL));
    common_copy_data_to_view(&editwin.surf);
    editor_draw_grid();
    editor_gdk_refresh_rgbimage(0,0, 0,0, &editwin.surf, widget);
    return FALSE;
    }
    }
    else if (editwin.tool_mode == TOOL_MODE_RECT) {
    if ((editwin.editor_prev_x > -1 && editwin.editor_prev_y > -1) &&
    (editwin.editor_curr_x > -1 && editwin.editor_curr_y > -1) && (editwin.editor_modkey&MK_SHIFT)) {
    editor_copy_to_undo();
    editor_set_linedraw_color();
    common_putpixel_setmode(PUTPIX_NORMAL);
    common_putrect(editwin.editor_prev_x,editwin.editor_prev_y, editwin.editor_curr_x,editwin.editor_curr_y, editwin.linecolor, &editwin.surf, (editwin.editor_modkey&MK_CONTROL));
    common_copy_data_to_view(&editwin.surf);
    editor_gdk_refresh_rgbimage(0,0, 0,0, &editwin.surf, widget);
    return FALSE;
    }
    }
    else if ((editwin.tool_mode == TOOL_MODE_LINE) ||
    ((editwin.tool_mode == TOOL_MODE_PEN) && (editwin.editor_modkey&MK_SHIFT))
    ) {
    if ((editwin.editor_prev_x > -1 && editwin.editor_prev_y > -1) &&
    (editwin.editor_curr_x > -1 && editwin.editor_curr_y > -1)) {
    editor_copy_to_undo();
    editor_set_linedraw_color();
    common_putpixel_setmode(PUTPIX_NORMAL);
    if ((editwin.editor_modkey&MK_CONTROL)) {
    if (abs(editwin.editor_prev_x - editwin.editor_curr_x) > abs(editwin.editor_prev_y - editwin.editor_curr_y)) {
    common_putline(editwin.editor_prev_x,editwin.editor_prev_y, editwin.editor_curr_x,editwin.editor_prev_y, editwin.linecolor, &editwin.surf);
    } else {
    common_putline(editwin.editor_prev_x,editwin.editor_prev_y, editwin.editor_prev_x,editwin.editor_curr_y, editwin.linecolor, &editwin.surf);
    }
    } else
    common_putline(editwin.editor_prev_x,editwin.editor_prev_y, editwin.editor_curr_x,editwin.editor_curr_y, editwin.linecolor, &editwin.surf);
    common_copy_data_to_view(&editwin.surf);
    editor_gdk_refresh_rgbimage(0,0, 0,0, &editwin.surf, widget);
    return FALSE;
    }
    }
  */
  else if (editwin.tool_mode == TOOL_MODE_PEN_1X1) {
    /* backup the old data */
    common_putpixel_setmode(PUTPIX_NORMAL);

    if (editor_need_to_draw(x, y) == NO) {
      /* update the previous pixel */
      common_copy_pixel_to_view(editwin.mouse_old_x, editwin.mouse_old_y, &editwin.surf);
      editor_draw_grid();

      editor_gdk_refresh_rgbimage(editwin.mouse_old_x,editwin.mouse_old_y, 1,1, &editwin.surf, widget);

      return FALSE;
    }

    editor_copy_to_undo();

    editor_draw_pen(x, y);

    /* refresh the editor draw area */
    common_copy_pixel_to_view(x, y, &editwin.surf);

    /* update the other windows, too, if required */
    if (tilwin.real_time == YES)
      tiled_window_refresh();
    if (memwin.real_time == YES) {
      /* copy to memory */
      common_copy_data_to_parent(&editwin.surf);

      memory_window_block_refresh();
    }

    gtk_widget_queue_draw(editwin.draw_area);
  }
  else if (editwin.tool_mode == TOOL_MODE_PEN_3X3) {
    editor_copy_to_undo();
    editor_set_linedraw_color();
    common_putpixel_setmode(PUTPIX_NORMAL);
    for (i = -1; i < 2; i++) {
      for (j = -1; j < 2; j++) {
        if (editor_need_to_draw(x + j, y + i) == YES)
          editor_draw_pen(x + j, y + i);
      }
    }
    common_copy_data_to_view(&editwin.surf);
    editor_gdk_refresh_rgbimage(0,0, 0,0, &editwin.surf, widget);
  }
  else if (editwin.tool_mode == TOOL_MODE_PEN_5X5) {
    editor_copy_to_undo();
    editor_set_linedraw_color();
    common_putpixel_setmode(PUTPIX_NORMAL);
    for (i = -2; i < 3; i++) {
      for (j = -2; j < 3; j++) {
        if (editor_need_to_draw(x + j, y + i) == YES)
          editor_draw_pen(x + j, y + i);
      }
    }
    common_copy_data_to_view(&editwin.surf);
    editor_gdk_refresh_rgbimage(0,0, 0,0, &editwin.surf, widget);
  }
  else if (editwin.tool_mode == TOOL_MODE_PEN_7X7) {
    editor_copy_to_undo();
    editor_set_linedraw_color();
    common_putpixel_setmode(PUTPIX_NORMAL);
    for (i = -3; i < 4; i++) {
      for (j = -3; j < 4; j++) {
        if (editor_need_to_draw(x + j, y + i) == YES)
          editor_draw_pen(x + j, y + i);
      }
    }
    common_copy_data_to_view(&editwin.surf);
    editor_gdk_refresh_rgbimage(0,0, 0,0, &editwin.surf, widget);
  }
  else if (editwin.tool_mode == TOOL_MODE_PICK) {
    editor_draw_pick(x, y);

    /* refresh the pointer */
    editor_draw_cursor(x, y, widget);
  }

  return FALSE;
}


static gint editor_button_release(GtkWidget *widget, GdkEventButton *event) {

  editwin.mouse_button = OFF;

  if (editwin.mouse_old_x >= 0 && editwin.mouse_old_y >= 0)
    editor_draw_cursor(editwin.mouse_old_x, editwin.mouse_old_y, widget);

  /* copy to memory */
  common_copy_data_to_parent(&editwin.surf);

  /* update the other windows only after the mouse button has been released */
  memory_window_block_refresh();
  tiled_window_refresh();

  editwin.editor_prev_x = editwin.editor_curr_x;
  editwin.editor_prev_y = editwin.editor_curr_y;

  editwin.editor_curr_x = event->x/editwin.surf.zoom;
  editwin.editor_curr_y = event->y/editwin.surf.zoom;

  return FALSE;
}


static gint editor_motion_notify(GtkWidget *widget, GdkEventButton *event) {

  int x, y, i, j;

  x = event->x/editwin.surf.zoom;
  y = event->y/editwin.surf.zoom;

  editwin.editor_next_x = x;
  editwin.editor_next_y = y;

  if (y < 0 || y >= editwin.surf.height || x < 0 || x >= editwin.surf.width)
    return FALSE;

  /* do we need to update the pixel? */
  if (editwin.mouse_old_x == x && editwin.mouse_old_y == y)
    return FALSE;

  if (editwin.mouse_button == OFF) {
    /* no button pressed -> highlight the pixel under the pointer */
    /*
      if ((editwin.tool_mode == TOOL_MODE_CIRCLE) && (editwin.editor_modkey&MK_SHIFT) &&
      (editwin.editor_curr_x > -1 && editwin.editor_curr_y > -1)) {
      common_copy_data_to_view(&editwin.surf);
      editor_set_linedraw_color();
      common_putpixel_setmode(PUTPIX_NORMAL|PUTPIX_VIEW|PUTPIX_ZOOM);
      common_putcircle(editwin.editor_curr_x,editwin.editor_curr_y, abs(editwin.editor_curr_x-x),abs(editwin.editor_curr_y-y), editwin.linecolor, &editwin.surf, (editwin.editor_modkey&MK_CONTROL));
      editor_draw_grid();
      editor_gdk_refresh_rgbimage(0,0, 0,0, &editwin.surf, widget);
      }
      else if ((editwin.tool_mode == TOOL_MODE_RECT) && (editwin.editor_modkey&MK_SHIFT) &&
      (editwin.editor_curr_x > -1 && editwin.editor_curr_y > -1)) {
      common_copy_data_to_view(&editwin.surf);
      editor_set_linedraw_color();
      common_putpixel_setmode(PUTPIX_NORMAL|PUTPIX_VIEW|PUTPIX_ZOOM);
      common_putrect(editwin.editor_curr_x,editwin.editor_curr_y, x,y, editwin.linecolor, &editwin.surf, (editwin.editor_modkey&MK_CONTROL));
      editor_draw_grid();
      editor_gdk_refresh_rgbimage(0,0, 0,0, &editwin.surf, widget);
      }
      else if ((((editwin.tool_mode == TOOL_MODE_PEN) && (editwin.editor_modkey&MK_SHIFT)) ||
      (editwin.tool_mode == TOOL_MODE_LINE)) &&
      (editwin.editor_curr_x > -1 && editwin.editor_curr_y > -1)) {
      common_copy_data_to_view(&editwin.surf);
      editor_set_linedraw_color();
      common_putpixel_setmode(PUTPIX_NORMAL|PUTPIX_VIEW|PUTPIX_ZOOM);
      if ((editwin.editor_modkey&MK_CONTROL)) {
      if (abs(editwin.editor_next_x - editwin.editor_curr_x) > abs(editwin.editor_next_y - editwin.editor_curr_y)) {
      common_putline(editwin.editor_curr_x,editwin.editor_curr_y, editwin.editor_next_x,editwin.editor_curr_y, editwin.linecolor, &editwin.surf);
      }
      else {
      common_putline(editwin.editor_curr_x,editwin.editor_curr_y, editwin.editor_curr_x,editwin.editor_next_y, editwin.linecolor, &editwin.surf);
      }
      }
      else
      common_putline(editwin.editor_curr_x,editwin.editor_curr_y, x,y, editwin.linecolor, &editwin.surf);
      editor_draw_grid();
      editor_gdk_refresh_rgbimage(0,0, 0,0, &editwin.surf, widget);
      }
    */
    if (0)
      ;
    else {
      /* update the previous pixel */
      if (editwin.mouse_old_x >= 0 && editwin.mouse_old_y >= 0) {
        common_copy_pixel_to_view(editwin.mouse_old_x, editwin.mouse_old_y, &editwin.surf);
        editor_draw_grid();
                                
        editor_gdk_refresh_rgbimage(editwin.mouse_old_x,editwin.mouse_old_y, 1,1, &editwin.surf, widget);
      }
                        
      editor_draw_cursor(x, y, widget);
                        
    }
    editwin.mouse_old_x = x;
    editwin.mouse_old_y = y;

    return FALSE;
  }

  if (editwin.tool_mode == TOOL_MODE_PEN_1X1) {
    /* do we need to update the pixel? */
    if (editor_need_to_draw(x, y) == YES) {
      editor_draw_pen(x, y);

      /* refresh the editor draw area */
      common_copy_pixel_to_view(x, y, &editwin.surf);
      editor_draw_grid();

      editor_gdk_refresh_rgbimage(x, y, 1, 1, &editwin.surf, widget);

      /* update the other windows, too, if required */
      if (tilwin.real_time == YES)
        tiled_window_refresh();
      if (memwin.real_time == YES) {
        /* copy to memory */
        common_copy_data_to_parent(&editwin.surf);

        memory_window_block_refresh();
      }
    }
  }
  else if (editwin.tool_mode == TOOL_MODE_PEN_3X3) {
    for (i = -1; i < 2; i++) {
      for (j = -1; j < 2; j++) {
        if (editor_need_to_draw(x + j, y + i) == YES) {
          editor_draw_pen(x + j, y + i);
          /* refresh the editor draw area */
          common_copy_pixel_to_view(x + j, y + i, &editwin.surf);
        }
      }
    }

    editor_draw_grid();

    editor_gdk_refresh_rgbimage(x - 1, y - 1, 3, 3, &editwin.surf, widget);

    /* update the other windows, too, if required */
    if (tilwin.real_time == YES)
      tiled_window_refresh();
    if (memwin.real_time == YES) {
      /* copy to memory */
      common_copy_data_to_parent(&editwin.surf);

      memory_window_block_refresh();
    }
  }
  else if (editwin.tool_mode == TOOL_MODE_PEN_5X5) {
    for (i = -2; i < 3; i++) {
      for (j = -2; j < 3; j++) {
        if (editor_need_to_draw(x + j, y + i) == YES) {
          editor_draw_pen(x + j, y + i);
          /* refresh the editor draw area */
          common_copy_pixel_to_view(x + j, y + i, &editwin.surf);
        }
      }
    }

    editor_draw_grid();

    editor_gdk_refresh_rgbimage(x - 2, y - 2, 5, 5, &editwin.surf, widget);

    /* update the other windows, too, if required */
    if (tilwin.real_time == YES)
      tiled_window_refresh();
    if (memwin.real_time == YES) {
      /* copy to memory */
      common_copy_data_to_parent(&editwin.surf);

      memory_window_block_refresh();
    }
  }
  else if (editwin.tool_mode == TOOL_MODE_PEN_7X7) {
    for (i = -3; i < 4; i++) {
      for (j = -3; j < 4; j++) {
        if (editor_need_to_draw(x + j, y + i) == YES) {
          editor_draw_pen(x + j, y + i);
          /* refresh the editor draw area */
          common_copy_pixel_to_view(x + j, y + i, &editwin.surf);
        }
      }
    }

    editor_draw_grid();

    editor_gdk_refresh_rgbimage(x - 3, y - 3, 7, 7, &editwin.surf, widget);

    /* update the other windows, too, if required */
    if (tilwin.real_time == YES)
      tiled_window_refresh();
    if (memwin.real_time == YES) {
      /* copy to memory */
      common_copy_data_to_parent(&editwin.surf);

      memory_window_block_refresh();
    }
  }
  else if (editwin.tool_mode == TOOL_MODE_BLUR_GAUSSIAN || editwin.tool_mode == TOOL_MODE_BLUR_NORMAL) {
    if (editwin.tool_mode == TOOL_MODE_BLUR_GAUSSIAN)
      editor_draw_blur_gaussian((unsigned int)x, (unsigned int)y);
    else
      editor_draw_blur_normal((unsigned int)x, (unsigned int)y);

    /* refresh the editor draw area */
    common_copy_pixel_to_view(x, y, &editwin.surf);
    editor_draw_grid();

    editor_gdk_refresh_rgbimage(x,y, 1,1, &editwin.surf, widget);

    /* update the other windows, too, if required */
    if (tilwin.real_time == YES)
      tiled_window_refresh();
    if (memwin.real_time == YES) {
      /* copy to memory */
      common_copy_data_to_parent(&editwin.surf);

      memory_window_block_refresh();
    }
  }
  else if (editwin.tool_mode == TOOL_MODE_PICK) {
    editor_draw_pick(x, y);

    /* update the previous pixel */
    if (editwin.mouse_old_x >= 0 && editwin.mouse_old_y >= 0) {
      common_copy_pixel_to_view(editwin.mouse_old_x, editwin.mouse_old_y, &editwin.surf);
      editor_draw_grid();

      editor_gdk_refresh_rgbimage(editwin.mouse_old_x,editwin.mouse_old_y, 1,1, &editwin.surf, widget);
    }

    /* refresh the pointer */
    editor_draw_cursor(x, y, widget);
  }
  else if (editwin.tool_mode == TOOL_MODE_FILL || editwin.tool_mode == TOOL_MODE_AGGRESSIVE_FILL || editwin.tool_mode == TOOL_MODE_GRADIENT_FILL) {
    /* update the previous pixel */
    if (editwin.mouse_old_x >= 0 && editwin.mouse_old_y >= 0) {
      common_copy_pixel_to_view(editwin.mouse_old_x, editwin.mouse_old_y, &editwin.surf);
      editor_draw_grid();

      editor_gdk_refresh_rgbimage(editwin.mouse_old_x,editwin.mouse_old_y, 1,1, &editwin.surf, widget);
    }

    /* refresh the pointer */
    editor_draw_cursor(x, y, widget);
  }

  editwin.mouse_old_x = x;
  editwin.mouse_old_y = y;

  return FALSE;
}


static gint editor_leave_notify(GtkWidget *widget, GdkEventButton *event) {

  if (editwin.surf.view == NULL)
    return FALSE;

  /* update the previous pixel */
  if (editwin.mouse_old_x >= 0 && editwin.mouse_old_y >= 0) {
    common_copy_pixel_to_view(editwin.mouse_old_x, editwin.mouse_old_y, &editwin.surf);
    editor_draw_grid();

    editor_gdk_refresh_rgbimage(editwin.mouse_old_x,editwin.mouse_old_y, 1,1, &editwin.surf, widget);
  }

  editwin.mouse_old_x = -1;
  editwin.mouse_old_y = -1;

  /*
    if (((editwin.tool_mode == TOOL_MODE_CIRCLE) ||
    (editwin.tool_mode == TOOL_MODE_RECT) ||
    (editwin.tool_mode == TOOL_MODE_LINE) ||
    (editwin.editor_modkey&MK_SHIFT)) &&
    (editwin.editor_next_x > -1 && editwin.editor_next_y > -1) &&
    (editwin.editor_curr_x > -1 && editwin.editor_curr_y > -1)) {
    common_copy_data_to_view(&editwin.surf);
    editor_draw_grid();
    editor_gdk_refresh_rgbimage(0,0, 0,0, &editwin.surf, widget);
    }
  */

  editwin.editor_next_x = -1;
  editwin.editor_next_y = -1;

  return FALSE;
}


static void editor_tools_n(GtkWidget *widget, gpointer data) {

  editwin.tool_mode = (int)data;

  editor_window_set_title();
  editor_window_refresh();
}


void editor_draw_grid() {
  
  if (editwin.show_grid) {
    int x, y;
    int startx = ((editwin.surf.parent_xofs+editwin.surf.width) % memwin.grid_x);
    int starty = ((editwin.surf.parent_yofs+editwin.surf.height) % memwin.grid_y);

    if (startx < 1) startx = memwin.grid_x;
    if (starty < 1) starty = memwin.grid_y;

    if (startx < editwin.surf.width)
      for (x = startx; x < editwin.surf.width; x += memwin.grid_x) {
        common_line_dotted_vert((editwin.surf.width-x) * editwin.surf.zoom, &editwin.surf);
      }

    if (starty < editwin.surf.height)
      for (y = starty; y < editwin.surf.height; y += memwin.grid_y) {
        common_line_dotted_horiz((editwin.surf.height-y) * editwin.surf.zoom, &editwin.surf);
      }
  }
}


static gboolean editor_draw_area_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {

  if (editwin.surf.view != NULL) {
    gtk_label_set_text(GTK_LABEL(editwin.statuslabel), editor_statusline());

    editor_draw_grid();
    editor_gdk_refresh_rgbimage(0,0, 0,0, &editwin.surf, widget);
  }

  return FALSE;
}
