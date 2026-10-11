#ifndef SUBTESTS_OFFCNT_H
#define SUBTESTS_OFFCNT_H

#include <stdint.h>

#include <atalk/globals.h>

extern int utest_offcnt_stat_causes(AFPObj *obj, uint16_t vid);
extern int utest_offcnt_hard_create_existing(AFPObj *obj, uint16_t vid);
extern int utest_offcnt_fresh_stat_refreshes_cache(AFPObj *obj,
                                                   uint16_t vid);
extern int utest_offcnt_access_refreshes_cache(AFPObj *obj, uint16_t vid);
extern int utest_access_delete_right_follows_parent(AFPObj *obj,
                                                    uint16_t vid);

#endif /* SUBTESTS_OFFCNT_H */
