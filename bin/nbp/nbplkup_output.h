#ifndef NETATALK_NBPLKUP_OUTPUT_H
#define NETATALK_NBPLKUP_OUTPUT_H

#include <stddef.h>

#include <atalk/unicode.h>

size_t nbplkup_convert_field(charset_t from, const char *src, size_t src_len,
                             char **dest);

#endif /* NETATALK_NBPLKUP_OUTPUT_H */
