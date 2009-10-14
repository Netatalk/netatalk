/*
 * $Id: megatron.h,v 1.5 2009-10-14 01:38:28 didg Exp $
 */

#ifndef _MEGATRON_H
#define _MEGATRON_H 1

#include <atalk/adouble.h>

#ifndef	STDIN
#	define	STDIN	"-"
#endif /* ! STDIN */

/*
    Where it matters, data stored in either of these two structs is in
    network byte order.  Any routines that need to interpret this data
    locally would need to do the conversion.  Mostly this affects the 
    fork length variables.  Time values are affected as well, if any
    routines actually need to look at them.
 */

#define DATA		0
#define RESOURCE	1
#define NUMFORKS	2

#define HEX2NAD		0	/* unhex */
#define BIN2NAD		1	/* unbin */
#define SINGLE2NAD	2	/* unsingle */
#define NAD2BIN		3	/* macbinary */
#define HEX2BIN		4	/* hqx2bin */
#define SINGLE2BIN	5	/* single2bin */
#define NADHEADER       6       /* header */
#define BINHEADER       7       /* mac binary header */
#define MEGATRON	8	/* megatron, default, usually HEX2NAD */
#define CONVERTS	9	/* # conversions defined */

#define OPTION_NONE       (0)
#define OPTION_HEADERONLY (1 << 0)
#define OPTION_STDOUT     (1 << 2)
#define OPTION_EUCJP      (1 << 3)
#define OPTION_SJIS       (1 << 4)

struct FInfo {
    u_int32_t		fdType;
    u_int32_t		fdCreator;
    u_int16_t		fdFlags;
    u_int32_t		fdLocation;
    u_int16_t		fdFldr;
};

struct FXInfo {
    u_int16_t           fdIconID;
    u_int16_t           fdUnused[3];
    u_int8_t            fdScript;
    u_int8_t            fdXFlags;
    u_int16_t           fdComment;
    u_int32_t           fdPutAway;
};

struct FHeader {
    char		name[ ADEDLEN_NAME ];
    char		comment[ ADEDLEN_COMMENT ];
    u_int32_t		forklen[ NUMFORKS ];
    u_int32_t		create_date;
    u_int32_t		mod_date;
    u_int32_t		backup_date;
    struct FInfo	finder_info;
    struct FXInfo       finder_xinfo;
};

#define MAC_DATE_TO_UNIX(a)   (ntohl(a) - 2082844800U)
#define MAC_DATE_FROM_UNIX(a) (htonl((a) + 2082844800U))
#define AD_DATE_FROM_MAC(a)   (htonl(ntohl(a) - 3029529600U)) 
#define AD_DATE_TO_MAC(a)     (htonl(ntohl(a) + 3029529600U))

#define FILEIOFF_CREATE	0
#define FILEIOFF_MODIFY	4
#define FILEIOFF_BACKUP	8
#define FILEIOFF_ATTR	14
#define FINDERIOFF_TYPE		0
#define FINDERIOFF_CREATOR	4
#define FINDERIOFF_FLAGS	8
#define FINDERIOFF_LOC		10
#define FINDERIOFF_FLDR		14
#define FINDERIOFF_SCRIPT       24
#define FINDERIOFF_XFLAGS       25

#define	TRASH		0
#define	KEEP		1

#ifndef S_ISDIR
#	define S_ISDIR(s)	(( s & S_IFMT ) == S_IFDIR )
#endif /* ! S_ISDIR */

extern char	*forkname[];
extern char     *(*_mtoupath) ( char *);
extern char     *(*_utompath) ( char *);
#define mtoupath(s) (*_mtoupath)(s)
#define utompath(s) (*_utompath)(s)

#endif /* _MEGATRON_H */
