
#ifndef PAPD_CUPS_H
#define PAPD_CUPS_H 1

#ifdef HAVE_CUPS
#include <sys/cdefs.h>

struct cups_status {
	int	pr_status;
	char *  status_message;
};

int 		cups_printername_ok __P((char * ));
const char * 	cups_get_printer_ppd __P(( char * ));
int 		cups_get_printer_status __P((struct printer *pr));
int 		cups_print_job __P(( char *, char *, char *, char *, char *));
struct printer * cups_autoadd_printers __P(( struct printer *, struct printer *));
int 		cups_check_printer __P(( struct printer *, struct printer *, int));
char		*cups_get_language __P(());
#endif /* HAVE_CUPS */
#endif /* PAPD_CUPS_H */
