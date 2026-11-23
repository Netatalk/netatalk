/*!
 * @file
 * @brief Socket pairs above FD_SETSIZE for the tests of waits that take them
 */

#ifndef TEST_HIGH_FD_H
#define TEST_HIGH_FD_H 1

#include <sys/resource.h>

extern int high_fd_pair(int pair[2], struct rlimit *saved);

#endif /* TEST_HIGH_FD_H */
