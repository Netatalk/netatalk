#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int _strdup_dummy;

#ifdef ultrix
char *strdup(const char *string)
{
  char *new;
  
  if (new = (char *) malloc(strlen(string) + 1))
    strcpy(new, string);

  return new;
}
#endif
