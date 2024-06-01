#ifndef _HQX_H
#define _HQX_H 1

/* Forward Declarations */
struct FHeader;

int skip_junk(int line);
int hqx_open(char *hqxfile, int flags, struct FHeader *fh, int options);
int hqx_close(int keepflag);
ssize_t hqx_read(int fork, char *buffer, size_t length);
int hqx_header_read(struct FHeader *fh);
int hqx_header_write(struct FHeader *fh);
size_t hqx_7tobin(char *outbuf, size_t datalen);
ssize_t hqx7_fill(unsigned char *hqx7_ptr);

#endif /* _HQX_H */
