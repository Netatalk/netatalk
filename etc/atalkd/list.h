/*
 * Copyright (c) 1990,1992 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

struct list {
    void	*l_data;
    struct list	*l_next,
		*l_prev;
};
