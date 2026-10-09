#ifndef SUBTESTS_CONF_H
#define SUBTESTS_CONF_H

#include <stddef.h>
#include <stdint.h>

/* Test seam in libatalk/util/netatalk_conf.c; declared here (not in the
 * installed netatalk_conf.h) so the hook stays out of the public API. */
extern void conf_testutil_set_lastvid(uint16_t vid);

/* Log-file helpers, shared with the CNID tests */
extern int conf_mklog(char *buf, size_t buflen);
extern void conf_log_truncate(const char *logpath);
extern int conf_log_contains(const char *logpath, const char *needle);

extern int utest_conf_parse_bool(void);
extern int utest_conf_permission_options_require_unix_priv(void);
extern int utest_conf_ea_fallback(void);
extern int utest_conf_cnid_server_listen_ignored(void);
extern int utest_conf_vol_dbnest_ignored(void);
extern int utest_conf_removed_options_appletalk_parse(void);
extern int utest_conf_singleuser_dbpath_volume_root(void);
extern int utest_conf_strict_locking_keys(void);
extern int utest_conf_srp_verifier_path_keys(void);
extern int utest_conf_spotlight_results_limit_keys(void);
extern int utest_conf_multiproto_defaults(void);
extern int utest_conf_multiproto_explicit_wins(void);
extern int utest_conf_no_multiproto_regression(void);
extern int utest_conf_stock_defaults(void);
extern int utest_conf_dircache_validation_freq_range(void);
extern int utest_conf_splice_size_bounds(void);
extern int utest_conf_multiproto_reverts_unusable_freq(void);
extern int utest_conf_ea_samba_no_defaults(void);
extern int utest_conf_multiproto_ea_recommendation(void);
extern int utest_conf_samba_requires_ea(void);
extern int utest_conf_samba_ea_failure_keeps_vid(void);
extern int utest_conf_multiproto_defaults_not_leaked_on_failed_volume(void);
extern int utest_conf_multiproto_reverts_compiled_defaults(void);
extern int utest_conf_load_afp_conf_vols_locked(void);
extern int utest_conf_dircache_resolve_size(void);
extern int utest_conf_singleuser_state_paths(void);

#endif /* SUBTESTS_CONF_H */
