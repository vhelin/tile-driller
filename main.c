
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef WIN32
/*
  this needs to be before defines.h, since winerr.h defines
  SUCCEEDED and FAILED macros.
*/
#include <windows.h>
#undef SUCCEEDED
#undef FAILED
#endif

#include <gtk/gtk.h>

#include "defines.h"
#include "memory.h"
#include "editor.h"
#include "palette.h"
#include "tiled.h"
#include "prefs.h"
#include "common.h"
#include "exit.h"


#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
int main(int argc, char *argv[]) {
#endif

  srand(1);

  /* init GTK */
  gtk_set_locale();

#ifdef WIN32
  gtk_init(&__argc, &__argv);
#else
  gtk_init(&argc, &argv);
#endif

  memory_struct_init();
  editor_struct_init();
  tiled_struct_init();
  palette_struct_init();

  /* read preferences */
  prefs_read();

  /* init the windows */
  memory_window_init();
  editor_window_init();
  tiled_window_init();
  palette_window_init();
  exit_dialog_init();

  /* apply preferences, and show the windows */
  prefs_apply();

  editwin.need_to_save = NO;

  /* GTK main loop */
  gtk_main();

  return 0;
}
