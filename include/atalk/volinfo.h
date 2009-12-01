/*
 * $Id: volinfo.h,v 1.9 2009-12-01 16:36:14 franklahm Exp $
 */

#ifndef _ATALK_VOLINFO_H
#define _ATALK_VOLINFO_H 1

#include <atalk/unicode.h>
#include <atalk/volume.h>

/* volinfo for shell utilities */
#define VOLINFODIR  ".AppleDesktop"
#define VOLINFOFILE ".volinfo"

typedef struct {
    const u_int32_t option;
    const char      *name;
} vol_opt_name_t;

struct volinfo {
    char                *v_name;
    char                *v_path;
    int                 v_flags;
    int                 v_casefold;
    char                *v_cnidscheme;
    char                *v_dbpath;
    char                *v_volcodepage;
    charset_t           v_volcharset;
    char                *v_maccodepage;
    charset_t           v_maccharset;
    int                 v_adouble;  /* default adouble format */
    int                 v_ad_options;
    char                *(*ad_path)(const char *, int);
    char                *v_dbd_host;
    int                 v_dbd_port;
};

extern const vol_opt_name_t vol_opt_names[];
extern const vol_opt_name_t vol_opt_casefold[];

extern int loadvolinfo(char *path, struct volinfo *vol);
extern int savevolinfo(const struct vol *vol, const char *Cnid_srv, const char *Cnid_port);
extern int vol_load_charsets(struct volinfo *vol);

#endif
