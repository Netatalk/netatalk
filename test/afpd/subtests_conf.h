#ifndef SUBTESTS_CONF_H
#define SUBTESTS_CONF_H

#include <stdint.h>

/* Test seam in libatalk/util/netatalk_conf.c; declared here (not in the
 * installed netatalk_conf.h) so the hook stays out of the public API. */
extern void conf_testutil_set_lastvid(uint16_t vid);

extern int utest_conf_parse_bool(void);
extern int utest_conf_permission_options_require_unix_priv(void);
extern int utest_conf_ea_fallback(void);
extern int utest_conf_strict_locking_keys(void);
extern int utest_conf_spotlight_results_limit_keys(void);
extern int utest_conf_multiproto_defaults(void);
extern int utest_conf_multiproto_explicit_wins(void);
extern int utest_conf_no_multiproto_regression(void);
extern int utest_conf_stock_defaults(void);
extern int utest_conf_dircache_validation_freq_range(void);
extern int utest_conf_multiproto_reverts_unusable_freq(void);
extern int utest_conf_ea_samba_no_defaults(void);
extern int utest_conf_multiproto_ea_recommendation(void);
extern int utest_conf_samba_requires_ea(void);
extern int utest_conf_samba_ea_failure_keeps_vid(void);
extern int utest_conf_multiproto_defaults_not_leaked_on_failed_volume(void);
extern int utest_conf_multiproto_reverts_compiled_defaults(void);
extern int utest_conf_load_afp_conf_vols_locked(void);
extern int utest_conf_dircache_resolve_size(void);

#endif /* SUBTESTS_CONF_H */
