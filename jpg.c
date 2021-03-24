
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <jpeglib.h>
#include <setjmp.h>

#include "defines.h"



struct my_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */
  jmp_buf setjmp_buffer;        /* for return to caller */
};


METHODDEF(void) my_error_exit(j_common_ptr cinfo) {

  struct my_error_mgr *myerr;


  myerr = (struct my_error_mgr *)cinfo->err;
  (*cinfo->err->output_message)(cinfo);
  longjmp(myerr->setjmp_buffer, 1);
}


int jpg_load(char *name, int *dx, int *dy, int *type, unsigned char **o) {

  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  unsigned char *c, *x;
  JSAMPARRAY buffer;
  int row_stride;
  FILE *f;
  int i;


  if (name == NULL || dx == NULL || dy == NULL || type == NULL || o == NULL)
    return FAILED;

  *o = NULL;

  f = fopen(name, "rb");
  if (f == NULL) {
    fprintf(stderr, "JPG_LOAD: Can't open file \"%s\"\n", name);
    return FAILED;
  }

  /* init the decompress structure */
  memset((void *)&cinfo, 0, sizeof(struct jpeg_decompress_struct));

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    fclose(f);
    return 0;
  }

  /* prepare for decompressing */
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, f);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);
  row_stride = cinfo.output_width * cinfo.output_components;
  buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  /* allocate buffer for output */
  c = malloc(3*cinfo.output_width*cinfo.output_height);
  if (c == NULL) {
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fprintf(stderr, "JPG_LOAD: Out of memory error.\n");
    return FAILED;
  }

  *dx = cinfo.output_width;
  *dy = cinfo.output_height;
  *o = c;
  *type = 3;

  /* decompress the image */
  x = c;
  while (cinfo.output_scanline < cinfo.output_height) {
    jpeg_read_scanlines(&cinfo, buffer, 1);
    /* copy the scanline to output buffer */
    for (i = 0; i < row_stride; i++) {
      *(x++) = (buffer[0])[i];
    }
  }

  /* clean up */
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  fclose(f);

  return SUCCEEDED;
}
