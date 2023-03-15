
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include "defines.h"
#include "memory.h"
#include "editor.h"
#include "prefs.h"
#include "common.h"
#include "exit.h"


/* version string */
char version_string[] = "Tile Driller v2.2";


void undo_init(int maxdepth, struct _undo *undo) {
  
  if (!undo)
    return;

  undo->max_stack_depth = maxdepth;
  undo->stack_depth = 0;
  undo->stack = NULL;
}


void undo_clear(struct _undo *undo) {
  
  if (!undo)
    return;
  else {
    struct _undo_stack *tmp = undo->stack, *nxt;

    while (tmp) {
      nxt = tmp->next;
      free(tmp->data);
      free(tmp);
      tmp = nxt;
    }
    undo->stack = NULL;
    undo->stack_depth = 0;
  }
}


void undo_prune(struct _undo *undo) {
  
  if (!undo)
    return;

  if (undo->stack_depth > undo->max_stack_depth) {
    int depth = undo->max_stack_depth;
    struct _undo_stack *tmp = undo->stack, *r;

    if (tmp) {
      while (tmp->next && --depth)
        tmp = tmp->next;
    }

    r = tmp;
    tmp = tmp->next;
    r->next = NULL;

    while (tmp) {
      r = tmp->next;
      free(tmp->data);
      free(tmp);
      undo->stack_depth--;
      tmp = r;
    }
  }
}


void undo_push(struct td_surf *surface, struct _undo *undo) {
  
  struct _undo_stack *tmp = (struct _undo_stack *)malloc(sizeof(struct _undo_stack));

  if (tmp) {
    tmp->next = undo->stack;

    tmp->data = (unsigned char *)malloc(surface->width * surface->height * 4);
    if (tmp->data == NULL) {
      fprintf(stderr, "UNDO_PUSH: Out of memory error.\n");
      free(tmp);
      return;
    }
    memcpy(tmp->data, surface->data, surface->width * surface->height * 4);

    tmp->width = surface->width;
    tmp->height = surface->height;
    tmp->memory_x = surface->parent_xofs;
    tmp->memory_y = surface->parent_yofs;
    undo->stack = tmp;
    undo->stack_depth++;
    undo_prune(undo);
  }
  else {
    fprintf(stderr, "UNDO_PUSH: Out of memory error.\n");
  }
}


int undo_pop(struct td_surf *surface, struct _undo *undo) {

  if (undo) {
    struct _undo_stack *tmp = undo->stack;

    if (tmp) {
      free(surface->data);
      free(surface->view);
      surface->data = tmp->data;
      surface->width = tmp->width;
      surface->height = tmp->height;
      surface->parent_xofs = tmp->memory_x;
      surface->parent_yofs = tmp->memory_y;
      undo->stack = tmp->next;
      free(tmp);
      undo->stack_depth--;
      surface->view = (unsigned char *)malloc((surface->width*surface->zoom) * (surface->height*surface->zoom) * 3);

      return SUCCEEDED;
    }
  }
  
  return FAILED;
}


int common_set_tile_position(int mx, int my, struct td_surf *surface) {

  int x,y,i;
  struct td_surf *root_surf, *child_surf;

  if (surface == NULL)
    return FAILED;
  if (surface->parent == NULL && surface->child == NULL)
    return FAILED;

  if (surface->parent == NULL) {
    /* It's the root, use child information */
    root_surf = surface;
    child_surf = surface->child;
  }
  else {
    /* It's a child, use parent information */
    root_surf = surface->parent;
    child_surf = surface;
  }

  /* get the position on the 1:1 image */
  if (mx < 0 || my < 0) {
    mx = child_surf->parent_xofs * root_surf->zoom;
    my = child_surf->parent_yofs * root_surf->zoom;
  }

  x = mx/root_surf->zoom;
  y = my/root_surf->zoom;

  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;
  if (x > root_surf->width - 1)
    x = root_surf->width - 1;
  if (y > root_surf->height - 1)
    y = root_surf->height - 1;

  /* compute the tile */
  x /= child_surf->width;
  y /= child_surf->height;
  i = y*(root_surf->width/child_surf->width) + x;

  if (child_surf->tilenum == i && memwin.snap_to_grid == YES)
    return FAILED;

  /* we moved to a new tile */
  child_surf->tilenum = i;
  child_surf->parent_xofs = x*child_surf->width;
  child_surf->parent_yofs = y*child_surf->height;

  return SUCCEEDED;
}


int common_compute_tile_position(struct td_surf *surface) {
  
  return common_set_tile_position(-1, -1, surface);
}


int common_copy_data_to_child(struct td_surf *surface) {
  
  int a, b, i, n, p;
  struct td_surf *root_surf, *child_surf;

  if (surface == NULL)
    return FAILED;

  if (surface->parent == NULL) {
    /* It's the root, use child information */
    root_surf = surface;
    child_surf = surface->child;
  }
  else {
    /* It's a child, use parent information */
    root_surf = surface->parent;
    child_surf = surface;
  }

  if (child_surf->data == NULL || child_surf->width <= 0 || child_surf->height <= 0)
    return FAILED;

  for (i = 0, a = 0; a < child_surf->height; a++) {
    p = (child_surf->parent_yofs+a)*root_surf->width + child_surf->parent_xofs;
    for (b = 0; b < child_surf->width; b++) {
      n = (p + b)<<2;
      child_surf->data[i++] = root_surf->data[n + 0];
      child_surf->data[i++] = root_surf->data[n + 1];
      child_surf->data[i++] = root_surf->data[n + 2];
      child_surf->data[i++] = root_surf->data[n + 3];
    }
  }

  return SUCCEEDED;
}


int common_copy_data_to_parent(struct td_surf *surface) {

  int a, b, i, n, p;

  if (surface == NULL)
    return FAILED;
  if (surface->parent == NULL)
    return FAILED;
  if (surface->data == NULL || surface->width <= 0 || surface->height <= 0)
    return FAILED;

  for (i = 0, a = 0; a < surface->height; a++) {
    p = (surface->parent_yofs+a)*surface->parent->width + surface->parent_xofs;
    for (b = 0; b < surface->width; b++) {
      n = (p + b)<<2;
      surface->parent->data[n + 0] = surface->data[i++];
      surface->parent->data[n + 1] = surface->data[i++];
      surface->parent->data[n + 2] = surface->data[i++];
      surface->parent->data[n + 3] = surface->data[i++];
    }
  }

  return SUCCEEDED;
}


void common_getpixel(int x, int y, struct td_color *c, struct td_surf *surface) {

  int i = (y*surface->width + x) * 4;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    c->r = surface->data[i + 0];
    c->g = surface->data[i + 1];
    c->b = surface->data[i + 2];
    c->a = surface->data[i + 3];
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    c->r = surface->data[i + 0];
    c->g = surface->data[i + 1];
    c->b = surface->data[i + 2];
  }
  else {
    c->a = surface->data[i + 3];
  }
}


void _common_putpixel(int x, int y, struct td_color *c, struct td_surf *surface) {

  int i = (y*surface->width + x)<<2;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    surface->data[i + 0] = c->r;
    surface->data[i + 1] = c->g;
    surface->data[i + 2] = c->b;
    surface->data[i + 3] = c->a;
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    surface->data[i + 0] = c->r;
    surface->data[i + 1] = c->g;
    surface->data[i + 2] = c->b;
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    surface->data[i + 3] = c->a;
  }
}


void _common_putpixel_negate(int x, int y, struct td_surf *surface) {
  
  int i = (y*surface->width + x)<<2;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    surface->data[i + 0] = ~(surface->data[i + 0]);
    surface->data[i + 1] = ~(surface->data[i + 1]);
    surface->data[i + 2] = ~(surface->data[i + 2]);
    surface->data[i + 3] = ~(surface->data[i + 3]);
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    surface->data[i + 0] = ~(surface->data[i + 0]);
    surface->data[i + 1] = ~(surface->data[i + 1]);
    surface->data[i + 2] = ~(surface->data[i + 2]);
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    surface->data[i + 3] = ~(surface->data[i + 3]);
  }
}


void _common_putpixel_view(int x, int y, struct td_color *c, struct td_surf *out) {
  
  int ip = (y*out->width*out->zoom + x)*3;

  out->view[ip] = c->r; ip++;
  out->view[ip] = c->g; ip++;
  out->view[ip] = c->b; ip++;
}


void _common_putpixel_view_negate(int x, int y, struct td_surf *out) {
  
  int ip = (y*out->width*out->zoom + x)*3;

  out->view[ip] = ~(out->view[ip]); ip++;
  out->view[ip] = ~(out->view[ip]); ip++;
  out->view[ip] = ~(out->view[ip]); ip++;
}


void _common_putpixel_zoomedview_negate(int x, int y, struct td_surf *surface) {

  unsigned char *d;
  int z, a, b, dx, dy, q;

  d = surface->view;
  z = surface->zoom;
  dx = surface->width;
  dy = surface->height;

  for (a = 0; a < z; a++) {
    for (b = 0; b < z; b++) {
      q = 255 - d[(y*z*dx*z + x*z + a*dx*z + b)*3 + 0];
      d[(y*z*dx*z + x*z + a*dx*z + b)*3 + 0] = q;
      q = 255 - d[(y*z*dx*z + x*z + a*dx*z + b)*3 + 1];
      d[(y*z*dx*z + x*z + a*dx*z + b)*3 + 1] = q;
      q = 255 - d[(y*z*dx*z + x*z + a*dx*z + b)*3 + 2];
      d[(y*z*dx*z + x*z + a*dx*z + b)*3 + 2] = q;
    }
  }
}


void _common_putpixel_zoomedview(int x, int y, struct td_color *c, struct td_surf *surface) {
  
  unsigned char *d;
  int z, a, b, dx, dy;

  d = surface->view;
  z = surface->zoom;
  dx = surface->width;
  dy = surface->height;

  for (a = 0; a < z; a++) {
    for (b = 0; b < z; b++) {
      d[(y*z*dx*z + x*z + a*dx*z + b)*3 + 0] = c->r;
      d[(y*z*dx*z + x*z + a*dx*z + b)*3 + 1] = c->g;
      d[(y*z*dx*z + x*z + a*dx*z + b)*3 + 2] = c->b;
    }
  }
}


int common_putpix_mode = PUTPIX_NORMAL | PUTPIX_VIEW;


void common_putpixel_setmode(int putpix) {

  common_putpix_mode = putpix;
}


