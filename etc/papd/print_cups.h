
#ifndef PAPD_CUPS_H
#define PAPD_CUPS_H 1

#ifdef HAVE_CUPS
#include <sys/cdefs.h>

struct cups_status {
	int	pr_status;
	char *  status_message;
};

int 		cups_printername_ok (char * );
const char * 	cups_get_printer_ppd ( char * );
int 		cups_get_printer_status (struct printer *pr);
int 		cups_print_job ( char *, char *, char *, char *, char *);
struct printer * cups_autoadd_printers ( struct printer *, struct printer *);
int 		cups_check_printer ( struct printer *, struct printer *, int);
const char	*cups_get_language ( void );
#endif /* HAVE_CUPS */
#endif /* PAPD_CUPS_H */
