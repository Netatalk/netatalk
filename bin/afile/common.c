/*  common functions, defines, and structures for afile, achfile, and acleandir

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

#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <netatalk/endian.h>

#include "common.h"


#define AD_PREFIX ".AppleDouble/"
#define PARENT_PREFIX "../"


char *dataname_to_adname(const char *dataname)
{
  const char *filepart;
  char *adname;
  size_t adlen = strlen(AD_PREFIX);
  size_t dirlen;

  /* Construct the AppleDouble file name from data fork file name. */
  adname = calloc(adlen + strlen(dataname) + 1, sizeof(char));
  filepart = rindex(dataname, '/');
  if (filepart == NULL) {
    /* Filename doesn't contain a path. */
    strcpy(adname, AD_PREFIX);
    strcpy(adname + adlen, dataname);
  } else {
    /* Filename does contain a path. */
    dirlen = (size_t) (filepart - dataname);
    strncpy(adname, dataname, dirlen + 1);
    strcpy(adname + dirlen + 1, AD_PREFIX);
    strcpy(adname + dirlen + adlen + 1, filepart + 1);
  }

  return adname;
}

char *adname_to_dataname(const char *adname)
{
  const char *filepart;
  char *dataname;
  size_t plen = strlen(PARENT_PREFIX);
  size_t dirlen;

  /* Construct the data file name from the AppleDouble file name. */
  dataname = calloc(strlen(adname) + plen + 1, sizeof(char));
  filepart = rindex(adname, '/');
  if (filepart == NULL) {
    strcpy(dataname, PARENT_PREFIX);
    strcpy(dataname + plen, adname);
  } else {
    dirlen = (size_t) (filepart - adname);
    strncpy(dataname, adname, dirlen + 1);
    strcpy(dataname + dirlen + 1, PARENT_PREFIX);
    strcpy(dataname + dirlen + plen + 1, filepart + 1);
  }

  return dataname;
}


#define FLAG_NONE 0x0000
#define FLAG_AD   0x0001
#define FLAG_DIR  0x0002


struct AFile *afile_new(const char *filename)
{
  struct stat fdstat;
  char adbuf[AD_DATASZ];
  ssize_t sz;
  struct stat statbuf;

  struct AFile *afile = (struct AFile *) calloc(sizeof(struct AFile), 1);
  afile->filename     = filename;
  afile->fd           = open(filename, O_RDONLY);
  afile->flags        = FLAG_NONE;

  /* Try to open file. */
  if (afile->fd == -1) {
    free(afile);
    return NULL;
  }

  fstat(afile->fd, &statbuf);
  afile->mode = statbuf.st_mode;

  /* Try to determine if this file is a regular file, a directory, or
   * something else.
   */
  fstat(afile->fd, &fdstat);
  if (S_ISREG(fdstat.st_mode)) {                     /* it is a regular file */
    /* Check if the opened file is the resource fork. */
    sz = read(afile->fd, adbuf, AD_DATASZ);
    if (sz >= 0) {
      /* If we can't read a whole AppleDouble header, this can't be a valid
       * AppleDouble file.
       */
      if (sz == AD_DATASZ) {
	/* ... but as we could, maybe this is. Now if only the magic number
	 * would be correct ...
	 */
	if (ntohl(((unsigned int *) adbuf)[0]) == AD_MAGIC)
	  /* Great! It obviously is a AppleDouble file. */
	  afile->flags |= FLAG_AD;
	  afile->creator[0] = adbuf[ADEDOFF_FINDERI + 0];
	  afile->creator[1] = adbuf[ADEDOFF_FINDERI + 1];
	  afile->creator[2] = adbuf[ADEDOFF_FINDERI + 2];
	  afile->creator[3] = adbuf[ADEDOFF_FINDERI + 3];
	  afile->type   [0] = adbuf[ADEDOFF_FINDERI + 4];
	  afile->type   [1] = adbuf[ADEDOFF_FINDERI + 5];
	  afile->type   [2] = adbuf[ADEDOFF_FINDERI + 6];
	  afile->type   [3] = adbuf[ADEDOFF_FINDERI + 7];
      }
 
    } else {
      afile_delete(afile);
      return NULL;
    }
    
  } else if (S_ISDIR(fdstat.st_mode))                /* it is a directory */
    afile->flags |= FLAG_DIR;
  else {                                             /* it is neither */
    afile_delete(afile);
    return NULL;
  }

  return afile;
}


void afile_delete(struct AFile *afile)
{
  close(afile->fd);
  free(afile);
}


const char *afile_filename(struct AFile *afile)
{
  return afile->filename;
}

int afile_is_ad(struct AFile *afile)
{
  return afile->flags & FLAG_AD;
}

int afile_is_dir(struct AFile *afile)
{
  return afile->flags & FLAG_DIR;
}

mode_t afile_mode(struct AFile *afile)
{
  return afile->mode;
}

const char *afile_creator(const struct AFile *afile)
{
  return afile->creator;
}

const char *afile_type(const struct AFile *afile)
{
  return afile->creator;
}
