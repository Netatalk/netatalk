/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

struct papfile {
    int			pf_state;
    struct state	*pf_xstate;
    int			pf_len;
    char		*pf_buf;
    char		*pf_cur;
    char		*pf_end;
};

#define PF_BOT		(1<<0)
#define PF_EOF		(1<<1)
#define PF_QUERY	(1<<2)

#define APPEND( pf, data, len )	\
	if ( (pf)->pf_end + (len) > (pf)->pf_buf + (pf)->pf_len ) { \
		morespace( (pf), (data), (len)); \
	} else { \
		bcopy( (data), (pf)->pf_end, (len)); \
		(pf)->pf_end += (len); \
	}
#define PF_BUFSIZ( pf )		((pf)->pf_end - (pf)->pf_cur)
#define CONSUME( pf, len )	(((pf)->pf_cur += (len)), \
	(((pf)->pf_cur >= (pf)->pf_end) && \
	((pf)->pf_cur = (pf)->pf_end = (pf)->pf_buf)))

#define PF_MORESPACE	1024
