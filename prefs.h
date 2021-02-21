
#ifndef PREFS_H
#define PREFS_H

extern int tiled_window_x, tiled_window_y;
extern int palette_window_x, palette_window_y;

int prefs_read(void);
int prefs_save(void);
int prefs_apply(void);

#endif

