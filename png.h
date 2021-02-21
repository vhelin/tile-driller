
#ifndef PNG_H
#define PNG_H

int png_load(char *name, int *dx, int *dy, int *bpp, unsigned char **o);
int png_save(char *name, int dx, int dy, unsigned char *o);

#endif
