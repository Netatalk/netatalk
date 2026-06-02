#ifndef MEGATRON_H
#define MEGATRON_H 1

#include <atalk/adouble.h>
#include <atalk/compat.h>
#include <stdbool.h>

struct vol;

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
#define NAD2BIN		2	/* bin */
#define NAD2HEX		3	/* hex */

#define OPTION_NONE       (0)
#define OPTION_HEADERONLY (1 << 0)
#define OPTION_STDOUT     (1 << 1)
#define OPTION_VERBOSE    (1 << 2)
#define OPTION_ADOUBLE    (1 << 3)

#define FH_DATE_CREATE    (1 << 0)
#define FH_DATE_MODIFY    (1 << 1)
#define FH_DATE_BACKUP    (1 << 2)

struct FInfo {
    uint32_t		fdType;
    uint32_t		fdCreator;
    uint16_t		fdFlags;
    uint32_t		fdLocation;
    uint16_t		fdFldr;
};

struct FXInfo {
    uint16_t           fdIconID;
    uint16_t           fdUnused[3];
    uint8_t            fdScript;
    uint8_t            fdXFlags;
    uint16_t           fdComment;
    uint32_t           fdPutAway;
};

struct FHeader {
    char		name[ ADEDLEN_NAME + 1 ];
    char		comment[ ADEDLEN_COMMENT + 1 ];
    uint32_t		forklen[ NUMFORKS ];
    uint32_t		create_date;
    uint32_t		mod_date;
    uint32_t		backup_date;
    unsigned int        date_flags;
    struct FInfo	finder_info;
    struct FXInfo       finder_xinfo;
};

struct nad_volume {
    struct vol     *vol;
    struct vol     *cnid_vol;
    char           db_stamp[ADEDLEN_PRIVSYN];
    bool           owns_cdb;
    bool           fallback;
    bool           skip_cnid;
};

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
extern char     *(*_mtoupath)(char *);
extern char     *(*_utompath)(char *);
#define mtoupath(s) (*_mtoupath)(s)
#define utompath(s) (*_utompath)(s)

#endif /* MEGATRON_H */
