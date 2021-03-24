
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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


static int _tga_load_2(char *name, int *dx, int *dy, int *bpp, unsigned char **o, int id, int cm, FILE *f) {

  unsigned char *c, m, l, tmp[4];
  int i, d, en;


  if (name == NULL || dx == NULL || dy == NULL || bpp == NULL || o == NULL || f == NULL)
    return FAILED;

  /* color map specification */
  fscanf(f, "%*c%*c");
  _READ_WORD;
  en = i;
  fscanf(f, "%c", &m);
  d = (int)m;
  d = d/8;

  /* x origin of the image */
  fscanf(f, "%*c%*c");

  /* y origin of the image */
  fscanf(f, "%*c%*c");

  /* width */
  _READ_WORD;
  *dx = i;

  /* height */
  _READ_WORD;
  *dy = i;

  /* pixel depth */
  fscanf(f, "%c", &l);
  i = (int)l;
  *bpp = i/8;

  /* image descriptor */
  fscanf(f, "%c", &l);

  if ((l & 15) != 8 && (l & 15) != 0) {
    fprintf(stderr, "_TGA_LOAD_2: Alpha channel bits == 8 is the only supported size, not %d.\n", l & 15);
    fclose(f);
    return FAILED;
  }

  *o = malloc((*dx)*(*dy)*(*bpp));
  if (*o == NULL) {
    fprintf(stderr, "_TGA_LOAD_2: Out of memory error.\n");
    fclose(f);
    return FAILED;
  }

  /* skip image id */
  for (i = 0; i < id; i++)
    fscanf(f, "%*c");

  /* skip color map */
  if (cm == 1) {
    for (i = 0; i < en*d; i++)
      fscanf(f, "%*c");
  }

  /* read the image */
  for (i = 0; i < (*dx)*(*dy); i++) {
    if (*bpp == 3) {
      fscanf(f, "%c%c%c", &tmp[2], &tmp[1], &tmp[0]);
      (*o)[i*3 + 0] = tmp[0];
      (*o)[i*3 + 1] = tmp[1];
      (*o)[i*3 + 2] = tmp[2];
    }
    else if (*bpp == 4) {
      fscanf(f, "%c%c%c%c", &tmp[2], &tmp[1], &tmp[0], &tmp[3]);
      (*o)[i*4 + 0] = tmp[0];
      (*o)[i*4 + 1] = tmp[1];
      (*o)[i*4 + 2] = tmp[2];
      (*o)[i*4 + 3] = tmp[3];
    }
  }

  fclose(f);

  return SUCCEEDED;
}


