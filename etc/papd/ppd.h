/*
 * Copyright (c) 1995 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

struct ppd_font {
    char		*pd_font;
    struct ppd_font	*pd_next;
};

struct ppd_feature {
    char	*pd_name;
    char	*pd_value;
};

struct ppd_feature	*ppd_feature();
struct ppd_font		*ppd_font();
