/*
 * Copyright (c) 1990,1992 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

/*!
 * @file
 * @brief Doubly linked list node type for atalkd routing tables
 */

struct list {
    void *l_data;
    struct list *l_next, *l_prev;
};
