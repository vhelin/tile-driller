
#ifndef PALETTE_H
#define PALETTE_H

struct pal_window {
  GtkWidget *window;
  GtkWidget *vbox1;
  GtkWidget *mode_hbox1;
  GtkWidget *selection;
  GtkWidget *fg_button;
  GtkWidget *bg_button;
  GSList *rgroup;

  int status;

  int select_mode;
};

extern struct pal_window palwin;

/* functions */
void palette_struct_init(void);
int palette_window_init(void);
int palette_window_refresh(void);

#endif