void common_putpixel(int x, int y, struct td_color *col, struct td_surf *surface) {
  
  if (x < 0 || x >= surface->width || y < 0 || y >= surface->height)
    return;

  switch (common_putpix_mode) {
  default:
  case PUTPIX_NORMAL:
    _common_putpixel(x,y, col, surface);
    break;
  case PUTPIX_NEGATE:
    _common_putpixel_negate(x,y, surface);
    break;
  case PUTPIX_NORMAL|PUTPIX_VIEW:
    _common_putpixel_view(x,y, col, surface);
    break;
  case PUTPIX_NEGATE|PUTPIX_VIEW:
    _common_putpixel_view_negate(x,y, surface);
    break;
  case PUTPIX_NORMAL|PUTPIX_ZOOM|PUTPIX_VIEW: /* Zoom implies View */
  case PUTPIX_NORMAL|PUTPIX_ZOOM:
    _common_putpixel_zoomedview(x,y, col, surface);
    break;
  case PUTPIX_NEGATE|PUTPIX_ZOOM|PUTPIX_VIEW: /* Zoom implies View */
  case PUTPIX_NEGATE|PUTPIX_ZOOM:
    _common_putpixel_zoomedview_negate(x,y, surface);
    break;
  }
}


void common_putrect(int x, int y, int x2, int y2, struct td_color *col, struct td_surf *surface, int filled) {
  
  int dy,dx;
  int tmp;

  if (x > x2) {
    tmp = x;
    x = x2;
    x2 = tmp;
  }
  if (y > y2) {
    tmp = y;
    y = y2;
    y2 = tmp;
  }

  if (filled) {
    for (dy = y; dy <= y2; dy++) {
      for (dx = x; dx <= x2; dx++)
        common_putpixel(dx,dy, col, surface);
    }
  }
  else {
    for (dy = y; dy <= y2; dy++) {
      common_putpixel(x,dy, col, surface);
      common_putpixel(x2,dy, col, surface);
    }
    for (dx = x; dx < x2; dx++) {
      common_putpixel(dx,y, col, surface);
      common_putpixel(dx,y2, col, surface);
    }
  }
}


void common_putline(int x, int y, int x2, int y2, struct td_color *col, struct td_surf *surface) {
  
  int d,dx,dy,ai,bi,xi,yi;

  if (x == x2 && y == y2) {
    common_putpixel(x,y, col, surface);
    return;
  }

  if (x < x2) {
    xi = 1;
    dx = x2 - x;
  }
  else {
    xi = - 1;
    dx = x - x2;
  }

  if (y < y2) {
    yi = 1;
    dy = y2 - y;
  }
  else {
    yi = - 1;
    dy = y - y2;
  }

  common_putpixel(x,y, col, surface);

  if (dx > dy) {
    ai = (dy - dx) * 2;
    bi = dy * 2;
    d  = bi - dx;
    do {
      if (d >= 0) {
        y += yi;
        d += ai;
      }
      else
        d += bi;

      x += xi;
      common_putpixel(x,y, col, surface);
    } while (x != x2);
  }
  else {
    ai = (dx - dy) * 2;
    bi = dx * 2;
    d  = bi - dy;
    do {
      if (d >= 0) {
        x += xi;
        d += ai;
      }
      else
        d += bi;

      y += yi;
      common_putpixel(x,y, col, surface);
    } while (y != y2);
  }
}


void _common_putcircle_filled(int center_x, int center_y, int rx, int ry, struct td_color *col, struct td_surf *surface) {

  /* intermediate terms to speed up loop */
  long t1 = rx*rx, t2 = t1<<1, t3 = t2<<1;
  long t4 = ry*ry, t5 = t4<<1, t6 = t5<<1;
  long t7 = rx*t5, t8 = t7<<1, t9 = 0L;
  long d1 = t2 - t7 + (t4>>1);  /* error terms */
  long d2 = (t1>>1) - t8 + t5;
  int tmp;

  register int x = rx, y = 0;   /* ellipse points */

  while (d2 < 0) {
    /* draw 4 points using symmetry */
    for (tmp = 0; tmp < x; tmp++) {
      common_putpixel(center_x + tmp, center_y + y, col, surface);
      common_putpixel(center_x + tmp, center_y - y, col, surface);
      common_putpixel(center_x - tmp, center_y + y, col, surface);
      common_putpixel(center_x - tmp, center_y - y, col, surface);
    }

    y++;                /* always move up here */
    t9 += t3;
    if (d1 < 0) {
      d1 += t9 + t2;
      d2 += t9;
    }
    else {
      x--;
      t8 -= t6;
      d1 += t9 + t2 - t8;
      d2 += t9 + t5 - t8;
    }
  }

  do {
    /* draw 4 points using symmetry */
    for (tmp = 0; tmp < y; tmp++) {
      common_putpixel(center_x + x, center_y + tmp, col, surface);
      common_putpixel(center_x + x, center_y - tmp, col, surface);
      common_putpixel(center_x - x, center_y + tmp, col, surface);
      common_putpixel(center_x - x, center_y - tmp, col, surface);
    }
    x--;                /* always move left here */
    t8 -= t6;
    if (d2 < 0) {
      y++;
      t9 += t3;
      d2 += t9 + t5 - t8;
    }
    else
      d2 += t5 - t8;
  } while (x>=0);
}


void _common_putcircle(int center_x, int center_y, int rx, int ry, struct td_color *col, struct td_surf *surface) {
  
  /* intermediate terms to speed up loop */
  long t1 = rx*rx, t2 = t1<<1, t3 = t2<<1;
  long t4 = ry*ry, t5 = t4<<1, t6 = t5<<1;
  long t7 = rx*t5, t8 = t7<<1, t9 = 0L;
  long d1 = t2 - t7 + (t4>>1);  /* error terms */
  long d2 = (t1>>1) - t8 + t5;

  register int x = rx, y = 0;   /* ellipse points */

  while (d2 < 0) {
    /* draw 4 points using symmetry */
    common_putpixel(center_x + x, center_y + y, col, surface);
    common_putpixel(center_x + x, center_y - y, col, surface);
    common_putpixel(center_x - x, center_y + y, col, surface);
    common_putpixel(center_x - x, center_y - y, col, surface);

    y++;                /* always move up here */
    t9 += t3;
    if (d1 < 0) {
      d1 += t9 + t2;
      d2 += t9;
    } else {
      x--;
      t8 -= t6;
      d1 += t9 + t2 - t8;
      d2 += t9 + t5 - t8;
    }
  }

  do {
    /* draw 4 points using symmetry */
    common_putpixel(center_x + x, center_y + y, col, surface);
    common_putpixel(center_x + x, center_y - y, col, surface);
    common_putpixel(center_x - x, center_y + y, col, surface);
    common_putpixel(center_x - x, center_y - y, col, surface);

    x--;                /* always move left here */
    t8 -= t6;
    if (d2 < 0) {
      y++;
      t9 += t3;
      d2 += t9 + t5 - t8;
    } else
      d2 += t5 - t8;
  } while (x>=0);
}


void common_putcircle(int center_x, int center_y, int rx, int ry, struct td_color *col, struct td_surf *surface, int filled) {
  
  if (filled)
    _common_putcircle_filled(center_x,center_y,rx,ry,col,surface);
  else
    _common_putcircle(center_x,center_y,rx,ry,col,surface);
}


void common_line_dotted_horiz(int y, struct td_surf *out) {
  
  int x;
  int count = 0;
  struct td_color col_black, col_white;

  col_black.r = col_black.g = col_black.b = 0;
  col_white.r = col_white.g = col_white.b = col_white.a = col_black.a = 255;

  for (x = 0; x < (out->width*out->zoom)-1; x++)
    _common_putpixel_view(x,y,(++count&1) ? &col_black : &col_white, out);
}


void common_line_dotted_vert(int x, struct td_surf *out) {
  
  int y;
  int count = 0;
  struct td_color col_black, col_white;

  col_black.r = col_black.g = col_black.b = 0;
  col_white.r = col_white.g = col_white.b = col_white.a = col_black.a = 255;

  for (y = 0; y < (out->height*out->zoom)-1; y++)
    _common_putpixel_view(x,y,(++count&1) ? &col_black : &col_white, out);
}


void common_clear(struct td_color *col, struct td_surf *surface) {

  int x, y;

  for (x = 0; x < surface->width; x++) {
    for (y = 0; y < surface->height; y++)
      common_putpixel(x,y, col, surface);
  }
}


int common_editor_filter_3x3(float *filter, struct td_surf *surface) {
  
  unsigned int x, y, a, b, n, i;
  unsigned char *t;
  float cr, cg, cb, ca;


  if (filter == NULL)
    return FAILED;

  t = malloc(surface->width*surface->height*4);
  if (t == NULL) {
    fprintf(stderr, "COMMON_EDITOR_FILTER_3X3: Out of memory error.\n");
    return FAILED;
  }

  /* blur the data */
  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < surface->height; y++) {
      for (x = 0; x < surface->width; x++) {
        cr = 0;
        cg = 0;
        cb = 0;
        ca = 0;
        for (i = 0, a = 0; a < 3; a++) {
          for (b = 0; b < 3; b++, i++) {
            n = (((y+a-1)%surface->height)*surface->width + ((x+b-1)%surface->width))<<2;
            cr += surface->data[n + 0]*filter[i];
            cg += surface->data[n + 1]*filter[i];
            cb += surface->data[n + 2]*filter[i];
            ca += surface->data[n + 3]*filter[i];
          }
        }
        n = (y*surface->width + x)<<2;
        t[n + 0] = cr;
        t[n + 1] = cg;
        t[n + 2] = cb;
        t[n + 3] = ca;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 0; y < surface->height; y++) {
      for (x = 0; x < surface->width; x++) {
        cr = 0;
        cg = 0;
        cb = 0;
        for (i = 0, a = 0; a < 3; a++) {
          for (b = 0; b < 3; b++, i++) {
            n = (((y+a-1)%surface->height)*surface->width + ((x+b-1)%surface->width))<<2;
            cr += surface->data[n + 0]*filter[i];
            cg += surface->data[n + 1]*filter[i];
            cb += surface->data[n + 2]*filter[i];
          }
        }
        n = (y*surface->width + x)<<2;
        t[n + 0] = cr;
        t[n + 1] = cg;
        t[n + 2] = cb;
        t[n + 3] = surface->data[n + 3];
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 0; y < surface->height; y++) {
      for (x = 0; x < surface->width; x++) {
        ca = 0;
        for (i = 0, a = 0; a < 3; a++) {
          for (b = 0; b < 3; b++, i++) {
            n = (((y+a-1)%surface->height)*surface->width + ((x+b-1)%surface->width))<<2;
            ca += surface->data[n + 3]*filter[i];
          }
        }
        n = (y*surface->width + x)<<2;
        t[n + 0] = surface->data[n + 0];
        t[n + 1] = surface->data[n + 1];
        t[n + 2] = surface->data[n + 2];
        t[n + 3] = ca;
      }
    }
  }

  memcpy(surface->data, t, surface->width*surface->height*4);
  free(t);

  return SUCCEEDED;
}


