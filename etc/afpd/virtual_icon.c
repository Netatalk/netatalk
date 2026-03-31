/*
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/globals.h>
#include <atalk/logger.h>

#include "file.h"
#include "directory.h"
#include "icon.h"
#include "virtual_icon.h"
#include "volume.h"


/* ---- Resource fork construction ---- */

/*
 * Mac resource fork binary format for Icon\r:
 *
 * [256-byte header]
 * [resource data section: 4-byte length prefix + data for each resource]
 * [resource map]
 *
 * We store three resources: ICN# (256 bytes), icl4 (512 bytes), icl8 (1024 bytes)
 * All with resource ID -16455 (0xBFB9) = volume custom icon ID.
 */

#define ICON_RES_ID   (-16455)  /* 0xBFB9 - Finder volume custom icon resource ID */

/*!
 * Write a big-endian uint32_t to buf
 */
static void put_u32(unsigned char *buf, uint32_t val)
{
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    buf[3] = val & 0xFF;
}

/*!
 * Write a big-endian uint16_t to buf
 */
static void put_u16(unsigned char *buf, uint16_t val)
{
    buf[0] = (val >> 8) & 0xFF;
    buf[1] = val & 0xFF;
}

/*!
 * @brief Build the Macintosh resource fork binary data containing ICN#, icl4, and icl8
 *
 * icn_hash: 256-byte ICN# resource (icon bitmap + mask)
 * icl4:     512-byte icl4 resource (32x32 4-bit color icon)
 * icl8:     1024-byte icl8 resource (32x32 8-bit color icon)
 */
