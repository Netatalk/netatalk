/*
 * Copyright (c) 1995 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */
#ifndef PAPD_PPD_H
#define PAPD_PPD_H 1

#include <sys/cdefs.h>

struct ppd_font {
    char		*pd_font;
    struct ppd_font	*pd_next;
};

struct ppd_feature {
    char	*pd_name;
    char	*pd_value;
};

struct ppd_feature	*ppd_feature __P((const char *, int));
struct ppd_font		*ppd_font __P((char *));
int read_ppd __P((char *, int));
int ppd_init __P(( void ));

#endif /* PAPD_PPD_H */