void common_filters_negate(struct td_surf *surface) {

  int tmp = common_putpix_mode;

  common_putpix_mode = PUTPIX_NEGATE;
  common_clear(NULL, surface);
  common_putpix_mode = tmp;
}


void common_filters_build_alpha(struct td_color *col, struct td_surf *surface) {
  
  unsigned int x, y, n;

  /* build the alpha channel from the colors */
  for (y = 0; y < surface->height; y++) {
    for (x = 0; x < surface->width; x++) {
      n = (y*surface->width + x)<<2;
      if (surface->data[n + 0] == col->r && surface->data[n + 1] == col->g && surface->data[n + 2] == col->b)
        surface->data[n + 3] = 0;
      else
        surface->data[n + 3] = 255;
    }
  }
}


void common_mirror_lr(struct td_surf *surface) {

  unsigned int x, y, n, m;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < surface->height; y++) {
      for (x = 0; x < surface->width/2; x++) {
        n = (y*surface->width + (surface->width-x-1)) << 2;
        m = (y*surface->width + x) << 2;
        surface->data[n++] = surface->data[m++];
        surface->data[n++] = surface->data[m++];
        surface->data[n++] = surface->data[m++];
        surface->data[n]   = surface->data[m];
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 0; y < surface->height; y++) {
      for (x = 0; x < surface->width/2; x++) {
        n = (y*surface->width + (surface->width-x-1)) << 2;
        m = (y*surface->width + x) << 2;
        surface->data[n++] = surface->data[m++];
        surface->data[n++] = surface->data[m++];
        surface->data[n]   = surface->data[m];
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 0; y < surface->height; y++) {
      for (x = 0; x < surface->width/2; x++) {
        surface->data[(y*surface->width + (surface->width-x-1))*4 + 3] = surface->data[(y*surface->width + x)*4 + 3];
      }
    }
  }
}


void common_mirror_tb(struct td_surf *surface) {

  unsigned int x, y, m, n;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < surface->height/2; y++) {
      for (x = 0; x < surface->width; x++) {
        n = ((surface->height-y-1)*surface->width + x) << 2;
        m = (y*surface->width + x) << 2;
        surface->data[n++] = surface->data[m++];
        surface->data[n++] = surface->data[m++];
        surface->data[n++] = surface->data[m++];
        surface->data[n]   = surface->data[m];
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 0; y < surface->height/2; y++) {
      for (x = 0; x < surface->width; x++) {
        n = ((surface->height-y-1)*surface->width + x) << 2;
        m = (y*surface->width + x) << 2;
        surface->data[n++] = surface->data[m++];
        surface->data[n++] = surface->data[m++];
        surface->data[n]   = surface->data[m];
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 0; y < surface->height/2; y++) {
      for (x = 0; x < surface->width; x++) {
        surface->data[((surface->height-y-1)*surface->width + x)*4 + 3] = surface->data[(y*surface->width + x)*4 + 3];
      }
    }
  }
}


void common_rotate_right(struct td_surf *surface) {

  unsigned int x, y;
  unsigned char *c;

  c = malloc(surface->width*surface->height*4);
  if (c == NULL) {
    fprintf(stderr, "COMMON_ROTATE_RIGHT: Out of memory error.\n");
    return;
  }
  memcpy(c, surface->data, surface->width*surface->height*4);

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < surface->width; y++) {
      for (x = 0; x < surface->height; x++) {
        surface->data[y*surface->width*4 + x*4 + 0] = c[(surface->width-x-1)*surface->width*4 + y*4 + 0];
        surface->data[y*surface->width*4 + x*4 + 1] = c[(surface->width-x-1)*surface->width*4 + y*4 + 1];
        surface->data[y*surface->width*4 + x*4 + 2] = c[(surface->width-x-1)*surface->width*4 + y*4 + 2];
        surface->data[y*surface->width*4 + x*4 + 3] = c[(surface->width-x-1)*surface->width*4 + y*4 + 3];
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 0; y < surface->width; y++) {
      for (x = 0; x < surface->height; x++) {
        surface->data[y*surface->width*4 + x*4 + 0] = c[(surface->width-x-1)*surface->width*4 + y*4 + 0];
        surface->data[y*surface->width*4 + x*4 + 1] = c[(surface->width-x-1)*surface->width*4 + y*4 + 1];
        surface->data[y*surface->width*4 + x*4 + 2] = c[(surface->width-x-1)*surface->width*4 + y*4 + 2];
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 0; y < surface->width; y++) {
      for (x = 0; x < surface->height; x++) {
        surface->data[y*surface->width*4 + x*4 + 3] = c[(surface->width-x-1)*surface->width*4 + y*4 + 3];
      }
    }
  }
  free(c);
}


void common_rotate_left(struct td_surf *surface) {

  unsigned int x, y;
  unsigned char *c;

  c = malloc(surface->width*surface->height*4);
  if (c == NULL) {
    fprintf(stderr, "COMMON_ROTATE_LEFT: Out of memory error.\n");
    return;
  }
  memcpy(c, surface->data, surface->width*surface->height*4);

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < surface->width; y++) {
      for (x = 0; x < surface->height; x++) {
        surface->data[y*surface->width*4 + x*4 + 0] = c[x*surface->width*4 + (surface->height-y-1)*4 + 0];
        surface->data[y*surface->width*4 + x*4 + 1] = c[x*surface->width*4 + (surface->height-y-1)*4 + 1];
        surface->data[y*surface->width*4 + x*4 + 2] = c[x*surface->width*4 + (surface->height-y-1)*4 + 2];
        surface->data[y*surface->width*4 + x*4 + 3] = c[x*surface->width*4 + (surface->height-y-1)*4 + 3];
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 0; y < surface->width; y++) {
      for (x = 0; x < surface->height; x++) {
        surface->data[y*surface->width*4 + x*4 + 0] = c[x*surface->width*4 + (surface->height-y-1)*4 + 0];
        surface->data[y*surface->width*4 + x*4 + 1] = c[x*surface->width*4 + (surface->height-y-1)*4 + 1];
        surface->data[y*surface->width*4 + x*4 + 2] = c[x*surface->width*4 + (surface->height-y-1)*4 + 2];
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 0; y < surface->width; y++) {
      for (x = 0; x < surface->height; x++) {
        surface->data[y*surface->width*4 + x*4 + 3] = c[x*surface->width*4 + (surface->height-y-1)*4 + 3];
      }
    }
  }
  free(c);
}


void common_mirror_and_blend_lin_x(struct td_surf *surface) {

  unsigned int x, y, a, b;
  float f, c, d, w, v;

  f = 1.0f/(surface->width/4.0f);

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < surface->height; y++) {
      c = 0;
      for (x = 0; x < surface->width/2-1; x++) {
        d = 1.0f - c;
        a = (y*surface->width + (surface->width-x-1))*4;
        b = (y*surface->width + x+1)*4;
        w = surface->data[a + 0];
        v = surface->data[b + 0];
        surface->data[a + 0] = w*c + v*d;
        w = surface->data[a + 1];
        v = surface->data[b + 1];
        surface->data[a + 1] = w*c + v*d;
        w = surface->data[a + 2];
        v = surface->data[b + 2];
        surface->data[a + 2] = w*c + v*d;
        w = surface->data[a + 3];
        v = surface->data[b + 3];
        surface->data[a + 3] = w*c + v*d;
        c += f;
        if (c > 1)
          c = 1;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 0; y < surface->height; y++) {
      c = 0;
      for (x = 0; x < surface->width/2-1; x++) {
        d = 1.0f - c;
        a = (y*surface->width + (surface->width-x-1))*4;
        b = (y*surface->width + x+1)*4;
        w = surface->data[a + 0];
        v = surface->data[b + 0];
        surface->data[a + 0] = w*c + v*d;
        w = surface->data[a + 1];
        v = surface->data[b + 1];
        surface->data[a + 1] = w*c + v*d;
        w = surface->data[a + 2];
        v = surface->data[b + 2];
        surface->data[a + 2] = w*c + v*d;
        c += f;
        if (c > 1)
          c = 1;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 0; y < surface->height; y++) {
      c = 0;
      for (x = 0; x < surface->width/2-1; x++) {
        d = 1.0f - c;
        a = (y*surface->width + (surface->width-x-1))*4;
        b = (y*surface->width + x+1)*4;
        w = surface->data[a + 2];
        v = surface->data[b + 2];
        surface->data[a + 2] = w*c + v*d;
        c += f;
        if (c > 1)
          c = 1;
      }
    }
  }
}


