#ifndef SUBTESTS_ENUMERATE_H
#define SUBTESTS_ENUMERATE_H

#include <stdint.h>

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_enumerate_rereads_counted(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_conversion_attempts(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_dehex_keeps_cnid(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_failed_batch_keeps_index(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_failed_read_rereads(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_interleaved_reads_once(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_one_kind_keeps_index(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_kind_change_rewinds(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_deleted_name_keeps_index(AFPObj *obj, uint16_t vid);
extern int utest_movecwd_skips_getcwd(AFPObj *obj, struct vol *vol);
extern int utest_movecwd_at_descriptor_limit(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_long_name_skipped(AFPObj *obj, uint16_t vid);

#endif /* SUBTESTS_ENUMERATE_H */
