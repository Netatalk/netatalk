/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

struct comment {
    char		*c_begin;
    char		*c_end;
    int			(*c_handler)();
    int			c_flags;
};

#define CH_DONE		0
#define CH_MORE		1
#define CH_ERROR	-1

struct comstate {
    struct comment	*cs_comment;
    struct comstate	*cs_prev;
    int			cs_flags;
};

extern struct comment	*commatch();
extern struct comstate	*comstate;
extern struct comment	magics[];
extern struct comment	queries[];
extern struct comment	headers[];
extern char		*comcont;

#define compeek()	(comstate==NULL?NULL:(comstate->cs_comment))
#define comgetflags()	(comstate->cs_flags)
#define comsetflags(f)	(comstate->cs_flags=(f))

/*
 * Comment flags.  0-15 reserved for "global" flags, 16-31 for specific
 * subtypes.
 */
#define C_FULL		(1<<0)				/* or prefix */
#define C_CONTINUE	(1<<1)

/*
 * Query subtypes.
 */

/*
 * Magic "number" subtypes.
 */
#define CM_NOPRINT	(1<<16)				/* or print */
