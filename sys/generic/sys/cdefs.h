/*
 * $Id: cdefs.h,v 1.4 2001-08-15 15:29:14 srittau Exp $
 */

#ifndef _SYS_CDEFS_H
#define _SYS_CDEFS_H 1

#if defined(__STDC__) || defined(__DECC)
/* Note that there must be exactly one space between __P(args) and args,
 * otherwise DEC C chokes.
 */
#define __P(args) args
#else /* __STDC__ */
#define __P(args)    ()
#endif /* __STDC__ */

#endif /* sys/cdefs.h */
