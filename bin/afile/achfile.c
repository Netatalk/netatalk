/*
 * $Id: achfile.c,v 1.4 2001-09-05 18:38:23 srittau Exp $
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
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <atalk/adouble.h>

#include "common.h"

/* Global Variables */
const char *type    = NULL;
const char *creator = NULL;


/* Print usage information. */
void usage(char *prog)
{
  fprintf(stderr, "Usage: %s [-t TYPE] [-c CREATOR] FILE ...\n", prog);
}

/* Print extensive help. */
void help(char *prog)
{
  usage(prog);
  fprintf(stderr,
	  "\n"
	  "Change the MacOS creator/type of FILEs.\n"
	  "\n"
	  "  -t, --type=TYPE        choose type\n"
	  "  -c, --creator=CREATOR  choose creator\n"
	  "  -h, --help             show this help and exit\n"
	  "  -v, --version          show version information and exit\n");
}

/* Print the version. */
void version()
{
  fprintf(stderr, "achfile (netatalk " VERSION ")\n");
}

/* Argument Handling
 * known options: -t, -c, -h, -v
 * known long options: --help, --version
 */
#define OPTSTRING "t:c:hv-:"
const char *get_long_arg(int argc, char *argv[], const char *arg, const char *oa) {
  switch (*oa) {
  case '=':
    return &oa[1];
  case '\0':
    if (optind == argc) {
      fprintf(stderr, "%s: option \'%s\' requires an argument\n", argv[0], arg);
      return NULL;
    }
    return argv[optind++];
  default:
    fprintf(stderr, "%s: unrecognized option \'%s\'\n", argv[0], arg);
    usage(argv[0]);
    return NULL;
  }
}

int parse_args(int argc, char *argv[])
{
  int c;
  const char *longarg;

  /* iterate through the command line */
  while ((c = getopt(argc, argv, OPTSTRING)) != -1) {
    switch (c) {
    case 'h':
      help(argv[0]);
      exit(0);
    case 'v':
      version();
      exit(0);
    case 't':
      type = optarg;
      break;
    case 'c':
      creator = optarg;
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
      if (strncmp(optarg, "type", 4) == 0) {
	longarg = get_long_arg(argc, argv, "type", &optarg[4]);
	if (!longarg)
	  return -1;
	type = longarg;
      } else if (strncmp(optarg, "creator", 7) == 0) {
	longarg = get_long_arg(argc, argv, "creator", &optarg[7]);
	if (!longarg)
	  return -1;
	creator = longarg;
      }
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

  /* Either type or creator is required. */
  if (!type && !creator) {
    fprintf(stderr, "achfile: either type or creator must be specified\n");
    return -1;
  }

  /* Type and creator must be exactly four characters long. */
  if ((type && strlen(type) != 4) || (creator && strlen(creator) != 4)) {
    fprintf(stderr, "achfile: type and creator must be four character IDs\n");
    return -1;
  }

  return 0;
}


/* Change the owner/creator of each file specified on the command line. */
int handle_file(const char *filename)
{
  int fd;
  struct stat statbuf;
  char *adname;
  ssize_t sz;
  char buf[AD_DATASZ];
  struct adouble *ad = (struct adouble *) &buf;

  if (stat(filename, &statbuf) == -1) {
    fprintf(stderr, "achfile:%s: %s\n", filename, strerror(errno));
    return -1;
  }

  adname = dataname_to_adname(filename);
  fd = open(adname, O_RDWR, 0);
  if (fd == -1) {
    if (errno == ENOENT)
      fprintf(stderr, "achfile:%s: no resource fork\n", filename);
    else
      fprintf(stderr, "achfile:%s: %s\n", adname, strerror(errno));
    free(adname);
    return -1;
  }
  sz = read(fd, buf, AD_DATASZ);
  if (sz == -1) {
    fprintf(stderr, "achfile:%s: %s\n", adname, strerror(errno));
    free(adname);
    close(fd);
    return -1;
  } else if (sz < AD_DATASZ) {
    fprintf(stderr, "achfile:%s: corrupt resource fork\n", filename);
    free(adname);
    close(fd);
    return -1;
  }
  if (ad->ad_magic != AD_MAGIC) {
    fprintf(stderr, "achfile:%s: corrupt resource fork\n", filename);
    free(adname);
    close(fd);
    return -1;
  }

  /* Change Type */
  if (type) {
    buf[ADEDOFF_FINDERI + 0] = type[0];
    buf[ADEDOFF_FINDERI + 1] = type[1];
    buf[ADEDOFF_FINDERI + 2] = type[2];
    buf[ADEDOFF_FINDERI + 3] = type[3];
 }

  /* Change Creator */
  if (creator) {
    buf[ADEDOFF_FINDERI + 4] = creator[0];
    buf[ADEDOFF_FINDERI + 5] = creator[1];
    buf[ADEDOFF_FINDERI + 6] = creator[2];
    buf[ADEDOFF_FINDERI + 7] = creator[3];
 }

  /* Write file back to disk. */
  if (lseek(fd, 0, SEEK_SET) == -1) {
    fprintf(stderr, "achfile:%s: %s\n", adname, strerror(errno));
    free(adname);
    close(fd);
    return -1;
  }
  if (write(fd, &buf, AD_DATASZ) < AD_DATASZ) {
    fprintf(stderr, "achfile:%s: %s\n", adname, strerror(errno));
    free(adname);
    close(fd);
    return -1;
  }

  /* Clean Up */
  free(adname);
  close(fd);

  return 0;
}


int main(int argc, char *argv[])
{
  /* argument handling */
  if (parse_args(argc, argv) == -1)
    return 1;

  while (optind < argc)
    handle_file(argv[optind++]);

  return 0;
}
