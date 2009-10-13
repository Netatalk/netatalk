/*
 * $Id: macbin.h,v 1.3 2009-10-13 22:55:36 didg Exp $
 */

#ifndef _MACBIN_H
#define _MACBIN_H 1

/* Forward Declarations */
struct FHeader;

int bin_open(char *binfile, int flags, struct FHeader *fh, int options);
int bin_close(int keepflag);
int bin_read(int fork, char *buffer, int length);
int bin_write(int fork, char *buffer, int length);
int bin_header_read(struct FHeader *fh, int revision);
int bin_header_write(struct FHeader *fh);
int test_header(void);

#endif /* _MACBIN_H */
