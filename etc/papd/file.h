/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef PAPD_FILE_H
#define PAPD_FILE_H 1

struct papfile {
    int			pf_state;
    struct state	*pf_xstate;
    int			pf_bufsize;
    int			pf_datalen;
    char		*pf_buf;
    char		*pf_data;
};

#define PF_BOT		(1<<0)
#define PF_EOF		(1<<1)
#define PF_QUERY	(1<<2)

#define CONSUME( pf, len )  {   (pf)->pf_data += (len); \
				(pf)->pf_datalen -= (len); \
				if ((pf)->pf_datalen <= 0) { \
				    (pf)->pf_data = (pf)->pf_buf; \
				    (pf)->pf_datalen = 0; \
				} \
			    }

#define PF_MORESPACE	1024

int markline __P(( struct papfile *, char **, int *, int * ));
void morespace __P(( struct papfile *, char *, int ));
void append __P(( struct papfile *, const char *, int ));
void spoolerror __P(( struct papfile *, char * ));

#endif /* PAPD_FILE_H */
