/*
 * $Id: list.h,v 1.2 2001-06-25 20:13:45 rufustfirefly Exp $
 *
 * Copyright (c) 1990,1992 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

struct list {
    void	*l_data;
    struct list	*l_next,
		*l_prev;
};
