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


char *dataname_to_adname(const char *dataname);
char *adname_to_dataname(const char *adname);

/* Class for handling data and AppleDouble files. Don't access directly.
 * Instead, use the methods provided below.
 */
struct AFile {
  const char *filename;
  int fd;
  mode_t mode;
  unsigned short flags;
  char creator[4], type[4];
};


/* Constructor */
struct AFile *afile_new(const char *filename);
/* Destructor */
void afile_delete(struct AFile *afile);
/* Accessor Methods */
const char *afile_filename(struct AFile *afile);
int afile_is_ad(struct AFile *afile);
int afile_is_dir(struct AFile *afile);
mode_t afile_mode(struct AFile *afile);
/* The following two methods are only valid if afile_is_ad yields true. */
const char *afile_creator(const struct AFile *afile);
const char *afile_type(const struct AFile *afile);
