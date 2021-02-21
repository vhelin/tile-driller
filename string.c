
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>



int string_compare_nocase(char *c1, char *c2) {

  int a, b;


  if (c1 == NULL || c2 == NULL)
    return 1;

  while (1) {
    a = *(c1++);
    b = *(c2++);
    if (toupper(a) != toupper(b))
      return 1;
    if (a == 0)
      return 0;
  }

  return 1;
}
