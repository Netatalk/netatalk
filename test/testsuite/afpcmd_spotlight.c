/*
 * test/testsuite/afpcmd_spotlight.c — Spotlight test helpers.
 *
 * Isolated from afpcmd.c so we can hard-code SL_PACK_BUFLEN against
 * libatalk's MAX_SLQ_DAT *without* including <atalk/dsi.h>. There
 * are two reasons not to include the libatalk header here:
 *
 *   1. Macro shadow — libatalk's DSI_DATASIZ (= 65536) and the
 *      testsuite-local DSI_DATASIZ (= 8192) share the header guard
 *      _ATALK_DSI_H. Including <atalk/dsi.h> would suppress the
 *      testsuite-local dsi.h that backs Conn->dsi.
 *   2. Struct-layout mismatch — the libatalk DSI struct
 *      (commands as a heap pointer, no clientID) and the testsuite
 *      DSI struct (commands[800] inline, clientID field) are
 *      layout-incompatible. SendInit/SetLen are compiled in
 *      afpclient.c against the testsuite DSI; calling them from
 *      this TU with a (libatalk DSI *) would be a silent cross-TU
 *      ABI break.
 *
 * Layering rule for this TU:
 *   - INCLUDE: <talloc.h>, <atalk/afp.h>, <atalk/spotlight.h>,
 *              <atalk/dalloc.h>, "afpcmd.h", "afpclient.h"
 *   - DO NOT INCLUDE: <atalk/dsi.h>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* afpcmd.h / afpclient.h MUST be included first so the testsuite-local
 * <test/testsuite/dsi.h> sets the _ATALK_DSI_H header guard before any
 * libatalk header chain (e.g. via <atalk/spotlight.h>) attempts to pull
 * in <atalk/dsi.h>. The testsuite DSI struct (commands[800] inline +
 * clientID) is what backs Conn->dsi and what SendInit/SetLen are
 * compiled against in afpclient.c; if libatalk's DSI struct (heap
 * commands pointer, no clientID) wins instead, the layout mismatch
 * is a silent cross-TU ABI break. */
#include "afpcmd.h"
#include "afpclient.h"

#include <talloc.h>

#include <atalk/afp.h>
#include <atalk/spotlight.h>
#include <atalk/dalloc.h>

/* Locally declared so we do not pull in testhelper.h. */
extern int Quiet;

/*
 * SL_PACK_BUFLEN tracks libatalk's MAX_SLQ_DAT
 * (= libatalk DSI_DATASIZ - 64). Hard-coded to the literal 65472
 * because we deliberately do NOT include <atalk/dsi.h> here (see the
 * file header comment above for the macro-shadow / struct-layout
 * rationale). If libatalk's DSI_DATASIZ ever changes, also update
 * this constant — the matching reminder lives at
 * etc/afpd/spotlight_marshalling.c next to MAX_SLQ_DAT.
 */
#define SL_PACK_BUFLEN  65472

/*!
 * @brief Wrap an AFP_SPOTLIGHT_PRIVATE request and ship it on the DSI
 *
 * Builds the 24-byte SPOTLIGHT_PRIVATE envelope (cmd byte, vid, reserved,
 * subcmd, reserved, padding) and appends @p rpc_buf, then sends and
 * receives the reply via the existing my_dsi_* helpers.
 */
