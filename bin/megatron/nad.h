/*
 * $Id: nad.h,v 1.5 2010-01-27 21:27:53 didg Exp $
 */

#ifndef _NAD_H
#define _NAD_H 1

/* Forward Declarations */
struct FHeader;

int nad_open(char *path, int openflags, struct FHeader *fh, int options);
int nad_header_read(struct FHeader *fh);
int nad_header_write(struct FHeader *fh);
ssize_t nad_read(int fork, char *forkbuf, size_t bufc);
ssize_t nad_write(int fork, char *forkbuf, size_t bufc);
int nad_close(int status);

void select_charset(int options);

#endif /* _NAD_H */
