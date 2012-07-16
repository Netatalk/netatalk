/*
  Copyright (c) 2012 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>

#include <atalk/errchk.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#include <atalk/talloc.h>
#include <atalk/dalloc.h>
#include <atalk/byteorder.h>
#include <atalk/netatalk_conf.h>
#include <atalk/volume.h>

#include "spotlight.h"

void *sl_module;
struct sl_module_export *sl_module_export;


/* Helper functions and stuff */
static const char *neststrings[] = {
    "",
    "\t",
    "\t\t",
    "\t\t\t",
    "\t\t\t\t",
    "\t\t\t\t\t",
    "\t\t\t\t\t\t",
};

static int dd_dump(DALLOC_CTX *dd, int nestinglevel)
{
    const char *type;

    LOG(log_debug, logtype_sl, "%s%s(#%d): {",
        neststrings[nestinglevel], talloc_get_name(dd), talloc_array_length(dd->dd_talloc_array));

    for (int n = 0; n < talloc_array_length(dd->dd_talloc_array); n++) {

        type = talloc_get_name(dd->dd_talloc_array[n]);

        if (STRCMP(type, ==, "DALLOC_CTX")
                   || STRCMP(type, ==, "sl_array_t")
                   || STRCMP(type, ==, "sl_dict_t")) {
            dd_dump(dd->dd_talloc_array[n], nestinglevel + 1);
        } else if (STRCMP(type, ==, "uint64_t")) {
            uint64_t i;
            memcpy(&i, dd->dd_talloc_array[n], sizeof(uint64_t));
            LOG(log_debug, logtype_sl, "%suint64_t: 0x%04x", neststrings[nestinglevel + 1], i);
        } else if (STRCMP(type, ==, "char *")) {
            char *s;
            memcpy(&s, dd->dd_talloc_array[n], sizeof(char *));
            LOG(log_debug, logtype_sl, "%sstring: %s", neststrings[nestinglevel + 1], s);
        } else if (STRCMP(type, ==, "sl_bool_t")) {
            sl_bool_t bl;
            memcpy(&bl, dd->dd_talloc_array[n], sizeof(sl_bool_t));
            LOG(log_debug, logtype_sl, "%sbool: %s", neststrings[nestinglevel + 1], bl ? "true" : "false");
        } else if (STRCMP(type, ==, "sl_cnids_t")) {
            sl_cnids_t cnids;
            memcpy(&cnids, dd->dd_talloc_array[n], sizeof(sl_cnids_t));
            LOG(log_debug, logtype_sl, "%sCNIDs: unkn1: %" PRIu16 ", unkn2: %" PRIu32,
                   neststrings[nestinglevel + 1], cnids.ca_unkn1, cnids.ca_context);
            if (cnids.ca_cnids)
                dd_dump(cnids.ca_cnids, nestinglevel + 1);
        }
    }
    LOG(log_debug, logtype_sl, "%s}", neststrings[nestinglevel]);
}

/**************************************************************************************************
 * Spotlight RPC functions
 **************************************************************************************************/

static int sl_rpc_fetchPropertiesForContext(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *v)
{
    EC_INIT;

    char *s;
    sl_dict_t *dict;
    sl_array_t *array;
    sl_uuid_t uuid;

    if (!v->v_uuid)
        EC_FAIL_LOG("sl_rpc_fetchPropertiesForContext: missing UUID for volume: %s", v->v_localname);

    dict = talloc_zero(reply, sl_dict_t);

    /* key/val 1 */
    s = talloc_strdup(dict, "kMDSStoreMetaScopes");
    dalloc_add(dict, &s, char *);

    array = talloc_zero(dict, sl_array_t);
    s = talloc_strdup(array, "kMDQueryScopeComputer");
    dalloc_add(array, &s, char *);
    dalloc_add(dict, array, sl_array_t);

    /* key/val 2 */
    s = talloc_strdup(dict, "kMDSStorePathScopes");
    dalloc_add(dict, &s, char *);

    array = talloc_zero(dict, sl_array_t);
    s = talloc_strdup(array, v->v_path);
    dalloc_add(array, &s, char *);
    dalloc_add(dict, array, sl_array_t);

    /* key/val 3 */
    s = talloc_strdup(dict, "kMDSStoreUUID");
    dalloc_add(dict, &s, char *);

    memcpy(uuid.sl_uuid, v->v_uuid, 16);
    dalloc_add(dict, &uuid, sl_uuid_t);

    /* key/val 4 */
    s = talloc_strdup(dict, "kMDSStoreHasPersistentUUID");
    dalloc_add(dict, &s, char *);
    sl_bool_t b = true;
    dalloc_add(dict, &b, sl_bool_t);

    dalloc_add(reply, dict, sl_dict_t);

EC_CLEANUP:
    EC_EXIT;
}