void common_mirror_and_blend_lin_y(struct td_surf *surface) {

  unsigned int x, y, a, b;
  float f, c, d, w, v;

  f = 1.0f/(surface->height/4.0f);

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (x = 0; x < surface->width; x++) {
      c = 0;
      for (y = 0; y < surface->height/2-1; y++) {
        d = 1.0f - c;
        a = ((surface->height-y-1)*surface->width + x)*4;
        b = ((y+1)*surface->width + x)*4;
        w = surface->data[a + 0];
        v = surface->data[b + 0];
        surface->data[a + 0] = w*c + v*d;
        w = surface->data[a + 1];
        v = surface->data[b + 1];
        surface->data[a + 1] = w*c + v*d;
        w = surface->data[a + 2];
        v = surface->data[b + 2];
        surface->data[a + 2] = w*c + v*d;
        w = surface->data[a + 3];
        v = surface->data[b + 3];
        surface->data[a + 3] = w*c + v*d;
        c += f;
        if (c > 1)
          c = 1;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (x = 0; x < surface->width; x++) {
      c = 0;
      for (y = 0; y < surface->height/2-1; y++) {
        d = 1.0f - c;
        a = ((surface->height-y-1)*surface->width + x)*4;
        b = ((y+1)*surface->width + x)*4;
        w = surface->data[a + 0];
        v = surface->data[b + 0];
        surface->data[a + 0] = w*c + v*d;
        w = surface->data[a + 1];
        v = surface->data[b + 1];
        surface->data[a + 1] = w*c + v*d;
        w = surface->data[a + 2];
        v = surface->data[b + 2];
        surface->data[a + 2] = w*c + v*d;
        c += f;
        if (c > 1)
          c = 1;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (x = 0; x < surface->width; x++) {
      c = 0;
      for (y = 0; y < surface->height/2-1; y++) {
        d = 1.0f - c;
        a = ((surface->height-y-1)*surface->width + x)*4;
        b = ((y+1)*surface->width + x)*4;
        w = surface->data[a + 3];
        v = surface->data[b + 3];
        surface->data[a + 3] = w*c + v*d;
        c += f;
        if (c > 1)
          c = 1;
      }
    }
  }
}


void common_mirror_and_blend_sto_x(struct td_surf *surface) {

  unsigned int x, y, a, b;
  float c, d, w, v;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < surface->height; y++) {
      c = 1.0f - (rand()/(double)RAND_MAX/5.0);
      for (x = 0; x < surface->width/2-1; x++) {
        d = 1.0f - c;
        a = (y*surface->width + (surface->width-x-1))*4;
        b = (y*surface->width + x+1)*4;
        w = surface->data[a + 0];
        v = surface->data[b + 0];
        surface->data[a + 0] = w*d + v*c;
        w = surface->data[a + 1];
        v = surface->data[b + 1];
        surface->data[a + 1] = w*d + v*c;
        w = surface->data[a + 2];
        v = surface->data[b + 2];
        surface->data[a + 2] = w*d + v*c;
        w = surface->data[a + 3];
        v = surface->data[b + 3];
        surface->data[a + 3] = w*d + v*c;
        c = c - (rand()/(double)RAND_MAX/20.0 + 0.1);
        if (c < 0)
          c = 0;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 0; y < surface->height; y++) {
      c = 1.0f - (rand()/(double)RAND_MAX/5.0);
      for (x = 0; x < surface->width/2-1; x++) {
        d = 1.0f - c;
        a = (y*surface->width + (surface->width-x-1))*4;
        b = (y*surface->width + x+1)*4;
        w = surface->data[a + 0];
        v = surface->data[b + 0];
        surface->data[a + 0] = w*d + v*c;
        w = surface->data[a + 1];
        v = surface->data[b + 1];
        surface->data[a + 1] = w*d + v*c;
        w = surface->data[a + 2];
        v = surface->data[b + 2];
        surface->data[a + 2] = w*d + v*c;
        c = c - (rand()/(double)RAND_MAX/20.0 + 0.1);
        if (c < 0)
          c = 0;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 0; y < surface->height; y++) {
      c = 1.0f - (rand()/(double)RAND_MAX/5.0);
      for (x = 0; x < surface->width/2-1; x++) {
        d = 1.0f - c;
        a = (y*surface->width + (surface->width-x-1))*4;
        b = (y*surface->width + x+1)*4;
        w = surface->data[a + 3];
        v = surface->data[b + 3];
        surface->data[a + 3] = w*d + v*c;
        c = c - (rand()/(double)RAND_MAX/20.0 + 0.1);
        if (c < 0)
          c = 0;
      }
    }
  }
}


void common_mirror_and_blend_sto_y(struct td_surf *surface) {

  unsigned int x, y, a, b;
  float c, d, w, v;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (x = 0; x < surface->width; x++) {
      c = 1.0f - (rand()/(double)RAND_MAX/5.0);
      for (y = 0; y < surface->height/2-1; y++) {
        d = 1.0f - c;
        a = ((surface->height-y-1)*surface->width + x)*4;
        b = ((y+1)*surface->width + x)*4;
        w = surface->data[a + 0];
        v = surface->data[b + 0];
        surface->data[a + 0] = w*d + v*c;
        w = surface->data[a + 1];
        v = surface->data[b + 1];
        surface->data[a + 1] = w*d + v*c;
        w = surface->data[a + 2];
        v = surface->data[b + 2];
        surface->data[a + 2] = w*d + v*c;
        w = surface->data[a + 3];
        v = surface->data[b + 3];
        surface->data[a + 3] = w*d + v*c;
        c = c - (rand()/(double)RAND_MAX/20.0 + 0.1);
        if (c < 0)
          c = 0;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (x = 0; x < surface->width; x++) {
      c = 1.0f - (rand()/(double)RAND_MAX/5.0);
      for (y = 0; y < surface->height/2-1; y++) {
        d = 1.0f - c;
        a = ((surface->height-y-1)*surface->width + x)*4;
        b = ((y+1)*surface->width + x)*4;
        w = surface->data[a + 0];
        v = surface->data[b + 0];
        surface->data[a + 0] = w*d + v*c;
        w = surface->data[a + 1];
        v = surface->data[b + 1];
        surface->data[a + 1] = w*d + v*c;
        w = surface->data[a + 2];
        v = surface->data[b + 2];
        surface->data[a + 2] = w*d + v*c;
        c = c - (rand()/(double)RAND_MAX/20.0 + 0.1);
        if (c < 0)
          c = 0;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (x = 0; x < surface->width; x++) {
      c = 1.0f - (rand()/(double)RAND_MAX/5.0);
      for (y = 0; y < surface->height/2-1; y++) {
        d = 1.0f - c;
        a = ((surface->height-y-1)*surface->width + x)*4;
        b = ((y+1)*surface->width + x)*4;
        w = surface->data[a + 3];
        v = surface->data[b + 3];
        surface->data[a + 3] = w*d + v*c;
        c = c - (rand()/(double)RAND_MAX/20.0 + 0.1);
        if (c < 0)
          c = 0;
      }
    }
  }
}


void common_erosion(struct td_color *col, struct td_surf *surface) {

  unsigned char *c, r, g, b, a;
  int n, i, j, x, y;

  c = malloc(surface->width*surface->height*4);
  if (c == NULL) {
    fprintf(stderr, "COMMON_EROSION: Out of memory error.\n");
    return;
  }
  memcpy(c, surface->data, surface->width*surface->height*4);

  n = surface->width<<2;
  r = col->r;
  g = col->g;
  b = col->b;
  a = col->a;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < surface->height; y++) {
      for (x = 0; x < surface->width; x++) {
        i = y*n + (x<<2);
        /* skip background pixels */
        if (surface->data[i + 0] == r && surface->data[i + 1] == g &&
            surface->data[i + 2] == b && surface->data[i + 3] == a)
          continue;

        /* can we erode the pixel? */
        j = 0;

        i -= n;
        if (y > 0) {
          if (surface->data[i + 0] == r && surface->data[i + 1] == g &&
              surface->data[i + 2] == b && surface->data[i + 3] == a)
            j++;
        }

        i += n+n;
        if (y < surface->height-1) {
          if (surface->data[i + 0] == r && surface->data[i + 1] == g &&
              surface->data[i + 2] == b && surface->data[i + 3] == a)
            j++;
        }

        i -= n+4;
        if (x > 0) {
          if (surface->data[i + 0] == r && surface->data[i + 1] == g &&
              surface->data[i + 2] == b && surface->data[i + 3] == a)
            j++;
        }

        i += 8;
        if (x < surface->width-1) {
          if (surface->data[i + 0] == r && surface->data[i + 1] == g &&
              surface->data[i + 2] == b && surface->data[i + 3] == a)
            j++;
        }

        if (j > 0) {
          /* yep! erosion, baby! */
          i -= 4;
          c[i + 0] = r;
          c[i + 1] = g;
          c[i + 2] = b;
          c[i + 3] = a;
        }
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 1; y < surface->height-1; y++) {
      for (x = 1; x < surface->width-1; x++) {
        i = y*n + (x<<2);
        /* skip background pixels */
        if (surface->data[i + 0] == r && surface->data[i + 1] == g && surface->data[i + 2] == b)
          continue;

        /* can we erode the pixel? */
        j = 0;

        i -= n;
        if (y > 0) {
          if (surface->data[i + 0] == r && surface->data[i + 1] == g && surface->data[i + 2] == b)
            j++;
        }

        i += n+n;
        if (y < surface->height-1) {
          if (surface->data[i + 0] == r && surface->data[i + 1] == g && surface->data[i + 2] == b)
            j++;
        }

        i -= n+4;
        if (x > 0) {
          if (surface->data[i + 0] == r && surface->data[i + 1] == g && surface->data[i + 2] == b)
            j++;
        }

        i += 8;
        if (x < surface->width-1) {
          if (surface->data[i + 0] == r && surface->data[i + 1] == g && surface->data[i + 2] == b)
            j++;
        }

        if (j > 0) {
          /* yep! erosion, baby! */
          i -= 4;
          c[i + 0] = r;
          c[i + 1] = g;
          c[i + 2] = b;
        }
      }
    }
  }
  if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 1; y < surface->height-1; y++) {
      for (x = 1; x < surface->width-1; x++) {
        i = y*n + (x<<2);
        /* skip background pixels */
        if (surface->data[i + 3] == a)
          continue;

        /* can we erode the pixel? */
        j = 0;

        i -= n;
        if (y > 0) {
          if (surface->data[i + 3] == a)
            j++;
        }

        i += n+n;
        if (y < surface->height-1) {
          if (surface->data[i + 3] == a)
            j++;
        }

        i -= n+4;
        if (x > 0) {
          if (surface->data[i + 3] == a)
            j++;
        }

        i += 8;
        if (x < surface->width-1) {
          if (surface->data[i + 3] == a)
            j++;
        }

        if (j > 0) {
          /* yep! erosion, baby! */
          i -= 4;
          c[i + 3] = a;
        }
      }
    }
  }

  memcpy(surface->data, c, surface->width*surface->height*4);
  free(c);
}


void common_flip_x(struct td_surf *surface) {

  unsigned char c;
  int x, y, a, b, n;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < surface->height; y++) {
      a = (y*surface->width)<<2;
      for (x = 0; x < surface->width/2; x++) {
        b = a + (x<<2);
        n = a + ((surface->width-1-x)<<2);
        c = surface->data[b + 0];
        surface->data[b + 0] = surface->data[n + 0];
        surface->data[n + 0] = c;
        c = surface->data[b + 1];
        surface->data[b + 1] = surface->data[n + 1];
        surface->data[n + 1] = c;
        c = surface->data[b + 2];
        surface->data[b + 2] = surface->data[n + 2];
        surface->data[n + 2] = c;
        c = surface->data[b + 3];
        surface->data[b + 3] = surface->data[n + 3];
        surface->data[n + 3] = c;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 0; y < surface->height; y++) {
      a = (y*surface->width)<<2;
      for (x = 0; x < surface->width/2; x++) {
        b = a + (x<<2);
        n = a + ((surface->width-1-x)<<2);
        c = surface->data[b + 0];
        surface->data[b + 0] = surface->data[n + 0];
        surface->data[n + 0] = c;
        c = surface->data[b + 1];
        surface->data[b + 1] = surface->data[n + 1];
        surface->data[n + 1] = c;
        c = surface->data[b + 2];
        surface->data[b + 2] = surface->data[n + 2];
        surface->data[n + 2] = c;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 0; y < surface->height; y++) {
      a = (y*surface->width)<<2;
      for (x = 0; x < surface->width/2; x++) {
        b = a + (x<<2);
        n = a + ((surface->width-1-x)<<2);
        c = surface->data[b + 3];
        surface->data[b + 3] = surface->data[n + 3];
        surface->data[n + 3] = c;
      }
    }
  }
}


void common_flip_y(struct td_surf *surface) {

  unsigned char c;
  int x, y, a, b, n;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < surface->height/2; y++) {
      a = (y*surface->width)<<2;
      b = ((surface->height-1-y)*surface->width)<<2;
      for (x = 0; x < surface->width; x++) {
        n = x<<2;
        c = surface->data[a + n + 0];
        surface->data[a + n + 0] = surface->data[b + n + 0];
        surface->data[b + n + 0] = c;
        c = surface->data[a + n + 1];
        surface->data[a + n + 1] = surface->data[b + n + 1];
        surface->data[b + n + 1] = c;
        c = surface->data[a + n + 2];
        surface->data[a + n + 2] = surface->data[b + n + 2];
        surface->data[b + n + 2] = c;
        c = surface->data[a + n + 3];
        surface->data[a + n + 3] = surface->data[b + n + 3];
        surface->data[b + n + 3] = c;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 0; y < surface->height/2; y++) {
      a = (y*surface->width)<<2;
      b = ((surface->height-1-y)*surface->width)<<2;
      for (x = 0; x < surface->width; x++) {
        n = x<<2;
        c = surface->data[a + n + 0];
        surface->data[a + n + 0] = surface->data[b + n + 0];
        surface->data[b + n + 0] = c;
        c = surface->data[a + n + 1];
        surface->data[a + n + 1] = surface->data[b + n + 1];
        surface->data[b + n + 1] = c;
        c = surface->data[a + n + 2];
        surface->data[a + n + 2] = surface->data[b + n + 2];
        surface->data[b + n + 2] = c;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 0; y < surface->height/2; y++) {
      a = (y*surface->width)<<2;
      b = ((surface->height-1-y)*surface->width)<<2;
      for (x = 0; x < surface->width; x++) {
        n = (x<<2) + 3;
        c = surface->data[a + n];
        surface->data[a + n] = surface->data[b + n];
        surface->data[b + n] = c;
      }
    }
  }
}


void common_shift_left(struct td_surf *surface) {

  unsigned char r, g, b, a;
  int x, y, n;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < surface->height; y++) {
      n = (y*surface->width)<<2;
      r = surface->data[n + 0];
      g = surface->data[n + 1];
      b = surface->data[n + 2];
      a = surface->data[n + 3];
      for (x = 0; x < surface->width-1; x++) {
        surface->data[n + x*4 + 0] = surface->data[n + (x+1)*4 + 0];
        surface->data[n + x*4 + 1] = surface->data[n + (x+1)*4 + 1];
        surface->data[n + x*4 + 2] = surface->data[n + (x+1)*4 + 2];
        surface->data[n + x*4 + 3] = surface->data[n + (x+1)*4 + 3];
      }
      surface->data[n + x*4 + 0] = r;
      surface->data[n + x*4 + 1] = g;
      surface->data[n + x*4 + 2] = b;
      surface->data[n + x*4 + 3] = a;
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 0; y < surface->height; y++) {
      n = (y*surface->width)<<2;
      r = surface->data[n + 0];
      g = surface->data[n + 1];
      b = surface->data[n + 2];
      for (x = 0; x < surface->width-1; x++) {
        surface->data[n + x*4 + 0] = surface->data[n + (x+1)*4 + 0];
        surface->data[n + x*4 + 1] = surface->data[n + (x+1)*4 + 1];
        surface->data[n + x*4 + 2] = surface->data[n + (x+1)*4 + 2];
      }
      surface->data[n + x*4 + 0] = r;
      surface->data[n + x*4 + 1] = g;
      surface->data[n + x*4 + 2] = b;
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 0; y < surface->height; y++) {
      n = ((y*surface->width)<<2) + 3;
      a = surface->data[n];
      for (x = 0; x < surface->width-1; x++) {
        surface->data[n + x*4] = surface->data[n + (x+1)*4];
      }
      surface->data[n + x*4] = a;
    }
  }
}


void common_shift_right(struct td_surf *surface) {

  unsigned char r, g, b, a;
  int x, y, n;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < surface->height; y++) {
      n = (y*surface->width)<<2;
      r = surface->data[n + (surface->width-1)*4 + 0];
      g = surface->data[n + (surface->width-1)*4 + 1];
      b = surface->data[n + (surface->width-1)*4 + 2];
      a = surface->data[n + (surface->width-1)*4 + 3];
      for (x = surface->width-1; x > 0; x--) {
        surface->data[n + x*4 + 0] = surface->data[n + (x-1)*4 + 0];
        surface->data[n + x*4 + 1] = surface->data[n + (x-1)*4 + 1];
        surface->data[n + x*4 + 2] = surface->data[n + (x-1)*4 + 2];
        surface->data[n + x*4 + 3] = surface->data[n + (x-1)*4 + 3];
      }
      surface->data[n + 0] = r;
      surface->data[n + 1] = g;
      surface->data[n + 2] = b;
      surface->data[n + 3] = a;
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 0; y < surface->height; y++) {
      n = (y*surface->width)<<2;
      r = surface->data[n + (surface->width-1)*4 + 0];
      g = surface->data[n + (surface->width-1)*4 + 1];
      b = surface->data[n + (surface->width-1)*4 + 2];
      for (x = surface->width-1; x > 0; x--) {
        surface->data[n + x*4 + 0] = surface->data[n + (x-1)*4 + 0];
        surface->data[n + x*4 + 1] = surface->data[n + (x-1)*4 + 1];
        surface->data[n + x*4 + 2] = surface->data[n + (x-1)*4 + 2];
      }
      surface->data[n + 0] = r;
      surface->data[n + 1] = g;
      surface->data[n + 2] = b;
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 0; y < surface->height; y++) {
      n = ((y*surface->width)<<2) + 3;
      a = surface->data[n + (surface->width-1)*4];
      for (x = surface->width-1; x > 0; x--) {
        surface->data[n + x*4] = surface->data[n + (x-1)*4];
      }
      surface->data[n] = a;
    }
  }
}


