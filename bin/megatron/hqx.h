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
