/*!
 * @file
 * @brief Veto file matching and vetoed file deletion test entry points
 */

#ifndef SUBTESTS_VETO_H
#define SUBTESTS_VETO_H

int utest_delete_veto_symlink_stays_in_volume(void);
int utest_delete_veto_recursive(void);
int utest_delete_veto_dirfd_failure(void);
int utest_delete_veto_any_case(void);
int utest_veto_file_matches_elements(void);

#endif /* SUBTESTS_VETO_H */
