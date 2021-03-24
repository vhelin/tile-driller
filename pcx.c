
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "defines.h"


#define _READ_INT {                                     \
    c = (unsigned char *)&i;                            \
    fscanf(f, "%c%c%c%c", &c[0], &c[1], &c[2], &c[3]);  \
  }

#define _READ_WORD {                            \
    c = (unsigned char *)&i;                    \
    fscanf(f, "%c%c", &c[0], &c[1]);            \
    c[2] = c[3] = 0;                            \
  }


int _pcx_load_1(FILE *f, int bpl, int *dx, int *dy, unsigned char **o) {

  fprintf(stderr, "PCX_LOAD: Monochrome image loading is not supported.\n");
  fclose(f);

  return FAILED;
}


int _pcx_load_2(FILE *f, int bpl, int *dx, int *dy, unsigned char **o) {

  fprintf(stderr, "PCX_LOAD: 2bit image loading is not supported.\n");
  fclose(f);

  return FAILED;
}


int _pcx_load_4(FILE *f, int bpl, int *dx, int *dy, unsigned char **o) {

  fprintf(stderr, "PCX_LOAD: 4bit image loading is not supported.\n");
  fclose(f);

  return FAILED;
}


int _pcx_load_8(FILE *f, int bpl, int *dx, int *dy, unsigned char **o) {

  unsigned char tmp, *t, p[256*3];
  int y, i, b, a, n;


  /* first, acquire the palette */
  i = ftell(f);
  fseek(f, 0, SEEK_END);
  fseek(f, -(3*256+1), SEEK_CUR);
  fscanf(f, "%c", &tmp);

  if (tmp != 12) {
    fprintf(stderr, "_PCX_LOAD_8: Invalid palette header %d.\n", tmp);
    free(*o);
    *o = NULL;
    fclose(f);
    return FAILED;
  }

  fread(p, 1, 3*256, f);
  fseek(f, i, SEEK_SET);

  t = malloc(bpl);
  if (t == NULL) {
    fprintf(stderr, "_PCX_LOAD_8: Out of memory error.\n");
    free(*o);
    *o = NULL;
    fclose(f);
    return FAILED;
  }

  for (a = 0, y = 0; y < *dy; y++) {
    /* decode one line */
    for (b = 0; b < bpl; ) {
      fscanf(f, "%c", &tmp);

      if ((tmp & 0xC0) == 0xC0) {
        i = tmp & 0x3F;
        fscanf(f, "%c", &tmp);
        for (; i > 0; i--)
          t[b++] = tmp;
      }
      else {
        t[b++] = tmp;
      }
    }

    /* one line has been decoded - copy it to the output */
    for (b = 0; b < *dx; b++) {
      n = t[b];
      (*o)[b*3 + a + 0] = p[n*3 + 0];
      (*o)[b*3 + a + 1] = p[n*3 + 1];
      (*o)[b*3 + a + 2] = p[n*3 + 2];
    }

    a += *dx * 3;
  }

  free(t);
  fclose(f);

  return SUCCEEDED;
}


int _pcx_load_24(FILE *f, int bpl, int *dx, int *dy, unsigned char **o) {

  unsigned char tmp, *t;
  int y, i, b, a;


  t = malloc(bpl);
  if (t == NULL) {
    fprintf(stderr, "_PCX_LOAD_24: Out of memory error.\n");
    free(*o);
    *o = NULL;
    fclose(f);
    return FAILED;
  }

  for (a = 0, y = 0; y < *dy; y++) {
    /* decode one line */
    for (b = 0; b < bpl; ) {
      fscanf(f, "%c", &tmp);

      if ((tmp & 0xC0) == 0xC0) {
        i = tmp & 0x3F;
        fscanf(f, "%c", &tmp);
        for (; i > 0; i--)
          t[b++] = tmp;
      }
      else {
        t[b++] = tmp;
      }
    }

    /* one line has been decoded - copy it to the output */
    for (b = 0; b < *dx; b++) {
      (*o)[b*3 + a + 0] = t[b + bpl/3*0];
      (*o)[b*3 + a + 1] = t[b + bpl/3*1];
      (*o)[b*3 + a + 2] = t[b + bpl/3*2];
    }

    a += *dx * 3;
  }

  free(t);
  fclose(f);

  return SUCCEEDED;
}


int pcx_load(char *name, int *dx, int *dy, int *bpp, unsigned char **o) {

  unsigned char tmp[4], *c;
  FILE *f;
  int i, xmin, ymin, xmax, ymax, bpl;


  if (name == NULL || dx == NULL || dy == NULL || bpp == NULL || o == NULL)
    return FAILED;

  *o = NULL;
  *bpp = 3;

  f = fopen(name, "rb");
  if (f == NULL) {
    fprintf(stderr, "PCX_LOAD: Could not open file \"%s\" for reading.\n", name);
    return FAILED;
  }

  /* check the header */
  fscanf(f, "%c%*c%c", &tmp[0], &tmp[1]);

  if (tmp[0] != 0xA) {
    fprintf(stderr, "PCX_LOAD: File \"%s\" is not a PCX image.\n", name);
    fclose(f);
    return FAILED;
  }

  if (tmp[1] != 1) {
    fprintf(stderr, "PCX_LOAD: Encoding scheme %d is not supported.\n", tmp[1]);
    fclose(f);
    return FAILED;
  }

  /* read bits per pixel */
  fscanf(f, "%c", &tmp[0]);

  /* read xmin, ymin, xmax and ymax */
  _READ_WORD;
  xmin = i;
  _READ_WORD;
  ymin = i;
  _READ_WORD;
  xmax = i;
  _READ_WORD;
  ymax = i;

  *dx = xmax - xmin + 1;
  *dy = ymax - ymin + 1;

  *o = malloc((*dx)*(*dy)*3);
  if (*o == NULL) {
    fprintf(stderr, "PCX_LOAD: Out of memory error.\n");
    fclose(f);
    return FAILED;
  }

  /* skip horizontal and vertical dpi */
  fscanf(f, "%*c%*c%*c%*c");

  /* skip palette */
  for (i = 0; i < 48; i++)
    fscanf(f, "%*c");

  /* skip reserved */
  fscanf(f, "%*c");

  /* read the number of color planes */
  fscanf(f, "%c", &tmp[2]);

  /* read bytes per line */
  _READ_WORD;
  bpl = i * tmp[2];

  /* skip palette info, horizontal screen size and vertical screen size */
  _READ_WORD;
  _READ_WORD;
  _READ_WORD;

  /* skip filler */
  for (i = 0; i < 54; i++)
    fscanf(f, "%*c");

  i = tmp[0]*tmp[2];

  if (i == 1)
    return _pcx_load_1(f, bpl, dx, dy, o);
  else if (i == 2)
    return _pcx_load_2(f, bpl, dx, dy, o);
  else if (i == 4)
    return _pcx_load_4(f, bpl, dx, dy, o);
  else if (i == 8)
    return _pcx_load_8(f, bpl, dx, dy, o);
  else if (i == 24)
    return _pcx_load_24(f, bpl, dx, dy, o);

  free(*o);
  *o = NULL;
  fclose(f);

  fprintf(stderr, "PCX_LOAD: %d bits per pixel?\n", i);

  return FAILED;
}
