/*
 * $Id: adv1tov2.c,v 1.5 2009-10-14 01:38:28 didg Exp $
 * v1tov2: given a root directory, run down and convert all the
 * files/directories into appledouble v2.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <atalk/adouble.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>
#include <string.h>

#include <atalk/util.h>

#if AD_VERSION == AD_VERSION2
#if 0
/* translate characters */
static void xlate(char *name, int flags) {
  static const char hexdig[] = "0123456789abcdef";
  char upath[MAXPATHLEN + 1];
  char *m, *u;
  int doit = 0;

  m = name;
  u = upath;
  while (*m != '\0') {
    if (isascii(*m) && !isprint(*m)) {
      doit = 1;
      *u++ = ':';
      *u++ = hexdig[(*m & 0xf0) >> 4];
      *u++ = hexdig[*m & 0x0f];
    } else
      *u++ = *m;
    m++;
  }
  
  if (doit) {
    *u = '\0';
    rename(name, upath);
    if ((flags & ADFLAGS_DIR) == 0)
      rename(ad_path(name, flags), ad_path(upath, flags)); /* rename rfork */
  }
}
#endif

/* ----------------------------- */
char *fullpathname(const char *name)
{
    static char wd[ MAXPATHLEN + 1];

    if ( getcwd( wd , MAXPATHLEN) ) {
        strlcat(wd, "/", MAXPATHLEN);
        strlcat(wd, name, MAXPATHLEN);
    }
    else {
        strlcpy(wd, name, MAXPATHLEN);
    }
    return wd;
}

#define MAXDESCEND 0xFFFF
/* recursively descend subdirectories. 
 * oh the stack space we use up! */
static void descend(DIR *dp)
{
  DIR *dpnew;
  struct dirent *de;
  struct stat st;
  struct adouble ad;
  int flags;
  static int count = 0;

  ad_init(&ad, AD_VERSION2, 0);
  if (count++ > MAXDESCEND) {
    fprintf(stderr, "FAILURE: too many subdirectories! possible infinite recursion.");
    return;
  }
    
  putc('(', stderr);
  for (de = readdir(dp); de; de = readdir(dp)) {
    if (de->d_name[0] == '.') 
      continue;

    if (stat(de->d_name, &st) < 0) {
      fprintf(stderr, "FAILURE: can't stat %s\n", de->d_name);
      continue;
    }

    /* go down subdirectory */
    flags = ADFLAGS_HF;
    if (S_ISDIR(st.st_mode) && (dpnew = opendir(de->d_name))) {
      chdir(de->d_name);
      descend(dpnew);
      closedir(dpnew);
      chdir("..");
      flags |= ADFLAGS_DIR;
    }

    if (ad_open(de->d_name, flags, O_RDWR, 0, &ad) < 0) {
      if (errno != ENOENT)
          fprintf(stderr, "\nFAILURE: can't convert %s, %s\n", fullpathname(de->d_name), strerror(errno));
      continue;
    }
    ad_close(&ad, ADFLAGS_HF);
    fputc('.', stderr);
  }
  putc(')', stderr);
}


int main(int argc, char **argv)
{
  DIR           *dp;
  struct adouble ad;
 
  ad_init(&ad, AD_VERSION2, 0);
  if (argc != 2) {
    fprintf(stderr, "%s <directory>\n", *argv);
    return -1;
  }
    
  /* convert main directory */
  if (ad_open(argv[1], ADFLAGS_HF | ADFLAGS_DIR, O_RDWR, 0, 
	      &ad) < 0) {
    fprintf(stderr, "FAILURE: can't convert %s\n", argv[1]);
    return -1;
  }

  ad_close(&ad, ADFLAGS_HF);
  if ((dp = opendir(argv[1])) == NULL) {
    fprintf(stderr, "%s: unable to open %s\n", *argv, argv[1]);
    return -1;
  }

  chdir(argv[1]);
  descend(dp);
  closedir(dp);
  chdir("..");

  putc('\n', stderr);
  return 0;
}

#else /* AD_VERSION == AD_VERSION2 */
int main(int argc, char **argv)
{
  fprintf(stderr, "%s not built for v2 AppleDouble files.\n", *argv);
  return -1;
}
#endif /* AD_VERSION == AD_VERSION2 */