static void build_resource_fork(struct vol *vol,
                                const unsigned char *icn_hash,
                                const unsigned char *icl4,
                                const unsigned char *icl8)
{
    unsigned char *p;
    uint32_t data_offset, map_offset, data_len, map_len;
    uint16_t type_list_offset, name_list_offset;
    uint32_t icn_data_off, icl4_data_off, icl8_data_off;
    /*
     * Data section: each resource is prefixed by a 4-byte length.
     * Layout:
     *   [4 + 256] ICN#
     *   [4 + 512] icl4
     *   [4 + 1024] icl8
     */
    data_len = (4 + ICN_HASH_SIZE) + (4 + ICL4_SIZE) + (4 + ICL8_SIZE);
    /*
     * Resource map layout:
     *   22 bytes: reserved(16) + handle(4) + fileref(2)
     *   2 bytes: attributes
     *   2 bytes: type_list_offset (from start of map)
     *   2 bytes: name_list_offset (from start of map)
     *   Type list: 2 bytes (num_types - 1)
     *              + 3 * (4 type + 2 count-1 + 2 ref_list_offset) = 3 * 8 = 24
     *   Ref list:  3 * (2 id + 2 name_offset + 1 attrs + 3 data_offset + 4 handle) = 3 * 12 = 36
     *   Total map: 28 + 2 + 24 + 36 = 90
     */
    type_list_offset = 28;  /* offset from map start to type list */
    name_list_offset = 28 + 2 + 24 + 36;  /* past everything, no names */
    map_len = name_list_offset;
    /* Header is at offset 0, data starts at 256 */
    data_offset = 256;
    map_offset = data_offset + data_len;
    vol->v_icon_rfork_len = map_offset + map_len;
    vol->v_icon_rfork = calloc(1, vol->v_icon_rfork_len);

    if (!vol->v_icon_rfork) {
        LOG(log_error, logtype_afpd, "build_resource_fork: malloc failed");
        vol->v_icon_rfork_len = 0;
        return;
    }

    /* ---- Resource header (256 bytes) ---- */
    p = vol->v_icon_rfork;
    put_u32(p, data_offset);       /* offset to resource data */
    put_u32(p + 4, map_offset);    /* offset to resource map */
    put_u32(p + 8, data_len);      /* length of resource data */
    put_u32(p + 12, map_len);      /* length of resource map */
    /* bytes 16-255: reserved (already zeroed) */
    /* ---- Resource data section ---- */
    p = vol->v_icon_rfork + data_offset;
    /* ICN# */
    icn_data_off = 0;
    put_u32(p, ICN_HASH_SIZE);
    p += 4;
    memcpy(p, icn_hash, ICN_HASH_SIZE);
    p += ICN_HASH_SIZE;
    /* icl4 */
    icl4_data_off = 4 + ICN_HASH_SIZE;
    put_u32(p, ICL4_SIZE);
    p += 4;
    memcpy(p, icl4, ICL4_SIZE);
    p += ICL4_SIZE;
    /* icl8 */
    icl8_data_off = icl4_data_off + 4 + ICL4_SIZE;
    put_u32(p, ICL8_SIZE);
    p += 4;
    memcpy(p, icl8, ICL8_SIZE);
    /* ---- Resource map ---- */
    p = vol->v_icon_rfork + map_offset;
    /* Copy of header fields */
    put_u32(p, data_offset);
    put_u32(p + 4, map_offset);
    put_u32(p + 8, data_len);
    put_u32(p + 12, map_len);
    /* bytes 16-21 */
    p += 22;
    /* Attributes (2 bytes) */
    put_u16(p, 0);
    p += 2;
    /* Type list offset (from map start) */
    put_u16(p, type_list_offset);
    p += 2;
    /* Name list offset (from map start) */
    put_u16(p, name_list_offset);
    p += 2;
    /* ---- Type list ---- */
    /* Number of types minus 1 */
    put_u16(p, 2);  /* 3 types - 1 = 2 */
    p += 2;
    /* Ref list starts right after type list entries.
     * Type list is 2 + 3*8 = 26 bytes from start of type list.
     * Ref list offset is relative to the type list start (the num_types field). */
    uint16_t ref_list_base = 2 + 3 * 8;  /* 26 */
    /* Type entry 0: ICN# */
    put_u32(p, RESTYPE_ICNH);
    put_u16(p + 4, 0);  /* count - 1 = 0 (one resource of this type) */
    put_u16(p + 6, ref_list_base + 0 * 12);  /* offset to first ref entry */
    p += 8;
    /* Type entry 1: icl4 */
    put_u32(p, RESTYPE_ICL4);
    put_u16(p + 4, 0);
    put_u16(p + 6, ref_list_base + 1 * 12);
    p += 8;
    /* Type entry 2: icl8 */
    put_u32(p, RESTYPE_ICL8);
    put_u16(p + 4, 0);
    put_u16(p + 6, ref_list_base + 2 * 12);
    p += 8;
    /* ---- Reference list entries ---- */
    /* Each entry: 2 id + 2 name_offset + 1 attrs + 3 data_offset + 4 handle = 12 bytes */
    /* Ref for ICN# */
    put_u16(p, (uint16_t)ICON_RES_ID);  /* resource ID -16455 */
    put_u16(p + 2, 0xFFFF);             /* no name (0xFFFF) */
    p[4] = 0;                           /* attributes */
    p[5] = (icn_data_off >> 16) & 0xFF;
    p[6] = (icn_data_off >> 8) & 0xFF;
    p[7] = icn_data_off & 0xFF;
    put_u32(p + 8, 0);                  /* reserved for handle */
    p += 12;
    /* Ref for icl4 */
    put_u16(p, (uint16_t)ICON_RES_ID);
    put_u16(p + 2, 0xFFFF);
    p[4] = 0;
    p[5] = (icl4_data_off >> 16) & 0xFF;
    p[6] = (icl4_data_off >> 8) & 0xFF;
    p[7] = icl4_data_off & 0xFF;
    put_u32(p + 8, 0);                  /* reserved for handle */
    p += 12;
    /* Ref for icl8 */
    put_u16(p, (uint16_t)ICON_RES_ID);
    put_u16(p + 2, 0xFFFF);
    p[4] = 0;
    p[5] = (icl8_data_off >> 16) & 0xFF;
    p[6] = (icl8_data_off >> 8) & 0xFF;
    p[7] = icl8_data_off & 0xFF;
    put_u32(p + 8, 0);                  /* reserved for handle */
}

