/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 *
 * if we could depend upon inline functions, this would be one.
 */

/*!
 * @file
 * @brief Fork lengths of an AppleDouble
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>

#include <atalk/adouble.h>
#include <atalk/logger.h>

/*!
 * @brief Length of a fork of an AppleDouble
 *
 * The data fork length comes from fstat, or from the link target of a symlink.
 * Any other eid yields ad_rlen, the resource fork length that the header read
 * and ad_write maintain.
 *
 * @param[in] ad   AppleDouble with the fork open
 * @param[in] eid  ADEID_DFORK, or any other entry for the resource fork
 * @returns the length in bytes, 0 when the data fork stat failed
 */
off_t ad_size(const struct adouble *ad, const uint32_t eid)
{
    if (eid == ADEID_DFORK) {
        struct stat st;

        if (ad->ad_data_fork.adf_syml) {
            return strlen(ad->ad_data_fork.adf_syml);
        }

        if (fstat(ad_data_fileno(ad), &st) < 0) {
            return 0;
        }

        return st.st_size;
    }

#if 0
    return ad_getentrylen(ad, eid);
#endif
    return ad->ad_rlen;
}
