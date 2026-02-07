#ifndef PAPD_PRINTCAP_H
#define PAPD_PRINTCAP_H 1

#include <sys/types.h>

int getprent(char *, char *, int);
int pnchktc(char *);
int pgetflag(char *);
void endprent(void);
int pgetent(char *, char *, const char *);
int pgetnum(char *);
int pnamatch(const char *);
char *pgetstr(char *id, char **area);
char *getpname(char **area, int bufsize);

#endif /* PAPD_PRINTCAP_H */
