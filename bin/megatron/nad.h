/* Forward Declarations */
struct FHeader;

int nad_open(char *path, int openflags, struct FHeader *fh, int options);
int nad_header_read(struct FHeader *fh);
int nad_header_write(struct FHeader *fh);
int nad_read(int fork, char *forkbuf, int bufc);
int nad_write(int fork, char *forkbuf, int bufc);
int nad_close(int status);