static int _tga_load_10(char *name, int *dx, int *dy, int *bpp, unsigned char **o, int id, int cm, FILE *f) {

  unsigned char *c, m, l, tmp[4], co;
  int i, d, en;


  if (name == NULL || dx == NULL || dy == NULL || bpp == NULL || o == NULL || f == NULL)
    return FAILED;

  /* color map specification */
  fscanf(f, "%*c%*c");
  _READ_WORD;
  en = i;
  fscanf(f, "%c", &m);
  d = (int)m;
  d = d/8;

  /* x origin of the image */
  fscanf(f, "%*c%*c");

  /* y origin of the image */
  fscanf(f, "%*c%*c");

  /* width */
  _READ_WORD;
  *dx = i;

  /* height */
  _READ_WORD;
  *dy = i;

  /* pixel depth */
  fscanf(f, "%c", &l);
  i = (int)l;
  *bpp = i/8;

  /* image descriptor */
  fscanf(f, "%c", &l);

  if ((l & 15) != 8 && (l & 15) != 0) {
    fprintf(stderr, "_TGA_LOAD_10: Alpha channel bits == 8 is the only supported size, not %d.\n", l & 15);
    fclose(f);
    return FAILED;
  }

  *o = malloc((*dx)*(*dy)*(*bpp));
  if (*o == NULL) {
    fprintf(stderr, "_TGA_LOAD_10: Out of memory error.\n");
    fclose(f);
    return FAILED;
  }

  /* skip image id */
  for (i = 0; i < id; i++)
    fscanf(f, "%*c");

  /* skip color map */
  if (cm == 1) {
    for (i = 0; i < en*d; i++)
      fscanf(f, "%*c");
  }

  /* read the image */
  for (i = 0; i < (*dx)*(*dy); ) {
    /* counter */
    fscanf(f, "%c", &co);
    if ((co & 128) == 0) {
      /* raw packet */
      co++;
      for (d = 0; d < co && i < (*dx)*(*dy); d++, i++) {
        if (*bpp == 3) {
          fscanf(f, "%c%c%c", &tmp[2], &tmp[1], &tmp[0]);
          (*o)[i*3 + 0] = tmp[0];
          (*o)[i*3 + 1] = tmp[1];
          (*o)[i*3 + 2] = tmp[2];
        }
        else if (*bpp == 4) {
          fscanf(f, "%c%c%c%c", &tmp[2], &tmp[1], &tmp[0], &tmp[3]);
          (*o)[i*4 + 0] = tmp[0];
          (*o)[i*4 + 1] = tmp[1];
          (*o)[i*4 + 2] = tmp[2];
          (*o)[i*4 + 3] = tmp[3];
        }
      }
    }
    else {
      /* rle packet */
      co = co & 127;
      co++;
      if (*bpp == 3) {
        fscanf(f, "%c%c%c", &tmp[2], &tmp[1], &tmp[0]);
        for (d = 0; d < co && i < (*dx)*(*dy); d++, i++) {
          (*o)[i*3 + 0] = tmp[0];
          (*o)[i*3 + 1] = tmp[1];
          (*o)[i*3 + 2] = tmp[2];
        }
      }
      else if (*bpp == 4) {
        fscanf(f, "%c%c%c%c", &tmp[2], &tmp[1], &tmp[0], &tmp[3]);
        for (d = 0; d < co && i < (*dx)*(*dy); d++, i++) {
          (*o)[i*4 + 0] = tmp[0];
          (*o)[i*4 + 1] = tmp[1];
          (*o)[i*4 + 2] = tmp[2];
          (*o)[i*4 + 3] = tmp[3];
        }
      }
    }
  }

  fclose(f);

  return SUCCEEDED;
}


int tga_load(char *name, int *dx, int *dy, int *bpp, unsigned char **o) {

  char tmp[26];
  int id, cm, it, tga, err;
  FILE *f;


  if (name == NULL || dx == NULL || dy == NULL || bpp == NULL || o == NULL)
    return FAILED;

  *o = NULL;
  tga = NO;
  err = NO;

  f = fopen(name, "rb");
  if (f == NULL) {
    fprintf(stderr, "TGA_LOAD: Could not open file \"%s\" for reading.\n", name);
    return FAILED;
  }

  /* read the footer */
  fseek(f, -26, SEEK_END);
  fread(tmp, 1, 26, f);
  fseek(f, 0, SEEK_SET);

  if (strncmp(&tmp[8], "TRUEVISION-XFILE", 16) == 0)
    tga = YES;

  /* id length */
  fscanf(f, "%c", &tmp[0]);
  id = (int)tmp[0];

  /* color map */
  fscanf(f, "%c", &tmp[0]);
  cm = (int)tmp[0];

  if (!(cm == 0 || cm == 1))
    err = YES;

  /* image type */
  fscanf(f, "%c", &tmp[0]);
  it = (int)tmp[0];

  if (!(it == 0 || it == 1 || it == 2 || it == 3 || it == 9 || it == 10 || it == 11))
    err = YES;

  if (tga == NO && err == YES) {
    fprintf(stderr, "TGA_LOAD: File \"%s\" is not a TGA image.\n", name);
    fclose(f);
    return FAILED;
  }

  if (it == 2)
    return _tga_load_2(name, dx, dy, bpp, o, id, cm, f);
  else if (it == 10)
    return _tga_load_10(name, dx, dy, bpp, o, id, cm, f);

  fclose(f);

  fprintf(stderr, "TGA_LOAD: Only true color images are supported.\n");

  return SUCCEEDED;
}
