/*
 * $Id: hqx.h,v 1.2 2001-06-29 14:14:46 rufustfirefly Exp $
 */

#ifndef _HQX_H
#define _HQX_H 1

/* Forward Declarations */
struct FHeader;

int skip_junk(int line);
int hqx_open(char *hqxfile, int flags, struct FHeader *fh, int options);
int hqx_close(int keepflag);
int hqx_read(int fork, char *buffer, int length);
int hqx_header_read(struct FHeader *fh);
int hqx_header_write(struct FHeader *fh);
int hqx_7tobin(char *outbuf, int datalen);
int hqx7_fill(u_char *hqx7_ptr);

#endif /* _HQX_H */