void common_shift_up(struct td_surf *surface) {

  unsigned char r, g, b, a;
  int x, y, n;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (x = 0; x < surface->width; x++) {
      n = x<<2;
      r = surface->data[n + 0];
      g = surface->data[n + 1];
      b = surface->data[n + 2];
      a = surface->data[n + 3];
      for (y = 0; y < surface->height-1; y++) {
        surface->data[y*surface->width*4 + n + 0] = surface->data[(y+1)*surface->width*4 + n + 0];
        surface->data[y*surface->width*4 + n + 1] = surface->data[(y+1)*surface->width*4 + n + 1];
        surface->data[y*surface->width*4 + n + 2] = surface->data[(y+1)*surface->width*4 + n + 2];
        surface->data[y*surface->width*4 + n + 3] = surface->data[(y+1)*surface->width*4 + n + 3];
      }
      surface->data[y*surface->width*4 + n + 0] = r;
      surface->data[y*surface->width*4 + n + 1] = g;
      surface->data[y*surface->width*4 + n + 2] = b;
      surface->data[y*surface->width*4 + n + 3] = a;
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (x = 0; x < surface->width; x++) {
      n = x<<2;
      r = surface->data[n + 0];
      g = surface->data[n + 1];
      b = surface->data[n + 2];
      for (y = 0; y < surface->height-1; y++) {
        surface->data[y*surface->width*4 + n + 0] = surface->data[(y+1)*surface->width*4 + n + 0];
        surface->data[y*surface->width*4 + n + 1] = surface->data[(y+1)*surface->width*4 + n + 1];
        surface->data[y*surface->width*4 + n + 2] = surface->data[(y+1)*surface->width*4 + n + 2];
      }
      surface->data[y*surface->width*4 + n + 0] = r;
      surface->data[y*surface->width*4 + n + 1] = g;
      surface->data[y*surface->width*4 + n + 2] = b;
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (x = 0; x < surface->width; x++) {
      n = (x<<2) + 3;
      a = surface->data[n];
      for (y = 0; y < surface->height-1; y++) {
        surface->data[y*surface->width*4 + n] = surface->data[(y+1)*surface->width*4 + n];
      }
      surface->data[y*surface->width*4 + n] = a;
    }
  }
}


void common_shift_down(struct td_surf *surface) {

  unsigned char r, g, b, a;
  int x, y, n;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (x = 0; x < surface->width; x++) {
      n = x<<2;
      r = surface->data[(surface->height-1)*surface->width*4 + n + 0];
      g = surface->data[(surface->height-1)*surface->width*4 + n + 1];
      b = surface->data[(surface->height-1)*surface->width*4 + n + 2];
      a = surface->data[(surface->height-1)*surface->width*4 + n + 3];
      for (y = surface->height-1; y > 0; y--) {
        surface->data[y*surface->width*4 + n + 0] = surface->data[(y-1)*surface->width*4 + n + 0];
        surface->data[y*surface->width*4 + n + 1] = surface->data[(y-1)*surface->width*4 + n + 1];
        surface->data[y*surface->width*4 + n + 2] = surface->data[(y-1)*surface->width*4 + n + 2];
        surface->data[y*surface->width*4 + n + 3] = surface->data[(y-1)*surface->width*4 + n + 3];
      }
      surface->data[n + 0] = r;
      surface->data[n + 1] = g;
      surface->data[n + 2] = b;
      surface->data[n + 3] = a;
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (x = 0; x < surface->width; x++) {
      n = x<<2;
      r = surface->data[(surface->height-1)*surface->width*4 + n + 0];
      g = surface->data[(surface->height-1)*surface->width*4 + n + 1];
      b = surface->data[(surface->height-1)*surface->width*4 + n + 2];
      for (y = surface->height-1; y > 0; y--) {
        surface->data[y*surface->width*4 + n + 0] = surface->data[(y-1)*surface->width*4 + n + 0];
        surface->data[y*surface->width*4 + n + 1] = surface->data[(y-1)*surface->width*4 + n + 1];
        surface->data[y*surface->width*4 + n + 2] = surface->data[(y-1)*surface->width*4 + n + 2];
      }
      surface->data[n + 0] = r;
      surface->data[n + 1] = g;
      surface->data[n + 2] = b;
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (x = 0; x < surface->width; x++) {
      n = (x<<2) + 3;
      a = surface->data[(surface->height-1)*surface->width*4 + n];
      for (y = surface->height-1; y > 0; y--) {
        surface->data[y*surface->width*4 + n] = surface->data[(y-1)*surface->width*4 + n];
      }
      surface->data[n] = a;
    }
  }
}


void common_draw_blur_normal(unsigned int x, unsigned int y, struct td_surf *surface) {

  unsigned i, a, b;
  float cr, cg, cb, ca;

  cr = cg = cb = ca = 0.0f;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (a = 0; a < 3; a++) {
      for (b = 0; b < 3; b++) {
        i = (((y+a-1)%surface->height)*surface->width + ((x+b-1)%surface->width))<<2;
        cr += surface->data[i + 0];
        cg += surface->data[i + 1];
        cb += surface->data[i + 2];
        ca += surface->data[i + 3];
      }
    }
    surface->data[(y*surface->width + x)*4 + 0] = cr/9.0f;
    surface->data[(y*surface->width + x)*4 + 1] = cg/9.0f;
    surface->data[(y*surface->width + x)*4 + 2] = cb/9.0f;
    surface->data[(y*surface->width + x)*4 + 3] = ca/9.0f;
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (a = 0; a < 3; a++) {
      for (b = 0; b < 3; b++) {
        i = (((y+a-1)%surface->height)*surface->width + ((x+b-1)%surface->width))<<2;
        cr += surface->data[i + 0];
        cg += surface->data[i + 1];
        cb += surface->data[i + 2];
      }
    }
    surface->data[(y*surface->width + x)*4 + 0] = cr/9.0f;
    surface->data[(y*surface->width + x)*4 + 1] = cg/9.0f;
    surface->data[(y*surface->width + x)*4 + 2] = cb/9.0f;
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (a = 0; a < 3; a++) {
      for (b = 0; b < 3; b++) {
        i = (((y+a-1)%surface->height)*surface->width + ((x+b-1)%surface->width))<<2;
        ca += surface->data[i + 3];
      }
    }
    surface->data[(y*surface->width + x)*4 + 3] = ca/9.0f;
  }
}


void common_draw_blur_gaussian(unsigned int x, unsigned int y, struct td_surf *surface) {

  unsigned i, a, b, n;
  float cr, cg, cb, ca;
  float filter[3*3] = { 1.0/36.0, 1.0/9.0, 1.0/36.0,
    1.0/9.0, 4.0/9.0, 1.0/9.0,
    1.0/36.0, 1.0/9.0, 1.0/36.0 };


  cr = cg = cb = ca = 0.0f;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (n = 0, a = 0; a < 3; a++) {
      for (b = 0; b < 3; b++, n++) {
        i = (((y+a-1)%surface->height)*surface->width + ((x+b-1)%surface->width))<<2;
        cr += surface->data[i + 0]*filter[n];
        cg += surface->data[i + 1]*filter[n];
        cb += surface->data[i + 2]*filter[n];
        ca += surface->data[i + 3]*filter[n];
      }
    }
    surface->data[(y*surface->width + x)*4 + 0] = cr;
    surface->data[(y*surface->width + x)*4 + 1] = cg;
    surface->data[(y*surface->width + x)*4 + 2] = cb;
    surface->data[(y*surface->width + x)*4 + 3] = ca;
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (n = 0, a = 0; a < 3; a++) {
      for (b = 0; b < 3; b++, n++) {
        i = (((y+a-1)%surface->height)*surface->width + ((x+b-1)%surface->width))<<2;
        cr += surface->data[i + 0]*filter[n];
        cg += surface->data[i + 1]*filter[n];
        cb += surface->data[i + 2]*filter[n];
      }
    }
    surface->data[(y*surface->width + x)*4 + 0] = cr;
    surface->data[(y*surface->width + x)*4 + 1] = cg;
    surface->data[(y*surface->width + x)*4 + 2] = cb;
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (n = 0, a = 0; a < 3; a++) {
      for (b = 0; b < 3; b++, n++) {
        i = (((y+a-1)%surface->height)*surface->width + ((x+b-1)%surface->width))<<2;
        ca += surface->data[i + 3]*filter[n];
      }
    }
    surface->data[(y*surface->width + x)*4 + 3] = ca;
  }
}