static int sl_rpc_openQuery(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *v)
{
    EC_INIT;

    char **sl_query;
    EC_NULL_LOG( sl_query = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1, "kMDQueryString") );

    LOG(log_debug, logtype_sl, "sl_rpc_openQuery: %s", *sl_query);

EC_CLEANUP:
    EC_EXIT;
}

/**************************************************************************************************
 * Spotlight module functions
 **************************************************************************************************/

int sl_mod_load(const char *path)
{
    EC_INIT;

    if ((sl_module = mod_open(path)) == NULL) {
        LOG(log_error, logtype_sl, "sl_mod_load(%s): failed to load: %s", path, mod_error());
        EC_FAIL;
    }

    if ((sl_module_export = mod_symbol(sl_module, "sl_mod")) == NULL) {
        LOG(log_error, logtype_sl, "sl_mod_load(%s): mod_symbol error for symbol %s", path, "sl_mod");
        EC_FAIL;
    }

    sl_module_export->sl_mod_init("test");
   
EC_CLEANUP:
    EC_EXIT;
}

/**************************************************************************************************
 * AFP functions
 **************************************************************************************************/
int afp_spotlight_rpc(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
    EC_INIT;
    TALLOC_CTX *tmp_ctx = talloc_new(NULL);
    uint16_t vid;
    int cmd;
    int endianess = SL_ENC_LITTLE_ENDIAN;
    struct vol      *vol;
    DALLOC_CTX *query;

    *rbuflen = 0;

    ibuf += 2;
    ibuflen -= 2;

    vid = SVAL(ibuf, 0);
    LOG(log_debug, logtype_sl, "afp_spotlight_rpc(vid: %" PRIu16 ")", vid);

    if ((vol = getvolbyvid(vid)) == NULL) {
        LOG(log_error, logtype_sl, "afp_spotlight_rpc: bad volume id: %" PRIu16 ")", vid);
        ret = AFPERR_ACCESS;
        goto EC_CLEANUP;
    }

    /*    IVAL(ibuf, 2): unknown, always 0x00008004, some flags ? */

    cmd = RIVAL(ibuf, 6);
    LOG(log_debug, logtype_sl, "afp_spotlight_rpc(cmd: %d)", cmd);

    /*    IVAL(ibuf, 10: unknown, always 0x00000000 */

    switch (cmd) {

    case SPOTLIGHT_CMD_OPEN:
    case SPOTLIGHT_CMD_OPEN2: {
        RSIVAL(rbuf, 0, ntohs(vid));
        RSIVAL(rbuf, 4, 0);
        int len = strlen(vol->v_path) + 1;
        strncpy(rbuf + 8, vol->v_path, len);
        *rbuflen += 8 + len;
        break;
    }
    case SPOTLIGHT_CMD_FLAGS:
        RSIVAL(rbuf, 0, 0x0100006b); /* Whatever this value means... flags? Helios uses 0x1eefface */
        *rbuflen += 4;
        break;

    case SPOTLIGHT_CMD_RPC: {
        DALLOC_CTX *query;
        EC_NULL( query = talloc_zero(tmp_ctx, DALLOC_CTX) );
        DALLOC_CTX *reply;
        EC_NULL( reply = talloc_zero(tmp_ctx, DALLOC_CTX) );

        EC_NEG1_LOG( sl_unpack(query, ibuf + 22) );
        dd_dump(query, 0);

        char **cmd;
        EC_NULL_LOG( cmd = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "char *", 0) );

        if (STRCMP(*cmd, ==, "fetchPropertiesForContext:")) {
            EC_ZERO_LOG( sl_rpc_fetchPropertiesForContext(obj, query, reply, vol) );
        } else if (STRCMP(*cmd, ==, "openQueryWithParams:forContext:")) {
            EC_ZERO_LOG( sl_rpc_openQuery(obj, query, reply, vol) );
        } else if (STRCMP(*cmd, ==, "fetchQueryResultsForContext:")) {
            uint64_t *p;
            if ((p = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 1)) != NULL) {
                LOG(log_info, logtype_sl, "fetchQueryResultsForContext: 0x%" PRIx64, *p);
            }
        }

        dd_dump(reply, 0);

        memset(rbuf, 0, 4);
        *rbuflen += 4;

        int len;
        EC_NEG1_LOG( len = sl_pack(reply, rbuf + 4) );
        *rbuflen += len;

        break;
    }
    }

