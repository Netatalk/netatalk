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
 * The resource fork length comes from fstat of its open descriptor less the
 * fork's place in the file, as ad_write's end-relative path counts it, so
 * another process's appends count; without the descriptor it is ad_rlen, the
 * length the header read and ad_write maintain.
 *
 * @param[in] ad   AppleDouble with the fork open
 * @param[in] eid  ADEID_DFORK, or any other entry for the resource fork
 * @returns the length in bytes, 0 when the data fork stat failed
 */
off_t ad_size(const struct adouble *ad, const uint32_t eid)
{
    struct stat st;

    if (eid == ADEID_DFORK) {
        if (ad->ad_data_fork.adf_syml) {
            return strlen(ad->ad_data_fork.adf_syml);
        }

        if (fstat(ad_data_fileno(ad), &st) < 0) {
            return 0;
        }

        return st.st_size;
    }

    if (ad_reso_fileno(ad) != -1 && fstat(ad_reso_fileno(ad), &st) == 0) {
        off_t len = st.st_size - ad_getentryoff(ad, ADEID_RFORK);
        return len > 0 ? len : 0;
    }

#if 0
    return ad_getentrylen(ad, eid);
#endif
    return ad->ad_rlen;
}