static struct td_color floodfill_base_color, floodfill_fill_color;
static struct td_surf floodfill_surface;


void _common_floodfill(int x, int y) {

  int i, n;

  i = 0;
  n = (y*floodfill_surface.width + x)<<2;

  if (floodfill_surface.viewmode == VIEW_MODE_RGBA) {
    if (floodfill_surface.data[n + 0] == floodfill_base_color.r &&
        floodfill_surface.data[n + 1] == floodfill_base_color.g &&
        floodfill_surface.data[n + 2] == floodfill_base_color.b &&
        floodfill_surface.data[n + 3] == floodfill_base_color.a)
      i = 1;
  }
  else if (floodfill_surface.viewmode == VIEW_MODE_RGB) {
    if (floodfill_surface.data[n + 0] == floodfill_base_color.r &&
        floodfill_surface.data[n + 1] == floodfill_base_color.g &&
        floodfill_surface.data[n + 2] == floodfill_base_color.b)
      i = 1;
  }
  else if (floodfill_surface.viewmode == VIEW_MODE_ALPHA) {
    if (floodfill_surface.data[n + 3] == floodfill_base_color.a)
      i = 1;
  }

  if (i == 1) {
    /* fill the pixel */
    if (floodfill_surface.viewmode == VIEW_MODE_RGBA) {
      floodfill_surface.data[n + 0] = floodfill_fill_color.r;
      floodfill_surface.data[n + 1] = floodfill_fill_color.g;
      floodfill_surface.data[n + 2] = floodfill_fill_color.b;
      floodfill_surface.data[n + 3] = floodfill_fill_color.a;
    }
    else if (floodfill_surface.viewmode == VIEW_MODE_RGB) {
      floodfill_surface.data[n + 0] = floodfill_fill_color.r;
      floodfill_surface.data[n + 1] = floodfill_fill_color.g;
      floodfill_surface.data[n + 2] = floodfill_fill_color.b;
    }
    else if (floodfill_surface.viewmode == VIEW_MODE_ALPHA) {
      floodfill_surface.data[n + 3] = floodfill_fill_color.a;
    }

    /* try to fill the neighboring pixels */
    if (x - 1 >= 0)
      _common_floodfill(x - 1, y);
    if (y - 1 >= 0)
      _common_floodfill(x, y - 1);
    if (x + 1 < floodfill_surface.width)
      _common_floodfill(x + 1, y);
    if (y + 1 < floodfill_surface.height)
      _common_floodfill(x, y + 1);
  }
}


void common_draw_fill(int x, int y, struct td_color *col, struct td_surf *surface) {

  int i = (y*surface->width + x) * 4;

  floodfill_base_color.r = surface->data[i + 0];
  floodfill_base_color.g = surface->data[i + 1];
  floodfill_base_color.b = surface->data[i + 2];
  floodfill_base_color.a = surface->data[i + 3];

  floodfill_fill_color = *col;

  floodfill_surface = *surface;

  _common_floodfill(x, y);
}


#define AGGRESSIVE_FILL_DISTANCE 10


void _common_aggressive_floodfill(int x, int y) {

  int i, n, dR, dG, dB, dA;

  i = 0;
  n = (y*floodfill_surface.width + x)<<2;

  dR = floodfill_surface.data[n + 0] - floodfill_base_color.r;
  if (dR < 0)
    dR = -dR;
  dG = floodfill_surface.data[n + 1] - floodfill_base_color.g;
  if (dG < 0)
    dG = -dG;
  dB = floodfill_surface.data[n + 2] - floodfill_base_color.b;
  if (dB < 0)
    dB = -dB;
  dA = floodfill_surface.data[n + 3] - floodfill_base_color.a;
  if (dA < 0)
    dA = -dA;

  if (floodfill_surface.viewmode == VIEW_MODE_RGBA) {
    if (dR < AGGRESSIVE_FILL_DISTANCE && dG < AGGRESSIVE_FILL_DISTANCE && dB < AGGRESSIVE_FILL_DISTANCE && dA < AGGRESSIVE_FILL_DISTANCE)
      i = 1;
  }
  else if (floodfill_surface.viewmode == VIEW_MODE_RGB) {
    if (dR < AGGRESSIVE_FILL_DISTANCE && dG < AGGRESSIVE_FILL_DISTANCE && dB < AGGRESSIVE_FILL_DISTANCE)
      i = 1;
  }
  else if (floodfill_surface.viewmode == VIEW_MODE_ALPHA) {
    if (dA < AGGRESSIVE_FILL_DISTANCE)
      i = 1;
  }

  if (i == 1) {
    /* fill the pixel */
    if (floodfill_surface.viewmode == VIEW_MODE_RGBA) {
      floodfill_surface.data[n + 0] = floodfill_fill_color.r;
      floodfill_surface.data[n + 1] = floodfill_fill_color.g;
      floodfill_surface.data[n + 2] = floodfill_fill_color.b;
      floodfill_surface.data[n + 3] = floodfill_fill_color.a;
    }
    else if (floodfill_surface.viewmode == VIEW_MODE_RGB) {
      floodfill_surface.data[n + 0] = floodfill_fill_color.r;
      floodfill_surface.data[n + 1] = floodfill_fill_color.g;
      floodfill_surface.data[n + 2] = floodfill_fill_color.b;
    }
    else if (floodfill_surface.viewmode == VIEW_MODE_ALPHA) {
      floodfill_surface.data[n + 3] = floodfill_fill_color.a;
    }

    /* try to fill the neighboring pixels */
    if (x - 1 >= 0)
      _common_aggressive_floodfill(x - 1, y);
    if (y - 1 >= 0)
      _common_aggressive_floodfill(x, y - 1);
    if (x + 1 < floodfill_surface.width)
      _common_aggressive_floodfill(x + 1, y);
    if (y + 1 < floodfill_surface.height)
      _common_aggressive_floodfill(x, y + 1);
  }
}


void common_draw_aggressive_fill(int x, int y, struct td_color *col, struct td_surf *surface) {

  int i = (y*surface->width + x) * 4;

  floodfill_base_color.r = surface->data[i + 0];
  floodfill_base_color.g = surface->data[i + 1];
  floodfill_base_color.b = surface->data[i + 2];
  floodfill_base_color.a = surface->data[i + 3];

  floodfill_fill_color = *col;

  floodfill_surface = *surface;

  _common_aggressive_floodfill(x, y);
}


#define GRADIENT_FILL_DISTANCE 8