/* ---- Public API ---- */

int virtual_icon_enabled(const struct vol *vol)
{
    return (vol->v_icon_rfork != NULL);
}

int real_icon_exists(const struct vol *vol)
{
    char path[MAXPATHLEN + 1];
    struct stat st;
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, VIRTUAL_ICON_NAME);
    return (lstat(path, &st) == 0);
}

int is_virtual_icon_name(const char *name)
{
    return (strcmp(name, VIRTUAL_ICON_NAME) == 0);
}

void virtual_icon_init(struct vol *vol)
{
    const unsigned char *icon;
    const unsigned char *icon_icl4;
    const unsigned char *icon_icl8;

    if (vol->v_icon_rfork) {
        return;
    }

    if (!vol->v_legacyicon || vol->v_legacyicon[0] == '\0') {
        return;
    }

    if (strcmp(vol->v_legacyicon, "daemon") == 0) {
        icon = daemon_icon;
        icon_icl4 = daemon_icon_icl4;
        icon_icl8 = daemon_icon_icl8;
    } else if (strcmp(vol->v_legacyicon, "declogo") == 0) {
        icon = declogo_icon;
        icon_icl4 = declogo_icon_icl4;
        icon_icl8 = declogo_icon_icl8;
    } else if (strcmp(vol->v_legacyicon, "fileserver") == 0) {
        icon = fileserver_icon;
        icon_icl4 = fileserver_icon_icl4;
        icon_icl8 = fileserver_icon_icl8;
    } else if (strcmp(vol->v_legacyicon, "globe") == 0) {
        icon = globe_icon;
        icon_icl4 = globe_icon_icl4;
        icon_icl8 = globe_icon_icl8;
    } else if (strcmp(vol->v_legacyicon, "hagar") == 0) {
        icon = hagar_icon;
        icon_icl4 = hagar_icon_icl4;
        icon_icl8 = hagar_icon_icl8;
    } else if (strcmp(vol->v_legacyicon, "nas") == 0) {
        icon = nas_icon;
        icon_icl4 = nas_icon_icl4;
        icon_icl8 = nas_icon_icl8;
    } else if (strcmp(vol->v_legacyicon, "sdcard") == 0) {
        icon = sdcard_icon;
        icon_icl4 = sdcard_icon_icl4;
        icon_icl8 = sdcard_icon_icl8;
    } else if (strcmp(vol->v_legacyicon, "sunlogo") == 0) {
        icon = sunlogo_icon;
        icon_icl4 = sunlogo_icon_icl4;
        icon_icl8 = sunlogo_icon_icl8;
    } else {
        LOG(log_warning, logtype_afpd, "virtual_icon_init: unknown legacy icon '%s'",
            vol->v_legacyicon);
        return;
    }

    build_resource_fork(vol, icon, icon_icl4, icon_icl8);
}

const unsigned char *virtual_icon_get_rfork(const struct vol *vol,
        size_t *outlen)
{
    if (!vol->v_icon_rfork) {
        *outlen = 0;
        return NULL;
    }

    *outlen = vol->v_icon_rfork_len;
    return vol->v_icon_rfork;
}

/*!
 * @brief Synthesize AFP file parameters for the virtual Icon\\r file
 *
 * This fills the reply buffer with parameters matching the requested bitmap,
 * just as getmetadata/getfilparams would for a real file.
 */
