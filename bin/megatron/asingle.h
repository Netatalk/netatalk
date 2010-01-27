/*
 * $Id: asingle.h,v 1.4 2010-01-27 21:27:53 didg Exp $
 */

#ifndef _ASINGLE_H
#define _ASINGLE_H 1

/* Forward Declarations */
struct FHeader;

int single_open(char *singlefile, int flags, struct FHeader *fh, int options);
int single_close(int readflag);
int single_header_read(struct FHeader *fh, int version);
int single_header_test(void);
ssize_t single_read(int fork, char *buffer, size_t length);

#endif /* _ASINGLE_H */