void _common_gradient_floodfill(int x, int y, int oR, int oG, int oB, int oA) {

  int i, n, dR, dG, dB, dA;

  i = 0;
  n = (y*floodfill_surface.width + x)<<2;

  dR = floodfill_surface.data[n + 0] - oR;
  if (dR < 0)
    dR = -dR;
  dG = floodfill_surface.data[n + 1] - oG;
  if (dG < 0)
    dG = -dG;
  dB = floodfill_surface.data[n + 2] - oB;
  if (dB < 0)
    dB = -dB;
  dA = floodfill_surface.data[n + 3] - oA;
  if (dA < 0)
    dA = -dA;

  if (floodfill_surface.viewmode == VIEW_MODE_RGBA) {
    if (dR < GRADIENT_FILL_DISTANCE && dG < GRADIENT_FILL_DISTANCE && dB < GRADIENT_FILL_DISTANCE && dA < GRADIENT_FILL_DISTANCE)
      i = 1;
  }
  else if (floodfill_surface.viewmode == VIEW_MODE_RGB) {
    if (dR < GRADIENT_FILL_DISTANCE && dG < GRADIENT_FILL_DISTANCE && dB < GRADIENT_FILL_DISTANCE)
      i = 1;
  }
  else if (floodfill_surface.viewmode == VIEW_MODE_ALPHA) {
    if (dA < GRADIENT_FILL_DISTANCE)
      i = 1;
  }

  if (i == 1) {
    /* fill the pixel */
    dR = floodfill_surface.data[n + 0];
    dG = floodfill_surface.data[n + 1];
    dB = floodfill_surface.data[n + 2];
    dA = floodfill_surface.data[n + 3];

    if (floodfill_surface.viewmode == VIEW_MODE_RGBA) {
      floodfill_surface.data[n + 0] = floodfill_fill_color.r;
      floodfill_surface.data[n + 1] = floodfill_fill_color.g;
      floodfill_surface.data[n + 2] = floodfill_fill_color.b;
      floodfill_surface.data[n + 3] = floodfill_fill_color.a;
    }
    else if (floodfill_surface.viewmode == VIEW_MODE_RGB) {
      floodfill_surface.data[n + 0] = floodfill_fill_color.r;
      floodfill_surface.data[n + 1] = floodfill_fill_color.g;
      floodfill_surface.data[n + 2] = floodfill_fill_color.b;
    }
    else if (floodfill_surface.viewmode == VIEW_MODE_ALPHA) {
      floodfill_surface.data[n + 3] = floodfill_fill_color.a;
    }

    /* try to fill the neighboring pixels */
    if (x - 1 >= 0)
      _common_gradient_floodfill(x - 1, y, dR, dG, dB, dA);
    if (y - 1 >= 0)
      _common_gradient_floodfill(x, y - 1, dR, dG, dB, dA);
    if (x + 1 < floodfill_surface.width)
      _common_gradient_floodfill(x + 1, y, dR, dG, dB, dA);
    if (y + 1 < floodfill_surface.height)
      _common_gradient_floodfill(x, y + 1, dR, dG, dB, dA);
  }
}


void common_draw_gradient_fill(int x, int y, struct td_color *col, struct td_surf *surface) {

  int i = (y*surface->width + x) * 4;

  int r = surface->data[i + 0];
  int g = surface->data[i + 1];
  int b = surface->data[i + 2];
  int a = surface->data[i + 3];

  floodfill_fill_color = *col;

  floodfill_surface = *surface;

  _common_gradient_floodfill(x, y, r, g, b, a);
}


