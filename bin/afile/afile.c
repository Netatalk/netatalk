/*
 * $Id: afile.c,v 1.7 2009-10-14 01:38:28 didg Exp $
 *
    afile - determine the MacOS creator/type of files

    Copyright (C) 2001 Sebastian Rittau.
    All rights reserved.

    This file may be distributed and/or modfied under the terms of the
    following license:

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the author nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <atalk/adouble.h>

#include "common.h"

/* Possible return types. These are taken from the original afile for
 * compatibility.
 */
#define RET_OK                0 /* everything went fine */
#define RET_NO_SUCH_FILE      1 /* file doesn't exist */
#define RET_UNREADABLE        2 /* file is unreadable */
#define RET_IS_DIR            3 /* file is a directory */
#define RET_NO_DF             4 /* there is no corresponding data fork */
#define RET_INVALID_DF        5 /* data fork exists but can't be read */
#define RET_NO_AD             6 /* AppleDouble file doesn't exist */
#define RET_INVALID_AD        7 /* AppleDouble file exists but can't be read */
#define RET_SHORT_AD          8 /* AppleDouble file is too short */
#define RET_BAD_MAGIC         9 /* AppleDouble file has a bad magic value */
#define RET_BAD_ARGS         99 /* bad command line options */


/* Global Variables */
static int showall = 0;


/* Print usage information. */
static void usage(char *prog)
{
  fprintf(stderr, "Usage: %s [-a] FILE ...\n", prog);
}

/* Print extensive help. */
static void help(char *prog)
{
  usage(prog);
  fprintf(stderr,
	  "\n"
	  "Determine the MacOS creator/type of FILEs.\n"
	  "\n"
	  "  -a, --show-all    also show unknown files and directories\n"
	  "  -h, --help        show this help and exit\n"
	  "  -v, --version     show version information and exit\n");
}

/* Print the version. */
static void version(void)
{
  fprintf(stderr, "afile (netatalk " VERSION ")\n");
}

/* Argument Handling
 * known options: -a, -h, -v
 * known long options: --show-all, --help, --version
 */
#define OPTSTRING "ahv-:"
static int parse_args(int argc, char *argv[])
{
  int c;

  /* iterate through the command line */
  while ((c = getopt(argc, argv, OPTSTRING)) != -1) {
    switch (c) {
    case 'h':
      help(argv[0]);
      exit(0);
    case 'v':
      version();
      exit(0);
    case 'a':
      showall = 1;
      break;
    case '-':
      if (strcmp(optarg, "help") == 0) {
	help(argv[0]);
	exit(0);
      }
      if (strcmp(optarg, "version") == 0) {
	version();
	exit(0);
      }
      if (strcmp(optarg, "show-all") == 0)
	showall = 1;
      break;
    default:
      usage(argv[0]);
      return -1;
    }
  }

  /* At least one file argument is required. */
  if (argc == optind) {
    usage(argv[0]);
    return -1;
  }

  return 0;
}


/* Print the creator/type as taken from the supplied character stream, which
 * must be a AppleDouble file header.
 */
static void print_type(const char *filename, const char *creator, const char *type)
{
  printf("%4.4s %4.4s %s\n", creator, type, filename);
}


static int handle_ad(struct AFile *rfile)
{
  int fd;
  char *dataname;

  dataname = adname_to_dataname(afile_filename(rfile));

  /* Try to open data file. */
  fd = open(dataname, O_RDONLY);
  free(dataname);
  if (fd == -1) {
    if (errno == ENOENT)
      return RET_NO_DF;      /* There is no data fork. */
    else
      return RET_INVALID_DF; /* The data fork can't be read. */
  }
  /* FIXME: stat */

  close(fd);

  return RET_OK;
}


static int handle_datafile(struct AFile *datafile)
{
  int ret;
  char *adname;
  struct AFile *rfile;

  /* Try to load AppleDouble file. */
  adname = dataname_to_adname(afile_filename(datafile));
  rfile = afile_new(adname);
  if (!rfile) {
    if (errno == ENOENT) {
      free(adname);
      if (showall)
	printf("unknown %s\n", afile_filename(datafile));
      return RET_NO_AD;
    }
    fprintf(stderr, "afile:%s: %s\n", adname, strerror(errno));
    free(adname);
    return RET_INVALID_AD;
  }
  free(adname);

  /* Check if this is really an AppleDouble file. */
  if (afile_is_ad(rfile)) {
    print_type(afile_filename(datafile),
	       afile_creator(rfile), afile_type(rfile));
    if (afile_mode(datafile) != afile_mode(rfile))
      fprintf(stderr, "afile:%s: mode does not match", afile_filename(datafile));
    ret = RET_OK;
  } else
    ret = RET_INVALID_AD;

  afile_delete(rfile);

  return ret;
}


/* Parse a file and its resource fork. Output the file's creator/type. */
static int parse_file(char *filename)
{
  int ret;
  struct AFile *afile;

  afile = afile_new(filename);
  if (!afile) {
    fprintf(stderr, "afile:%s: %s\n", filename, strerror(errno));
    return errno == ENOENT ? RET_NO_SUCH_FILE : RET_UNREADABLE;
  }

  if (afile_is_dir(afile)) {
    if (showall)
      printf("directory %s\n", filename);
    ret = RET_IS_DIR;
  } else if (afile_is_ad(afile))
    ret = handle_ad(afile);
  else
    ret = handle_datafile(afile);

  afile_delete(afile);

  return ret;
}


int main(int argc, char *argv[])
{
  int ret = RET_OK;

  /* argument handling */
  if (parse_args(argc, argv) == -1)
    return RET_BAD_ARGS;

  /* iterate through all files specified as arguments */
  while (optind < argc)
    ret = parse_file(argv[optind++]);

  return ret;
}
