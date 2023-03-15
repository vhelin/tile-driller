
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkwidget.h>
#include <gdk/gdkkeysyms.h>

#include <png.h>
#include <zlib.h>

#include "defines.h"
#include "editor.h"
#include "png.h"



int png_load(char *name, int *dx, int *dy, int *bpp, unsigned char **o) {

  png_structp png_ptr;
  png_infop info_ptr, end_info;
  png_bytep *row_pointers;
  png_uint_32 width, height, i;
  int depth, colortype;
  unsigned char *t;
  FILE *f;


  if (name == NULL)
    return FAILED;

  f = fopen(name, "rb");
  if (f == NULL) {
    fprintf(stderr, "PNG_LOAD: Can't open file \"%s\"\n", name);
    return FAILED;
  }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);
  if (png_ptr == NULL) {
    fclose(f);
    return FAILED;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    fclose(f);
    return FAILED;
  }

  end_info = png_create_info_struct(png_ptr);
  if (end_info == NULL) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(f);
    return FAILED;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(f);
    return FAILED;
  }

  png_init_io(png_ptr, f);
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &depth, &colortype, NULL, NULL, NULL);

  /* palette -> RGB */
  if (colortype == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
    colortype = PNG_COLOR_TYPE_RGB;
    depth = 8;
  }

  /* grayscale -> RGB */
  if (colortype == PNG_COLOR_TYPE_GRAY) {
    png_set_gray_to_rgb(png_ptr);
    colortype = PNG_COLOR_TYPE_RGB;
    depth = 8;
  }

  /* bit depth -> 8 */
  if (depth < 8) {
    png_set_packing(png_ptr);
    depth = 8;
  }

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_set_expand(png_ptr);
    colortype = PNG_COLOR_TYPE_RGBA;
    depth = 8;
  }

  /* scale down to 8 bits per channel */
  if (depth == 16) {
    png_set_strip_16(png_ptr);
    depth = 8;
  }

  /* update transformations */
  png_read_update_info(png_ptr, info_ptr);

  if (colortype == PNG_COLOR_TYPE_RGB)
    *bpp = 3;
  else
    *bpp = 4;

  t = malloc(width*height*(*bpp));
  if (t == NULL) {
    fprintf(stderr, "PNG_LOAD: Out of memory error.\n");
    fclose(f);
    return FAILED;
  }

  /* allocate row pointers and fill them in */
  row_pointers = malloc(height*sizeof(png_bytep));
  if (row_pointers == NULL) {
    fprintf(stderr, "PNG_LOAD: Out of memory error.\n");
    fclose(f);
    free(t);
    return FAILED;
  }

  for (i = 0; i < height; i++)
    row_pointers[i] = t + i*width*(*bpp);

  /* read the image */
  png_read_image(png_ptr, row_pointers);

  png_read_end(png_ptr, end_info);
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  free(row_pointers);

  fclose(f);

  *o = t;
  *dx = width;
  *dy = height;

  return SUCCEEDED;
}


int png_save(char *name, int dx, int dy, unsigned char *o) {

  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep *row_pointers;
  int i;
  FILE *f;


  if (name == NULL || dx <= 0 || dy <= 0)
    return FAILED;

  f = fopen(name, "wb");
  if (f == NULL) {
    fprintf(stderr, "PNG_SAVE: Can't open file \"%s\"\n", name);
    return FAILED;
  }

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);
  if (png_ptr == NULL) {
    fclose(f);
    return FAILED;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    fclose(f);
    return FAILED;
  }

  png_init_io(png_ptr, f);

  /* no filtering */
  png_set_filter(png_ptr, 0, PNG_FILTER_NONE);

  /* select compression level */
  png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
  png_set_compression_mem_level(png_ptr, 8);
  png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
  png_set_compression_window_bits(png_ptr, 15);
  png_set_compression_method(png_ptr, 8);

  png_set_IHDR(png_ptr, info_ptr, dx, dy, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr, info_ptr);

  /* allocate row pointers and fill them in */
  row_pointers = malloc(dy * sizeof(png_bytep));
  for (i = 0; i < dy; i++)
    row_pointers[i] = (png_bytep)o + i*dx*4*sizeof(unsigned char);

  png_write_image(png_ptr, row_pointers);

  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  free(row_pointers);

  fclose(f);

  return SUCCEEDED;
}
