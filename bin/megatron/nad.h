/*
 * $Id: nad.h,v 1.2 2001-06-29 14:14:46 rufustfirefly Exp $
 */

#ifndef _NAD_H
#define _NAD_H 1

/* Forward Declarations */
struct FHeader;

int nad_open(char *path, int openflags, struct FHeader *fh, int options);
int nad_header_read(struct FHeader *fh);
int nad_header_write(struct FHeader *fh);
int nad_read(int fork, char *forkbuf, int bufc);
int nad_write(int fork, char *forkbuf, int bufc);
int nad_close(int status);

#endif /* _NAD_H */
