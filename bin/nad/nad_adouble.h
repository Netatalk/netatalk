#ifndef NAD_ADOUBLE_H
#define NAD_ADOUBLE_H 1

/* Forward Declarations */
struct nad_volume;
struct FHeader;

void nad_set_volume(const struct nad_volume *volume);
int nad_open(char *path, int openflags, struct FHeader *fh, int options);
int nad_header_read(struct FHeader *fh);
int nad_header_write(struct FHeader *fh);
ssize_t nad_read(int fork, char *forkbuf, size_t bufc);
ssize_t nad_write(int fork, char *forkbuf, size_t bufc);
int nad_close(int status);

#endif /* NAD_ADOUBLE_H */