EC_CLEANUP:
    talloc_free(tmp_ctx);
    if (ret != AFP_OK) {
        *rbuflen = 0;
        return AFPERR_MISC;
    }
    EC_EXIT;
}

/**************************************************************************************************
 * Testing
 **************************************************************************************************/

#ifdef SPOT_TEST_MAIN

int main(int argc, char **argv)
{
    EC_INIT;
    TALLOC_CTX *mem_ctx = talloc_new(NULL);
    DALLOC_CTX *dd = talloc_zero(mem_ctx, DALLOC_CTX);
    uint64_t i;

    set_processname("spot");
    setuplog("default:info,spotlight:debug", "/dev/tty");

    LOG(log_info, logtype_sl, "Start");

#if 0
    i = 2;
    dalloc_add(dd, &i, uint64_t);

    i = 1;
    dalloc_add(dd, &i, uint64_t);


    char *str = talloc_strdup(dd, "hello world");
    dalloc_add(dd, &str, char *);

    sl_bool_t b = true;
    dalloc_add(dd, &b, sl_bool_t);

    b = false;
    dalloc_add(dd, &b, sl_bool_t);


    /* add a nested array */
    DALLOC_CTX *nested = talloc_zero(dd, DALLOC_CTX);
    i = 3;
    dalloc_add(nested, &i, uint64_t);
    dalloc_add(dd, nested, DALLOC_CTX);

    /* test an allocated CNID array */
    uint64_t id = 16;
    sl_cnids_t *cnids = talloc_zero(dd, sl_cnids_t);

    cnids->ca_cnids = talloc_zero(cnids, DALLOC_CTX);

    cnids->ca_unkn1 = 1;
    cnids->ca_unkn2 = 2;

    dalloc_add(cnids->ca_cnids, &id, uint64_t);
    dalloc_add(dd, cnids, sl_cnids_t);

#endif

#if 0
    /* Now the Spotlight types */
    sl_array_t *sl_arrary = talloc_zero(dd, sl_array_t);
    i = 0x1234;
    dalloc_add(sl_arrary, &i, uint64_t);

    sl_dict_t *sl_dict = talloc_zero(dd, sl_dict_t);
    i = 0x5678;
    dalloc_add(sl_dict, &i, uint64_t);
    dalloc_add(sl_arrary, sl_dict, sl_dict_t);

    dalloc_add(dd, sl_arrary, sl_array_t);
    dd_dump(dd, 0);
#endif

    /* now parse a real spotlight packet */
    if (argc > 1) {
        char ibuf[8192];
        char rbuf[8192];
        int fd;
        size_t len;
        DALLOC_CTX *query;

        EC_NULL( query = talloc_zero(mem_ctx, DALLOC_CTX) );

        EC_NEG1_LOG( fd = open(argv[1], O_RDONLY) );
        EC_NEG1_LOG( len = read(fd, ibuf, 8192) );
        close(fd);
        EC_NEG1_LOG( sl_unpack(query, ibuf + 24) );

        /* Now dump the whole thing */
        dd_dump(query, 0);
    }

#if 0
    /* packing  */
    int qlen;
    char buf[MAX_SLQ_DAT];
    EC_NEG1_LOG( qlen = sl_pack(query, buf) );

    EC_NEG1_LOG( fd = open("test.bin", O_RDWR) );
    lseek(fd, 24, SEEK_SET);
    write(fd, buf, qlen);
    close(fd);
#endif

EC_CLEANUP:
    if (mem_ctx) {
        talloc_free(mem_ctx);
        mem_ctx = NULL;
    }
    EC_EXIT;
}
#endif