void common_copy_data_to_view(struct td_surf *surface) {

  int x, y, a, b, z3, ip, op, ap, xz, yz, dxz3;
  float a1, a2, a3, a4, bg;
  unsigned char c, cr, cg, cb;


  z3 = surface->zoom*3;
  dxz3 = surface->width*z3;

  if (surface->viewmode == VIEW_MODE_RGB) {
    /* render RGB */
    for (y = 0; y < surface->height; y++) {
      for (x = 0; x < surface->width; x++) {
        ip = (y*surface->width + x)<<2;
        op = (y*surface->zoom*surface->width + x)*z3;
        for (a = 0; a < surface->zoom; a++) {
          ap = op + a*dxz3;
          for (b = 0; b < surface->zoom; b++) {
            surface->view[ap++] = surface->data[ip + 0];
            surface->view[ap++] = surface->data[ip + 1];
            surface->view[ap++] = surface->data[ip + 2];
          }
        }
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    /* render only the alpha channel */
    for (y = 0; y < surface->height; y++) {
      for (x = 0; x < surface->width; x++) {
        ip = (y*surface->width + x)<<2;
        c = surface->data[ip + 3];
        op = (y*surface->zoom*surface->width + x)*z3;
        for (a = 0; a < surface->zoom; a++) {
          ap = op + a*dxz3;
          for (b = 0; b < surface->zoom; b++) {
            surface->view[ap++] = c;
            surface->view[ap++] = c;
            surface->view[ap++] = c;
          }
        }
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGBA) {
    /* render RGBA */
    yz = 0;
    for (y = 0; y < surface->height; y++) {
      xz = 0;
      ip = (y*surface->width) << 2;
      for (x = 0; x < surface->width; x++) {
        a1 = ((float)surface->data[ip + 3])/255.0f;
        a2 = 1.0f - a1;
        a3 = 128.0f*a2;
        a4 = 192.0f*a2;
        op = (yz*surface->width + x)*z3;
        cr = ((float)surface->data[ip + 0])*a1;
        cg = ((float)surface->data[ip + 1])*a1;
        cb = ((float)surface->data[ip + 2])*a1;
        for (a = 0; a < surface->zoom; a++) {
          ap = op + a*dxz3;
          for (b = 0; b < surface->zoom; b++) {
            /* compute the background color */
            if ((yz+a) & 8) {
              if ((xz+b) & 8)
                bg = a3;
              else
                bg = a4;
            }
            else {
              if ((xz+b) & 8)
                bg = a4;
              else
                bg = a3;
            }

            surface->view[ap++] = cr + bg;
            surface->view[ap++] = cg + bg;
            surface->view[ap++] = cb + bg;
          }
        }
        xz += surface->zoom;
        ip += 4;
      }
      yz += surface->zoom;
    }
  }
}


void common_copy_pixel_to_view(int x, int y, struct td_surf *surface) {

  int a, b, z3, ip, op, ap, xz, yz, dxz3;
  float a1, a2, a3, a4, bg;
  unsigned char c, cr, cg, cb;


  z3 = surface->zoom*3;
  dxz3 = surface->width*z3;

  if (surface->viewmode == VIEW_MODE_RGB) {
    /* render RGB */
    ip = (y*surface->width + x)<<2;
    op = (y*surface->zoom*surface->width + x)*z3;
    for (a = 0; a < surface->zoom; a++) {
      ap = op + a*dxz3;
      for (b = 0; b < surface->zoom; b++) {
        surface->view[ap++] = surface->data[ip + 0];
        surface->view[ap++] = surface->data[ip + 1];
        surface->view[ap++] = surface->data[ip + 2];
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    /* render only the alpha channel */
    ip = (y*surface->width + x)<<2;
    c = surface->data[ip + 3];
    op = (y*surface->zoom*surface->width + x)*z3;
    for (a = 0; a < surface->zoom; a++) {
      ap = op + a*dxz3;
      for (b = 0; b < surface->zoom; b++) {
        surface->view[ap++] = c;
        surface->view[ap++] = c;
        surface->view[ap++] = c;
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGBA) {
    /* render RGBA */
    yz = y*surface->zoom;
    xz = x*surface->zoom;
    ip = (y*surface->width + x)<<2;
    a1 = ((float)surface->data[ip + 3])/255.0f;
    a2 = 1.0f - a1;
    a3 = 128.0f*a2;
    a4 = 192.0f*a2;
    op = (yz*surface->width + x)*z3;
    cr = ((float)surface->data[ip + 0])*a1;
    cg = ((float)surface->data[ip + 1])*a1;
    cb = ((float)surface->data[ip + 2])*a1;
    for (a = 0; a < surface->zoom; a++) {
      ap = op + a*dxz3;
      for (b = 0; b < surface->zoom; b++) {
        /* compute the background color */
        if ((yz+a) & 8) {
          if ((xz+b) & 8)
            bg = a3;
          else
            bg = a4;
        }
        else {
          if ((xz+b) & 8)
            bg = a4;
          else
            bg = a3;
        }

        surface->view[ap++] = cr + bg;
        surface->view[ap++] = cg + bg;
        surface->view[ap++] = cb + bg;
      }
    }
  }
}


void common_copy_block_to_view(int ax, int ay, int mx, int my, struct td_surf *surface) {

  int x, y, a, b, z3, ip, op, ap, xz, yz, dxz3;
  float a1, a2, a3, a4, bg;
  unsigned char c, cr, cg, cb;


  z3 = surface->zoom*3;
  dxz3 = surface->width*z3;

  mx += ax;
  my += ay;

  if (surface->viewmode == VIEW_MODE_RGB) {
    /* render RGB */
    for (y = ay; y < my; y++) {
      for (x = ax; x < mx; x++) {
        ip = (y*surface->width + x)<<2;
        op = (y*surface->zoom*surface->width + x)*z3;
        for (a = 0; a < surface->zoom; a++) {
          ap = op + a*dxz3;
          for (b = 0; b < surface->zoom; b++) {
            surface->view[ap++] = surface->data[ip + 0];
            surface->view[ap++] = surface->data[ip + 1];
            surface->view[ap++] = surface->data[ip + 2];
          }
        }
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    /* render only the alpha channel */
    for (y = ay; y < my; y++) {
      for (x = ax; x < mx; x++) {
        ip = (y*surface->width + x)<<2;
        c = surface->data[ip + 3];
        op = (y*surface->zoom*surface->width + x)*z3;
        for (a = 0; a < surface->zoom; a++) {
          ap = op + a*dxz3;
          for (b = 0; b < surface->zoom; b++) {
            surface->view[ap++] = c;
            surface->view[ap++] = c;
            surface->view[ap++] = c;
          }
        }
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGBA) {
    /* render RGBA */
    for (y = ay; y < my; y++) {
      yz = y*surface->zoom;
      for (x = ax; x < mx; x++) {
        xz = x*surface->zoom;
        ip = (y*surface->width + x)<<2;
        a1 = ((float)surface->data[ip + 3])/255.0f;
        a2 = 1.0f - a1;
        a3 = 128.0f*a2;
        a4 = 192.0f*a2;
        op = (yz*surface->width + x)*z3;
        cr = ((float)surface->data[ip + 0])*a1;
        cg = ((float)surface->data[ip + 1])*a1;
        cb = ((float)surface->data[ip + 2])*a1;
        for (a = 0; a < surface->zoom; a++) {
          ap = op + a*dxz3;
          for (b = 0; b < surface->zoom; b++) {
            /* compute the background color */
            if ((yz+a) & 8) {
              if ((xz+b) & 8)
                bg = a3;
              else
                bg = a4;
            }
            else {
              if ((xz+b) & 8)
                bg = a4;
              else
                bg = a3;
            }

            surface->view[ap++] = cr + bg;
            surface->view[ap++] = cg + bg;
            surface->view[ap++] = cb + bg;
          }
        }
      }
    }
  }
}


void common_gtk_exit(GtkWidget *widget, gpointer data) {

  /* display a warning if the image has not been saved */
  if (editwin.need_to_save == YES) {
    gtk_widget_show_all(exit_dialog);
    return;
  }

  /* save the prefs before quitting */
  prefs_save();

  gtk_main_quit();
}


gint common_gtk_hide_widget(GtkWidget *widget, gpointer data) {

  gtk_widget_hide(widget);

  return TRUE;
}


int common_copy_image(unsigned char *out, int ox, int oy, unsigned char *in, int ix, int iy) {

  int x, y, z, i, n, ex, ey;


  if (ox < ix)
    ex = ox;
  else
    ex = ix;

  if (oy < iy)
    ey = oy;
  else
    ey = iy;

  /* first clear the output */
  for (x = 0, i = 0; x < ox*oy; x++) {
    out[i++] = editwin.pen_bg.r;
    out[i++] = editwin.pen_bg.g;
    out[i++] = editwin.pen_bg.b;
    out[i++] = editwin.pen_bg.a;
  }

  /* then copy as much as possible */
  for (y = 0, i = 0; y < ey; y++) {
    n = i;
    z = (y*ix) << 2;
    for (x = 0; x < ex; x++) {
      out[n++] = in[z + 0];
      out[n++] = in[z + 1];
      out[n++] = in[z + 2];
      out[n++] = in[z + 3];
      z += 4;
    }
    i += ox<<2;
  }

  return SUCCEEDED;
}


int common_reformat_image(int *dx, int *dy, int *bpp, unsigned char **d) {

  unsigned char *n;
  int w, h, x, y, i, p, k;


  if (dx == NULL || dy == NULL || bpp == NULL || d == NULL)
    return FAILED;

  if (*bpp == 3) {
    /* add the alpha channel */
    n = malloc((*dx)*(*dy)*4);
    if (n == NULL) {
      fprintf(stderr, "COMMON_REFORMAT_IMAGE: Out of memory error.\n");
      return FAILED;
    }

    for (k = 0, p = 0, i = 0; i < (*dx)*(*dy); i++) {
      n[p++] = (*d)[k++];
      n[p++] = (*d)[k++];
      n[p++] = (*d)[k++];
      n[p++] = 255;
    }

    free(*d);
    *d = n;
  }

  /* resize the image, if it's not of suitable size */
  if (*dx != *dy || !(*dx == 64 || *dx == 128 || *dx == 256 || *dx == 512 || *dx == 1024 || *dx == 2048)) {
    if (*dx <= 64)
      w = 64;
    else if (*dx <= 128)
      w = 128;
    else if (*dx <= 256)
      w = 256;
    else if (*dx <= 512)
      w = 512;
    else if (*dx <= 1024)
      w = 1024;
    else
      w = 2048;

    if (*dy <= 64)
      h = 64;
    else if (*dy <= 128)
      h = 128;
    else if (*dy <= 256)
      h = 256;
    else if (*dy <= 512)
      h = 512;
    else if (*dy <= 1024)
      h = 1024;
    else
      h = 2048;

    if (w != h) {
      if (w > h)
        h = w;
      else
        w = h;
    }

    n = malloc(w*h*4);
    if (n == NULL) {
      fprintf(stderr, "COMMON_REFORMAT_IMAGE: Out of memory error.\n");
      return FAILED;
    }

    /* clear the output buffer */
    for (i = 0; i < w*h*4; ) {
      n[i++] = editwin.pen_bg.r;
      n[i++] = editwin.pen_bg.g;
      n[i++] = editwin.pen_bg.b;
      n[i++] = editwin.pen_bg.a;
    }

    /* copy the image data */
    for (y = 0; y < h && y < *dy; y++) {
      for (x = 0; x < w && x < *dx; x++) {
        n[(y*w + x)*4 + 0] = (*d)[(y*(*dx) + x)*4 + 0];
        n[(y*w + x)*4 + 1] = (*d)[(y*(*dx) + x)*4 + 1];
        n[(y*w + x)*4 + 2] = (*d)[(y*(*dx) + x)*4 + 2];
        n[(y*w + x)*4 + 3] = (*d)[(y*(*dx) + x)*4 + 3];
      }
    }

    free(*d);
    *d = n;
    *dx = w;
    *dy = h;
  }

  return SUCCEEDED;
}


int common_get_path(char *filename, char *path) {

  int i;


  if (filename == NULL || path == NULL)
    return FAILED;

  /* strip the filename from the whole path */
  strcpy(path, filename);

  for (i = strlen(filename)-1; i >= 0; i--) {
#ifdef WIN32
    if (path[i] == '\\')
#else
      if (path[i] == '/')
#endif
        break;
  }
  path[i+1] = 0;

  return SUCCEEDED;
}


void common_resize_data(int dx, int dy, struct td_color *col, struct td_surf *surface) {

  unsigned char *m;
  int i, ox, oy, x, y;

  /* save the old */
  m = surface->data;
  ox = surface->width;
  oy = surface->height;

  surface->data = malloc(dx*dy*4);
  if (surface->data == NULL) {
    fprintf(stderr, "COMMON_RESIZE_DATA: Out of memory error.\n");
    free(m);
    return;
  }

  surface->width = dx;
  surface->height = dy;

  /* clear the new image */
  for (i = 0; i < surface->width*surface->height*4; i += 4) {
    surface->data[i + 0] = col->r;
    surface->data[i + 1] = col->g;
    surface->data[i + 2] = col->b;
    surface->data[i + 3] = col->a;
  }

  /* copy as much from the old image as possible */
  if (m != NULL) {
    for (y = 0; y < oy && y < dy; y++) {
      for (x = 0; x < ox && x < dx; x++) {
        surface->data[(y*dx + x)*4 + 0] = m[(y*ox + x)*4 + 0];
        surface->data[(y*dx + x)*4 + 1] = m[(y*ox + x)*4 + 1];
        surface->data[(y*dx + x)*4 + 2] = m[(y*ox + x)*4 + 2];
        surface->data[(y*dx + x)*4 + 3] = m[(y*ox + x)*4 + 3];
      }
    }
    free(m);
  }
}


int common_scale(int i, struct td_surf *surface) {

  static unsigned char *tmp = NULL;
  static int tmp_size = 0;
  int x, y, dx, dy, a, b;
  float fr, fg, fb, fa;


  /* manage the tmp buffer */
  if (tmp_size < surface->width*surface->height*4) {
    tmp_size = surface->width*surface->height*4;
    tmp = realloc(tmp, tmp_size);
    if (tmp == NULL) {
      fprintf(stderr, "COMMON_SCALE: Out of memory error.\n");
      tmp_size = 0;
      return FAILED;
    }
  }

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (a = 0; a < surface->width*surface->height*4; a += 4) {
      tmp[a + 0] = editwin.pen_bg.r;
      tmp[a + 1] = editwin.pen_bg.g;
      tmp[a + 2] = editwin.pen_bg.b;
      tmp[a + 3] = editwin.pen_bg.a;
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (a = 0; a < surface->width*surface->height*4; a += 4) {
      tmp[a + 0] = editwin.pen_bg.r;
      tmp[a + 1] = editwin.pen_bg.g;
      tmp[a + 2] = editwin.pen_bg.b;
      tmp[a + 3] = surface->data[a + 3];
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (a = 0; a < surface->width*surface->height*4; a += 4) {
      tmp[a + 0] = surface->data[a + 0];
      tmp[a + 1] = surface->data[a + 1];
      tmp[a + 2] = surface->data[a + 2];
      tmp[a + 3] = editwin.pen_bg.a;
    }
  }

  dx = surface->width / i;
  dy = surface->height / i;

  if (surface->viewmode == VIEW_MODE_RGBA) {
    for (y = 0; y < dy; y++) {
      for (x = 0; x < dx; x++) {
        fr = fg = fb = fa = 0.0f;
        for (a = 0; a < i; a++) {
          for (b = 0; b < i; b++) {
            fr += surface->data[(y*i+a)*surface->width*4 + (x*i+b)*4 + 0];
            fg += surface->data[(y*i+a)*surface->width*4 + (x*i+b)*4 + 1];
            fb += surface->data[(y*i+a)*surface->width*4 + (x*i+b)*4 + 2];
            fa += surface->data[(y*i+a)*surface->width*4 + (x*i+b)*4 + 3];
          }
        }
        tmp[y*surface->width*4 + x*4 + 0] = fr/(float)(i*i);
        tmp[y*surface->width*4 + x*4 + 1] = fg/(float)(i*i);
        tmp[y*surface->width*4 + x*4 + 2] = fb/(float)(i*i);
        tmp[y*surface->width*4 + x*4 + 3] = fa/(float)(i*i);
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_RGB) {
    for (y = 0; y < dy; y++) {
      for (x = 0; x < dx; x++) {
        fr = fg = fb = fa = 0.0f;
        for (a = 0; a < i; a++) {
          for (b = 0; b < i; b++) {
            fr += surface->data[(y*i+a)*surface->width*4 + (x*i+b)*4 + 0];
            fg += surface->data[(y*i+a)*surface->width*4 + (x*i+b)*4 + 1];
            fb += surface->data[(y*i+a)*surface->width*4 + (x*i+b)*4 + 2];
          }
        }
        tmp[y*surface->width*4 + x*4 + 0] = fr/(float)(i*i);
        tmp[y*surface->width*4 + x*4 + 1] = fg/(float)(i*i);
        tmp[y*surface->width*4 + x*4 + 2] = fb/(float)(i*i);
      }
    }
  }
  else if (surface->viewmode == VIEW_MODE_ALPHA) {
    for (y = 0; y < dy; y++) {
      for (x = 0; x < dx; x++) {
        fr = fg = fb = fa = 0.0f;
        for (a = 0; a < i; a++) {
          for (b = 0; b < i; b++) {
            fa += surface->data[(y*i+a)*surface->width*4 + (x*i+b)*4 + 3];
          }
        }
        tmp[y*surface->width*4 + x*4 + 3] = fa/(float)(i*i);
      }
    }
  }

  memcpy(surface->data, tmp, surface->height*surface->width*4);

  return SUCCEEDED;
}
