#ifndef AFPD_VIRTUAL_ICON_H
#define AFPD_VIRTUAL_ICON_H

#include <atalk/globals.h>

#include "volume.h"

/*! The Mac filename: "Icon" followed by carriage return (0x0D) */
#define VIRTUAL_ICON_NAME "Icon\x0D"

/*! Reserved CNID for the virtual Icon file (below CNID_START=17) */
#define VIRTUAL_ICON_CNID 16

/*! Maximum resource fork size for the virtual Icon file */
#define VIRTUAL_ICON_RFORK_MAXLEN 2304

/*! Size of the classic Mac resource fork header (data offset,
 *  map offset, data length, map length — four big-endian uint32s).
 *  The "data length" field at offset 8 is zero when the fork
 *  contains no resources. */
#define RFORK_HEADER_SIZE 16

/*! Offset within the resource fork header of the data-length field */
#define RFORK_DATA_LEN_OFF 8

int  virtual_icon_enabled(const struct vol *vol);
int  is_virtual_icon_name(const char *name);
int  real_icon_exists(const struct vol *vol);

void virtual_icon_init(struct vol *vol);
const unsigned char *virtual_icon_get_rfork(const struct vol *vol,
        size_t *outlen);

int  virtual_icon_getfilparams(const AFPObj *obj,
                               struct vol *vol,
                               uint16_t bitmap,
                               char *buf,
                               size_t *buflen);

#endif /* AFPD_VIRTUAL_ICON_H */
