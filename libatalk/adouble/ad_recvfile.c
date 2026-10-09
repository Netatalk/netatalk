/*
 * Copyright (c) 2013 Ralph Boehme <sloowfranklin@gmail.com>
 * Copyright (c) 2026 Andy Lemin (andylemin)
 * All rights reserved. See COPYRIGHT.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef WITH_RECVFILE

#include <atalk/adouble.h>
#include <atalk/dsi.h>

/*!
 * @brief Receive the rest of a DSIWrite payload into a fork
 *
 * @param[in] ad       adouble holding the fork
 * @param[in] eid      ADEID_DFORK or ADEID_RFORK
 * @param[in,out] dsi  session; its datasize is the payload left to read
 * @param[in,out] off  fork offset of the first byte, advanced past every
 *                     byte written, also when the call fails
 *
 * @returns the bytes written, 0 when the write is left to dsi_write() and
 *          ad_write(), or -1 with errno set
 */
ssize_t ad_recvfile(struct adouble *ad, int eid, DSI *dsi, off_t *off)
{
    ssize_t cc;
    int fd;
    off_t pos = *off;
    off_t start;

    /* ad_write() refuses the symlink */
    if (ad_data_fileno(ad) == AD_SYMLINK || ad->ad_nosplice) {
        return 0;
    }

    fd = ad_fork_fileno(ad, eid, &pos);
    start = pos;
    cc = dsi_write_file(dsi, fd, &pos, &ad->ad_nosplice);
    *off += pos - start;

    /* every byte written counts, also before a failure, as with ad_write() */
    if (eid == ADEID_RFORK && pos > start && ad->ad_rlen < *off) {
        ad->ad_rlen = *off;
    }

    return cc;
}
#endif /* WITH_RECVFILE */