static unsigned int spotlight_send(CONN *conn,
                                   uint16_t vid,
                                   uint32_t subcmd,
                                   const uint8_t *rpc_buf,
                                   size_t rpc_len)
{
    DSI     *dsi = &conn->dsi;
    uint32_t u32_n;
    int      ofs = 0;

    if (rpc_len > (size_t)DSI_CMDSIZ - 24) {
        if (!Quiet) {
            fprintf(stderr,
                    "[%s] payload size %zu exceeds DSI_CMDSIZ-24=%d\n",
                    __func__, rpc_len, DSI_CMDSIZ - 24);
        }

        return htonl(AFPERR_PARAM);
    }

    /* vid is already in network byte order — VolID is stored as
     * htons(vid_le) by AFPOpenVol and consumed verbatim by every other
     * SendCmdWithU16-based AFP request (e.g. FPCloseVol, FPGetVolParms) */
    /* SendInit/SetLen are global helpers in afpclient.c. */
    SendInit(dsi);
    dsi->commands[ofs++] = AFP_SPOTLIGHT_PRIVATE;
    dsi->commands[ofs++] = 0;
    /* vid is already network-byte-order, do NOT htons() again. */
    memcpy(dsi->commands + ofs, &vid, sizeof(vid));
    ofs += sizeof(vid);
    u32_n = htonl(0x00008004);
    memcpy(dsi->commands + ofs, &u32_n, sizeof(u32_n));
    ofs += sizeof(u32_n);
    u32_n = htonl(subcmd);
    memcpy(dsi->commands + ofs, &u32_n, sizeof(u32_n));
    ofs += sizeof(u32_n);
    u32_n = htonl(0x00000000);
    memcpy(dsi->commands + ofs, &u32_n, sizeof(u32_n));
    ofs += sizeof(u32_n);
    /* 8-byte header padding, ofs == 24 here. */
    memset(dsi->commands + ofs, 0, 8);
    ofs += 8;

    if (rpc_len > 0) {
        memcpy(dsi->commands + ofs, rpc_buf, rpc_len);
        ofs += (int)rpc_len;
    }

    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

/*!
 * @brief Send SPOTLIGHT_CMD_OPEN and optionally extract the volume path
 *
 * On success the reply layout is vid (4) + zero (4) + NUL-terminated path
 * starting at dsi->data + 8; if @p vol_path_out is non-NULL the path is
 * copied via strlcpy bounded by @p vol_path_buflen.
 */
unsigned int FPSpotlightOpen(CONN *conn, uint16_t vid,
                             char *vol_path_out, size_t vol_path_buflen)
{
    unsigned int ret;

    if (!Quiet) {
        fprintf(stdout, "[%s] FPSpotlightOpen vol: %u\n",
                __func__, (unsigned)ntohs(vid));
    }

    ret = spotlight_send(conn, vid, SPOTLIGHT_CMD_OPEN, NULL, 0);

    if (ret == AFP_OK && vol_path_out != NULL) {
        /* Reply: vid (4) + zero (4) + null-terminated path */
        const DSI *dsi = &conn->dsi;
        strlcpy(vol_path_out, (const char *)dsi->data + 8,
                vol_path_buflen);
    }

    return ret;
}

/*!
 * @brief Build and send an openQueryWithParams:forContext: Spotlight RPC
 *
 * Mirrors sl_rpc_openQuery's expectations exactly. The outer DALLOC_CTX
 * holds one sl_array_t (`outer_array`) with two children:
 *   [0] args : sl_array_t with three elements
 *       [0] = "openQueryWithParams:forContext:"  (char *)
 *       [1] = ctx1  (uint64_t)
 *       [2] = ctx2  (uint64_t)
 *   [1] params : sl_dict_t — key/value pairs (kMDQueryString → DSL,
 *                                              kMDAttributeArray → sl_array_t,
 *                                              kMDScopeArray → char *)
 *
 * Stack-local scalars use dalloc_add_copy so the value is memcpy'd into a
 * freshly-talloc'd chunk; dalloc_add only captures a pointer.
 *
 * Both ctx values are needed because sl_rpc_openQuery dereferences both
 * via dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 1)
 * and "uint64_t", 2.
 */
unsigned int FPSpotlightOpenQuery(CONN *conn, uint16_t vid,
                                  const char *query_dsl, uint64_t ctx)
{
    /* sl_pack writes up to SL_PACK_BUFLEN bytes regardless of caller
     * buffer; see comment at top of this TU. */
    char         rpcbuf[SL_PACK_BUFLEN];
    int          rpclen;
    TALLOC_CTX  *tmp          = talloc_new(NULL);
    DALLOC_CTX  *outer        = talloc_zero(tmp,    DALLOC_CTX);
    sl_array_t  *outer_array  = talloc_zero(outer,  sl_array_t);
    sl_array_t  *args         = talloc_zero(outer_array, sl_array_t);
    sl_dict_t   *params       = talloc_zero(outer_array, sl_dict_t);
    sl_array_t  *attrs        = talloc_zero(params, sl_array_t);
    uint64_t     ctxval1      = ctx;
    uint64_t     ctxval2      = ctx;

    if (!Quiet) {
        fprintf(stdout, "[%s] vol=%u ctx=%llu query=\"%s\"\n",
                __func__, (unsigned)ntohs(vid),
                (unsigned long long)ctx, query_dsl);
    }

    /* args : ["openQueryWithParams:forContext:", ctx1, ctx2] */
    dalloc_add(args, dalloc_strdup(args, "openQueryWithParams:forContext:"),
               "char *");
    dalloc_add_copy(args, &ctxval1, uint64_t);
    dalloc_add_copy(args, &ctxval2, uint64_t);
    /* attrs : sl_array_t of attribute name strings the client wants
     * returned. Match the minimal attribute set actually consumed by
     * add_filemeta() in etc/afpd/spotlight.c so the daemon does not
     * EC_FAIL on a missing kMDAttributeArray key. */
    dalloc_add(attrs, dalloc_strdup(attrs, "kMDItemFSName"), "char *");
    /* params : sl_dict_t — alternating key/value pairs */
    dalloc_add(params, dalloc_strdup(params, "kMDQueryString"), "char *");
    dalloc_add(params, dalloc_strdup(params, query_dsl), "char *");
    dalloc_add(params, dalloc_strdup(params, "kMDAttributeArray"), "char *");
    dalloc_add(params, attrs, "sl_array_t");
    /* outer_array : [args, params] */
    dalloc_add(outer_array, args, "sl_array_t");
    dalloc_add(outer_array, params, "sl_dict_t");
    /* outer : [outer_array] */
    dalloc_add(outer, outer_array, "sl_array_t");
    rpclen = sl_pack(outer, rpcbuf);
    talloc_free(tmp);

    if (rpclen < 0) {
        return htonl(AFPERR_PARAM);
    }

    /* AFP command-channel envelope budget: payload must fit dsi->commands[]
     * after the 24-byte SPOTLIGHT_PRIVATE envelope. */
    if ((size_t)rpclen > (size_t)DSI_CMDSIZ - 24) {
        if (!Quiet) {
            fprintf(stderr,
                    "[%s] sl_pack produced %d bytes, exceeds "
                    "DSI_CMDSIZ-24=%d\n",
                    __func__, rpclen, DSI_CMDSIZ - 24);
        }

        return htonl(AFPERR_PARAM);
    }

    return spotlight_send(conn, vid, SPOTLIGHT_CMD_RPC,
                          (const uint8_t *)rpcbuf, (size_t)rpclen);
}

/*!
 * @brief Drain Spotlight query results until the server reports complete
 *
 * Loops fetchQueryResultsForContext: until the embedded status field is
 * 0 (search complete) or the loop guard expires. Sums the
 * kMDQueryResultIndices array length from each reply.
 *
 * The SPOTLIGHT_CMD_RPC reply envelope has 4 leading bytes before the
 * marshalled DALLOC_CTX (matches the server's sl_pack(reply, rbuf+4)).
 */
unsigned int FPSpotlightDrainResults(CONN *conn, uint16_t vid,
                                     uint64_t ctx,
                                     int *total_results_out)
{
    int          total      = 0;
    int          loop_guard = 1000;
    unsigned int ret;
    char         rpcbuf[SL_PACK_BUFLEN];
    int          rpclen;
    TALLOC_CTX  *tmp;
    DALLOC_CTX  *outer, *reply;
    sl_array_t  *args;
    uint64_t     ctxval;
    uint64_t     status = 35;
    const sl_cnids_t  *cnids;
    const DSI   *dsi = &conn->dsi;

    while (loop_guard-- > 0) {
        sl_dict_t *empty_dict;
        sl_array_t *outer_array;
        tmp         = talloc_new(NULL);
        outer       = talloc_zero(tmp, DALLOC_CTX);
        outer_array = talloc_zero(outer, sl_array_t);
        args        = talloc_zero(outer_array, sl_array_t);
        empty_dict  = talloc_zero(outer_array, sl_dict_t);
        ctxval      = ctx;
        /* args : [method-name, ctx1, ctx2] — sl_rpc_fetchQueryResultsForContext
         * dereferences ctx1/ctx2 via dalloc_get(query, "DALLOC_CTX", 0,
         * "DALLOC_CTX", 0, "uint64_t", 1) and "uint64_t", 2). */
        dalloc_add(args, dalloc_strdup(args,
                                       "fetchQueryResultsForContext:"),
                   "char *");
        dalloc_add_copy(args, &ctxval, uint64_t);
        dalloc_add_copy(args, &ctxval, uint64_t);
        /* outer_array : [args, empty_dict] — params slot is required by
         * the dispatch layout even when no parameters are sent. */
        dalloc_add(outer_array, args, "sl_array_t");
        dalloc_add(outer_array, empty_dict, "sl_dict_t");
        dalloc_add(outer, outer_array, "sl_array_t");
        rpclen = sl_pack(outer, rpcbuf);
        talloc_free(tmp);

        if (rpclen < 0) {
            return htonl(AFPERR_PARAM);
        }

        if ((size_t)rpclen > (size_t)DSI_CMDSIZ - 24) {
            if (!Quiet) {
                fprintf(stderr,
                        "[%s] sl_pack produced %d bytes, exceeds "
                        "DSI_CMDSIZ-24=%d\n",
                        __func__, rpclen, DSI_CMDSIZ - 24);
            }

            return htonl(AFPERR_PARAM);
        }

        ret = spotlight_send(conn, vid, SPOTLIGHT_CMD_RPC,
                             (const uint8_t *)rpcbuf, (size_t)rpclen);

        if (ret != AFP_OK) {
            return ret;
        }

        tmp   = talloc_new(NULL);
        reply = talloc_zero(tmp, DALLOC_CTX);

        if (sl_unpack(reply, (const char *)dsi->data + 4) < 0) {
            talloc_free(tmp);
            return htonl(AFPERR_PARAM);
        }

        /*
         * dalloc_get() type-string contract: "DALLOC_CTX" is a magic
         * descender token (NOT a literal talloc-name string). It tells
         * dalloc_get to step into the inner DALLOC_CTX without
         * performing a talloc_check_name test. The reply[0] element is
         * talloc-named "sl_array_t", but the descender token bypasses
         * that check and lets us reach scalar/named children inside.
         */
        {
            const uint64_t *status_p = dalloc_get(reply, "DALLOC_CTX", 0,
                                                  "uint64_t", 0);

            if (status_p == NULL) {
                talloc_free(tmp);
                return htonl(AFPERR_PARAM);
            }

            status = *status_p;
        }
        /* sl_cnids_t sits at index 1 of the inner reply array (status
         * uint64_t at index 0; sl_cnids_t at 1; sl_filemeta_t at 2),
         * matching the server's add_results(). */
        cnids = dalloc_get(reply, "DALLOC_CTX", 0, "sl_cnids_t", 1);

        if (cnids != NULL && cnids->ca_cnids != NULL) {
            total += (int)talloc_array_length(
                         cnids->ca_cnids->dd_talloc_array);
        }

        talloc_free(tmp);

        if (status == 0) {
            break;
        }
    }

    if (status != 0 && loop_guard <= 0) {
        if (!Quiet) {
            fprintf(stderr,
                    "[%s] loop guard exhausted with status=%llu\n",
                    __func__, (unsigned long long)status);
        }

        return htonl(AFPERR_MISC);
    }

    if (total_results_out != NULL) {
        *total_results_out = total;
    }

    return AFP_OK;
}

/*!
 * @brief Send a closeQueryForContext: Spotlight RPC for the given context
 */
unsigned int FPSpotlightCloseQuery(CONN *conn, uint16_t vid, uint64_t ctx)
{
    char         rpcbuf[SL_PACK_BUFLEN];
    int          rpclen;
    TALLOC_CTX  *tmp         = talloc_new(NULL);
    DALLOC_CTX  *outer       = talloc_zero(tmp, DALLOC_CTX);
    sl_array_t  *outer_array = talloc_zero(outer, sl_array_t);
    sl_array_t  *args        = talloc_zero(outer_array, sl_array_t);
    sl_dict_t   *empty_dict  = talloc_zero(outer_array, sl_dict_t);
    uint64_t     ctxval      = ctx;

    if (!Quiet) {
        fprintf(stdout, "[%s] vol=%u ctx=%llu\n",
                __func__, (unsigned)ntohs(vid),
                (unsigned long long)ctx);
    }

    /* args : [method-name, ctx1, ctx2] — sl_rpc_closeQueryForContext
     * dereferences ctx1/ctx2 via dalloc_get(... "uint64_t", 1) and ", 2). */
    dalloc_add(args, dalloc_strdup(args, "closeQueryForContext:"), "char *");
    dalloc_add_copy(args, &ctxval, uint64_t);
    dalloc_add_copy(args, &ctxval, uint64_t);
    /* outer_array : [args, empty_dict] — params slot is required by the
     * dispatch layout even when no parameters are sent. */
    dalloc_add(outer_array, args, "sl_array_t");
    dalloc_add(outer_array, empty_dict, "sl_dict_t");
    dalloc_add(outer, outer_array, "sl_array_t");
    rpclen = sl_pack(outer, rpcbuf);
    talloc_free(tmp);

    if (rpclen < 0) {
        return htonl(AFPERR_PARAM);
    }

    if ((size_t)rpclen > (size_t)DSI_CMDSIZ - 24) {
        if (!Quiet) {
            fprintf(stderr,
                    "[%s] sl_pack produced %d bytes, exceeds "
                    "DSI_CMDSIZ-24=%d\n",
                    __func__, rpclen, DSI_CMDSIZ - 24);
        }

        return htonl(AFPERR_PARAM);
    }

    return spotlight_send(conn, vid, SPOTLIGHT_CMD_RPC,
                          (const uint8_t *)rpcbuf, (size_t)rpclen);
}