int virtual_icon_getfilparams(const AFPObj *obj,
                              struct vol *vol,
                              uint16_t bitmap,
                              char *buf,
                              size_t *buflen)
{
    char *data = buf;
    char *l_nameoff = NULL;
    char *utf_nameoff = NULL;
    uint16_t ashort;
    uint32_t aint;
    uint32_t utf8 = 0;
    int bit = 0;
    uint16_t bmap = bitmap;
    cnid_t id = htonl(VIRTUAL_ICON_CNID);
    cnid_t pdid = DIRDID_ROOT;
    char name[] = VIRTUAL_ICON_NAME;

    if (!vol->v_icon_rfork) {
        return AFPERR_MISC;
    }

    while (bmap != 0) {
        while ((bmap & 1) == 0) {
            bmap >>= 1;
            bit++;
        }

        switch (bit) {
        case FILPBIT_ATTR:
            ashort = htons(ATTRBIT_INVISIBLE);
            memcpy(data, &ashort, sizeof(ashort));
            data += sizeof(ashort);
            break;

        case FILPBIT_PDID:
            memcpy(data, &pdid, sizeof(pdid));
            data += sizeof(pdid);
            break;

        case FILPBIT_CDATE:
        case FILPBIT_MDATE:
        case FILPBIT_BDATE:
            aint = AD_DATE_START;
            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            break;

        case FILPBIT_FINFO: {
            memset(data, 0, ADEDLEN_FINDERI);
            ashort = htons(FINDERINFO_INVISIBLE);
            memcpy(data + FINDERINFO_FRFLAGOFF, &ashort, sizeof(ashort));
            data += ADEDLEN_FINDERI;
            break;
        }

        case FILPBIT_LNAME:
            l_nameoff = data;
            data += sizeof(uint16_t);
            break;

        case FILPBIT_SNAME:
            memset(data, 0, sizeof(uint16_t));
            data += sizeof(uint16_t);
            break;

        case FILPBIT_FNUM:
            memcpy(data, &id, sizeof(id));
            data += sizeof(id);
            break;

        case FILPBIT_DFLEN:
            aint = 0;  /* empty data fork */
            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            break;

        case FILPBIT_RFLEN:
            aint = htonl((uint32_t)vol->v_icon_rfork_len);
            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            break;

        case FILPBIT_EXTDFLEN:
            memset(data, 0, 8);  /* 64-bit zero data fork length */
            data += 8;
            break;

        case FILPBIT_EXTRFLEN: {
            uint32_t hi = 0;
            uint32_t lo = htonl((uint32_t)vol->v_icon_rfork_len);
            memcpy(data, &hi, sizeof(hi));
            data += sizeof(hi);
            memcpy(data, &lo, sizeof(lo));
            data += sizeof(lo);
            break;
        }

        case FILPBIT_PDINFO:
            if (obj->afp_version >= 30) {
                utf8 = kTextEncodingUTF8;
                utf_nameoff = data;
                data += sizeof(uint16_t);
                aint = 0;
                memcpy(data, &aint, sizeof(aint));
                data += sizeof(aint);
            } else {
                memset(data, 0, 6);
                data += 6;
            }

            break;

        case FILPBIT_UNIXPR:
            aint = 0;
            memcpy(data, &aint, sizeof(aint));  /* uid */
            data += sizeof(aint);
            memcpy(data, &aint, sizeof(aint));  /* gid */
            data += sizeof(aint);
            aint = htonl(S_IFREG | 0444);       /* mode: regular file, read-only */
            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            *data++ = AR_UREAD;   /* user access */
            *data++ = AR_UREAD;   /* world access */
            *data++ = AR_UREAD;   /* group access */
            *data++ = AR_UREAD;   /* owner access */
            break;

        default:
            return AFPERR_BITMAP;
        }

        bmap >>= 1;
        bit++;
    }

    if (l_nameoff) {
        ashort = htons((uint16_t)(data - buf));
        memcpy(l_nameoff, &ashort, sizeof(ashort));
        data = set_name(vol, data, DIRDID_ROOT, (char *)name,
                        htonl(VIRTUAL_ICON_CNID), 0);
    }

    if (utf_nameoff) {
        ashort = htons((uint16_t)(data - buf));
        memcpy(utf_nameoff, &ashort, sizeof(ashort));
        data = set_name(vol, data, DIRDID_ROOT, (char *)name,
                        htonl(VIRTUAL_ICON_CNID), utf8);
    }

    *buflen = data - buf;
    return AFP_OK;
}
