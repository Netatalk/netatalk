#ifndef SUBTESTS_PFD_H
#define SUBTESTS_PFD_H

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int test_pfd_ostat_equivalence(struct vol *vol);
extern int test_pfd_rename_repairs_child_path(struct vol *vol);
extern int test_pfd_probe_rename_detection(struct vol *vol);
extern int test_pfd_vol_close_purges_slots(AFPObj *obj, struct vol *vol);

#endif /* SUBTESTS_PFD_H */
