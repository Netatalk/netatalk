/*!
 * @file
 * @brief Spotlight's name comparisons: the value syntax, the c, d and w
 *        modifiers, and the && and || of a query
 */

#ifndef ATALK_SL_NAMES_H
#define ATALK_SL_NAMES_H 1

#include <stdbool.h>
#include <stddef.h>

#include <atalk/unicode.h>

#define SL_NAMES_CASE      (1 << 0) /*!< c: letters ignore case */
#define SL_NAMES_DIACRITIC (1 << 1) /*!< d: letters ignore diacritics */
#define SL_NAMES_WORD      (1 << 2) /*!< w: the value anchors at words */
#define SL_NAMES_NEGATE    (1 << 3) /*!< !=: the names that do not match */

struct sl_name_pattern;
struct sl_name_query;

/*! Tells whether a backend narrows a search by a seed */
typedef bool sl_name_narrows_fn(const struct sl_name_pattern *seed,
                                void *ctx);

extern struct sl_name_pattern *sl_name_pattern_new(const char *value,
                                                   size_t len,
                                                   unsigned int flags);
extern void sl_name_pattern_free(struct sl_name_pattern *pattern);
extern bool sl_name_pattern_match(const struct sl_name_pattern *pattern,
                                  const char *name, size_t len);
extern size_t sl_name_pattern_literals(const struct sl_name_pattern *pattern);
extern const char *sl_name_pattern_like(const struct sl_name_pattern *pattern);
extern const char *sl_name_pattern_index_glob(
    const struct sl_name_pattern *pattern);
extern const char *sl_name_pattern_index_like(
    const struct sl_name_pattern *pattern);
extern char *sl_name_fold(const char *name, size_t len);
extern char *sl_name_fold_in(const char *name, size_t len, charset_t charset);
extern struct sl_name_query *sl_name_query_parse(const char *query,
                                                 size_t min_literals,
                                                 int max_comparisons);
extern struct sl_name_query *sl_name_query_parse_in(const char *query,
                                                    size_t min_literals,
                                                    int max_comparisons,
                                                    charset_t charset);
extern struct sl_name_query *sl_name_query_value(const char *value,
                                                 size_t len,
                                                 unsigned int flags);
extern struct sl_name_query *sl_name_query_term(const char *term, size_t len,
                                                bool exact,
                                                unsigned int flags);
extern struct sl_name_query *sl_name_query_term_in(const char *term,
                                                   size_t len, bool exact,
                                                   unsigned int flags,
                                                   charset_t charset);
extern void sl_name_query_free(struct sl_name_query *query);
extern bool sl_name_query_match(const struct sl_name_query *query,
                                const char *name, size_t len);
extern int sl_name_query_seeds(const struct sl_name_query *query,
                               const struct sl_name_pattern **seeds, int max,
                               sl_name_narrows_fn *narrows, void *ctx);

#endif /* ATALK_SL_NAMES_H */
