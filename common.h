
#ifndef COMMON_H
#define COMMON_H

#define PUTPIX_NORMAL	0
#define PUTPIX_NEGATE	1
#define PUTPIX_VIEW	128
#define PUTPIX_ZOOM	256

struct td_color
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};

struct td_surf
{
  int width, height;	/* size of the canvas */
  int zoom;		/* zoom, only used for view */
  int viewmode;
  int tilenum;

  unsigned char *data;	/* actual canvas data */
  unsigned char *view;	/* how it looks on screen */

  struct td_surf *parent;
  int parent_xofs, parent_yofs;

  struct td_surf *child;
};

/* undo */
struct _undo_stack {
  struct _undo_stack *next;
  unsigned char *data;
  int width, height;
  int memory_x, memory_y;
};

struct _undo {
  struct _undo_stack *stack;
  int stack_depth, max_stack_depth;
};

/* variables */
extern char version_string[];
extern unsigned char background_r, background_g, background_b;

/* functions */
void undo_init(int maxdepth, struct _undo *undo);
void undo_clear(struct _undo *undo);
void undo_prune(struct _undo *undo);
void undo_push(struct td_surf *surface, struct _undo *undo);
int  undo_pop(struct td_surf *surface, struct _undo *undo);

void common_putpixel_setmode(int putpix);

void common_getpixel(int x, int y, struct td_color *c, struct td_surf *surface);
void common_putpixel(int x, int y, struct td_color *c, struct td_surf *surface);

void common_putline(int x, int y, int x2, int y2, struct td_color *col, struct td_surf *surface);
void common_putrect(int x, int y, int x2, int y2, struct td_color *col, struct td_surf *surface, int filled);
void common_putcircle(int center_x, int center_y, int rx, int ry, struct td_color *col, struct td_surf *surface, int filled);

void common_line_dotted_horiz(int y, struct td_surf *out);
void common_line_dotted_vert(int x, struct td_surf *out);
void common_draw_fill(int x, int y, struct td_color *col, struct td_surf *surface);
void common_draw_aggressive_fill(int x, int y, struct td_color *col, struct td_surf *surface);
void common_draw_gradient_fill(int x, int y, struct td_color *col, struct td_surf *surface);
void common_clear(struct td_color *col, struct td_surf *surface);
void common_draw_blur_gaussian(unsigned int x, unsigned int y, struct td_surf *surface);
void common_draw_blur_normal(unsigned int x, unsigned int y, struct td_surf *surface);
void common_shift_down(struct td_surf *surface);
void common_shift_up(struct td_surf *surface);
void common_shift_right(struct td_surf *surface);
void common_shift_left(struct td_surf *surface);
void common_flip_y(struct td_surf *surface);
void common_flip_x(struct td_surf *surface);
void common_erosion(struct td_color *col, struct td_surf *surface);
void common_mirror_and_blend_sto_y(struct td_surf *surface);
void common_mirror_and_blend_sto_x(struct td_surf *surface);
void common_mirror_and_blend_lin_y(struct td_surf *surface);
void common_mirror_and_blend_lin_x(struct td_surf *surface);
void common_rotate_left(struct td_surf *surface);
void common_rotate_right(struct td_surf *surface);
void common_mirror_tb(struct td_surf *surface);
void common_mirror_lr(struct td_surf *surface);
void common_filters_build_alpha(struct td_color *col, struct td_surf *surface);
void common_filters_negate(struct td_surf *surface);
int  common_editor_filter_3x3(float *filter, struct td_surf *surface);

int  common_set_tile_position(int mx, int my, struct td_surf *surface);
int  common_compute_tile_position(struct td_surf *surface);

int common_copy_data_to_child(struct td_surf *surface);
int common_copy_data_to_parent(struct td_surf *surface);

void common_copy_data_to_view(struct td_surf *surface);
void common_copy_pixel_to_view(int x, int y, struct td_surf *surface);
void common_copy_block_to_view(int ax, int ay, int mx, int my, struct td_surf *surface);

void common_gtk_exit(GtkWidget *widget, gpointer data);
gint common_gtk_hide_widget(GtkWidget *widget, gpointer data);

int common_reformat_image(int *dx, int *dy, int *bpp, unsigned char **d);
int common_copy_image(unsigned char *out, int ox, int oy, unsigned char *in, int ix, int iy);
int common_scale(int i, struct td_surf *surface);
void common_resize_data(int dx, int dy, struct td_color *col, struct td_surf *surface);

int common_get_path(char *filename, char *path);
#endif
