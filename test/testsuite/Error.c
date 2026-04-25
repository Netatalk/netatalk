/* ----------------------------------------------
*/
#include <atalk/afp_util.h>

#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

struct t35_cmd {
    unsigned char cmd;
    int expected;
    int min_version;
    unsigned char variant;
    int required_attrs;
};

#define T35_DEFAULT          0
#define T35_COPYFILE_DEST   1

/* ------------------------- */
static struct t35_cmd afp_cmd_with_vol[] = {
    { AFP_CLOSEVOL, AFPERR_PARAM, 21 },			/* 2 */
    { AFP_CLOSEDIR, AFPERR_PARAM, 21 },			/* 3 */
    { AFP_COPYFILE, AFPERR_PARAM, 21 },			/* 5 */
    { AFP_COPYFILE, AFPERR_PARAM, 21, T35_COPYFILE_DEST },	/* 5, DestVolumeID */
    { AFP_CREATEDIR, AFPERR_PARAM, 21 },			/* 6 */
    { AFP_CREATEFILE, AFPERR_PARAM, 21 },		/* 7 */
    { AFP_DELETE, AFPERR_PARAM, 21 },			/* 8 */
    { AFP_ENUMERATE, AFPERR_PARAM, 21 },		/* 9 */
    { AFP_FLUSH, AFPERR_PARAM, 21 },			/* 10 */

    { AFP_GETVOLPARAM, AFPERR_PARAM, 21 },		/* 17 */
    { AFP_MOVE, AFPERR_PARAM, 21 },			/* 23 */
    { AFP_OPENDIR, AFPERR_PARAM, 21 },			/* 25 */
    { AFP_OPENFORK, AFPERR_PARAM, 21 },			/* 26 */
    { AFP_RENAME, AFPERR_PARAM, 21 },			/* 28 */
    { AFP_SETDIRPARAM, AFPERR_PARAM, 21 },		/* 29 */
    { AFP_SETFILEPARAM, AFPERR_PARAM, 21 },		/* 30 */
    { AFP_SETVOLPARAM, AFPERR_PARAM, 21 },		/* 32 */
    { AFP_GETFLDRPARAM, AFPERR_PARAM, 21 },		/* 34 */
    { AFP_SETFLDRPARAM, AFPERR_PARAM, 21 },		/* 35 */
    { AFP_CREATEID, AFPERR_PARAM, 21 },			/* 39 */
    { AFP_DELETEID, AFPERR_PARAM, 21 },			/* 40 */
    { AFP_RESOLVEID, AFPERR_PARAM, 21 },		/* 41 */
    { AFP_EXCHANGEFILE, AFPERR_PARAM, 21 },		/* 42 */
    { AFP_CATSEARCH, AFPERR_PARAM, 21 },		/* 43 */
    { AFP_OPENDT, AFPERR_PARAM, 21 },			/* 48 */

    { AFP_ENUMERATE_EXT, AFPERR_PARAM, 30 },		/* 66 */
    { AFP_CATSEARCH_EXT, AFPERR_PARAM, 30 },		/* 67 */
    { AFP_ENUMERATE_EXT2, AFPERR_PARAM, 31 },		/* 68 */
    { AFP_GETEXTATTR, AFPERR_PARAM, 32 },		/* 69 */
    { AFP_SETEXTATTR, AFPERR_PARAM, 32 },		/* 70 */
    { AFP_REMOVEATTR, AFPERR_PARAM, 32 },		/* 71 */
    { AFP_LISTEXTATTR, AFPERR_PARAM, 32 },		/* 72 */
#ifdef HAVE_ACLS
    { AFP_GETACL, AFPERR_PARAM, 32, T35_DEFAULT, VOLPBIT_ATTR_ACLS },	/* 73 */
    { AFP_SETACL, AFPERR_PARAM, 32, T35_DEFAULT, VOLPBIT_ATTR_ACLS },	/* 74 */
    { AFP_ACCESS, AFPERR_PARAM, 32, T35_DEFAULT, VOLPBIT_ATTR_ACLS },	/* 75 */
#endif
    { AFP_SYNCDIR, AFPERR_NOOBJ, 31 },			/* 78 */
};

static struct t35_cmd afp_cmd_with_dt[] = {
    { AFP_CLOSEDT, AFPERR_PARAM, 21 },			/* 49 */
    { AFP_GETICON, AFPERR_PARAM, 21 },			/* 51 */
    { AFP_GTICNINFO, AFPERR_PARAM, 21 },		/* 52 */
    { AFP_ADDAPPL, AFPERR_PARAM, 21 },			/* 53 */
    { AFP_RMVAPPL, AFPERR_PARAM, 21 },			/* 54 */
    { AFP_GETAPPL, AFPERR_PARAM, 21 },			/* 55 */
    { AFP_ADDCMT, AFPERR_PARAM, 21 },			/* 56 */
    { AFP_RMVCMT, AFPERR_PARAM, 21 },			/* 57 */
    { AFP_GETCMT, AFPERR_PARAM, 21 },			/* 58 */
    { AFP_ADDICON, AFPERR_PARAM, 21 },			/* 192 */
};

static int t35_add_u16(unsigned char *buf, int ofs, uint16_t value)
{
    memcpy(buf + ofs, &value, sizeof(value));
    return ofs + sizeof(value);
}

static int t35_add_u32(unsigned char *buf, int ofs, uint32_t value)
{
    memcpy(buf + ofs, &value, sizeof(value));
    return ofs + sizeof(value);
}

static int t35_add_u64(unsigned char *buf, int ofs, uint64_t value)
{
    memcpy(buf + ofs, &value, sizeof(value));
    return ofs + sizeof(value);
}

static int t35_add_did(unsigned char *buf, int ofs)
{
    uint32_t did = DIRDID_ROOT;
    return t35_add_u32(buf, ofs, did);
}

static int t35_add_name(int ofs, char *name)
{
    return FPset_name(Conn, ofs, name);
}

static int t35_add_extattr_name(unsigned char *buf, int ofs, const char *name)
{
    size_t len;
    size_t maxlen;
    size_t remaining;
    uint16_t netlen;

    if (ofs < 0 || (size_t)ofs > DSI_CMDSIZ - sizeof(netlen)) {
        return -1;
    }

    remaining = DSI_CMDSIZ - (size_t)ofs - sizeof(netlen);
    maxlen = remaining < UINT16_MAX ? remaining : UINT16_MAX;
    len = strnlen(name, maxlen + 1);

    if (len > maxlen) {
        return -1;
    }

    netlen = htons((uint16_t) len);
    ofs = t35_add_u16(buf, ofs, netlen);
    memcpy(buf + ofs, name, len);
    return ofs + (int)len;
}

static int t35_add_filedir_parms(unsigned char *buf, int ofs, uint16_t bitmap,
                                 int isdir)
{
    struct afp_filedir_parms filedir = { 0 };
    filedir.isdir = isdir;
    return ofs + afp_filedir_pack(buf + ofs, &filedir, bitmap, bitmap);
}

static const char *t35_variant_name(unsigned char variant)
{
    switch (variant) {
    case T35_COPYFILE_DEST:
        return " DestVolumeID";

    default:
        return "";
    }
}

static int t35_cmd_supported(const struct t35_cmd *entry, int vol_attrs)
{
    if (Conn->afp_version < entry->min_version) {
        return 0;
    }

    return (entry->required_attrs & vol_attrs) == entry->required_attrs;
}

static int t35_build_request(DSI *dsi, unsigned char cmd, unsigned char variant,
                             uint16_t bad_ref)
{
    int ofs = 0;
    uint16_t bitmap;
    uint32_t temp;
    uint32_t zero = 0;
    uint64_t zero64 = 0;
    uint64_t allones64 = UINT64_MAX;
    unsigned char creator[] = "ttxt";
    char catpos[16] = { 0 };
    memset(dsi->commands, 0, DSI_CMDSIZ);
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_CMD;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
    dsi->commands[ofs++] = cmd;
    dsi->commands[ofs++] = 0;

    switch (cmd) {
    case AFP_CLOSEVOL:
    case AFP_FLUSH:
    case AFP_OPENDT:
    case AFP_CLOSEDT:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        break;

    case AFP_CLOSEDIR:
    case AFP_SYNCDIR:
    case AFP_DELETEID:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        break;

    case AFP_GETVOLPARAM:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        bitmap = htons(1 << VOLPBIT_VID);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        break;

    case AFP_SETVOLPARAM:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        bitmap = htons(1 << VOLPBIT_BDATE);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_u32(dsi->commands, ofs, zero);
        break;

    case AFP_CREATEDIR:
    case AFP_DELETE:
    case AFP_OPENDIR:
    case AFP_CREATEID:
    case AFP_CREATEFILE:
    case AFP_RMVCMT:
    case AFP_GETCMT:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        ofs = t35_add_name(ofs, "t35");
        break;

    case AFP_RESOLVEID:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = htons(1 << FILPBIT_LNAME);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        break;

    case AFP_ENUMERATE:
    case AFP_ENUMERATE_EXT:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = htons(1 << FILPBIT_LNAME);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        bitmap = htons(1 << DIRPBIT_LNAME);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        bitmap = htons(1);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        bitmap = htons(1024);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_name(ofs, "");
        break;

    case AFP_ENUMERATE_EXT2:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = htons(1 << FILPBIT_LNAME);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        bitmap = htons(1 << DIRPBIT_LNAME);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        bitmap = htons(1);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        temp = htonl(1);
        ofs = t35_add_u32(dsi->commands, ofs, temp);
        temp = htonl(1024);
        ofs = t35_add_u32(dsi->commands, ofs, temp);
        ofs = t35_add_name(ofs, "");
        break;

    case AFP_OPENFORK:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = 0;
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        bitmap = htons(OPENACC_RD);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_name(ofs, "t35");
        break;

    case AFP_GETFLDRPARAM:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = htons(1 << FILPBIT_LNAME);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        bitmap = htons(1 << DIRPBIT_LNAME);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_name(ofs, "");
        break;

    case AFP_SETDIRPARAM:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = htons(1 << DIRPBIT_ATTR);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_name(ofs, "t35");

        if (ofs & 1) {
            ofs++;
        }

        ofs = t35_add_filedir_parms(dsi->commands, ofs, 1 << DIRPBIT_ATTR, 1);
        break;

    case AFP_SETFILEPARAM:
    case AFP_SETFLDRPARAM:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = htons(1 << FILPBIT_ATTR);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_name(ofs, "t35");

        if (ofs & 1) {
            ofs++;
        }

        ofs = t35_add_filedir_parms(dsi->commands, ofs, 1 << FILPBIT_ATTR, 0);
        break;

    case AFP_COPYFILE:
        ofs = t35_add_u16(dsi->commands, ofs,
                          variant == T35_COPYFILE_DEST ? VolID : bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        ofs = t35_add_u16(dsi->commands, ofs,
                          variant == T35_COPYFILE_DEST ? bad_ref : VolID);
        ofs = t35_add_did(dsi->commands, ofs);
        ofs = t35_add_name(ofs, "t35");
        ofs = t35_add_name(ofs, "");
        ofs = t35_add_name(ofs, "t35-copy");
        break;

    case AFP_MOVE:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        ofs = t35_add_did(dsi->commands, ofs);
        ofs = t35_add_name(ofs, "t35");
        ofs = t35_add_name(ofs, "");
        ofs = t35_add_name(ofs, "t35-moved");
        break;

    case AFP_RENAME:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        ofs = t35_add_name(ofs, "t35");
        ofs = t35_add_name(ofs, "t35-renamed");
        break;

    case AFP_EXCHANGEFILE:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        ofs = t35_add_did(dsi->commands, ofs);
        ofs = t35_add_name(ofs, "t35-a");
        ofs = t35_add_name(ofs, "t35-b");
        break;

    case AFP_CATSEARCH:
    case AFP_CATSEARCH_EXT:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        temp = htonl(1);
        ofs = t35_add_u32(dsi->commands, ofs, temp);
        ofs = t35_add_u32(dsi->commands, ofs, zero);
        memcpy(dsi->commands + ofs, catpos, sizeof(catpos));
        ofs += sizeof(catpos);
        bitmap = htons(1 << FILPBIT_LNAME);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        bitmap = htons(1 << DIRPBIT_LNAME);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_u32(dsi->commands, ofs, zero);
        ofs += 2;
        ofs += 2;
        break;

    case AFP_GETEXTATTR:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = 0;
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_u64(dsi->commands, ofs, zero64);
        ofs = t35_add_u64(dsi->commands, ofs, allones64);
        temp = htonl(1024);
        ofs = t35_add_u32(dsi->commands, ofs, temp);
        ofs = t35_add_name(ofs, "t35");

        if (ofs & 1) {
            ofs++;
        }

        ofs = t35_add_extattr_name(dsi->commands, ofs, "user.t35");
        break;

    case AFP_SETEXTATTR:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = 0;
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_u64(dsi->commands, ofs, zero64);
        ofs = t35_add_name(ofs, "t35");

        if (ofs & 1) {
            ofs++;
        }

        ofs = t35_add_extattr_name(dsi->commands, ofs, "user.t35");

        if (ofs < 0) {
            return -1;
        }

        temp = htonl(1);
        ofs = t35_add_u32(dsi->commands, ofs, temp);
        dsi->commands[ofs++] = 'x';
        break;

    case AFP_REMOVEATTR:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = 0;
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_name(ofs, "t35");

        if (ofs & 1) {
            ofs++;
        }

        ofs = t35_add_extattr_name(dsi->commands, ofs, "user.t35");
        break;

    case AFP_LISTEXTATTR:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = 0;
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        bitmap = 0;
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_u32(dsi->commands, ofs, zero);
        temp = htonl(1024);
        ofs = t35_add_u32(dsi->commands, ofs, temp);
        ofs = t35_add_name(ofs, "t35");
        break;
#ifdef HAVE_ACLS

    case AFP_GETACL:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = htons(kFileSec_UUID);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_u32(dsi->commands, ofs, zero);
        ofs = t35_add_name(ofs, "t35");
        break;

    case AFP_SETACL:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = 0;
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        ofs = t35_add_name(ofs, "t35");
        break;

    case AFP_ACCESS:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        bitmap = 0;
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        memset(dsi->commands + ofs, 0, 16);
        ofs += 16;
        ofs = t35_add_u32(dsi->commands, ofs, zero);
        ofs = t35_add_name(ofs, "t35");
        break;
#endif

    case AFP_GETICON:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        memcpy(dsi->commands + ofs, creator, 4);
        ofs += 4;
        memcpy(dsi->commands + ofs, "TEXT", 4);
        ofs += 4;
        dsi->commands[ofs++] = 1;
        dsi->commands[ofs++] = 0;
        bitmap = 0;
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        break;

    case AFP_GTICNINFO:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        memcpy(dsi->commands + ofs, creator, 4);
        ofs += 4;
        bitmap = 0;
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        break;

    case AFP_ADDAPPL:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        memcpy(dsi->commands + ofs, creator, 4);
        ofs += 4;
        ofs = t35_add_u32(dsi->commands, ofs, zero);
        ofs = t35_add_name(ofs, "t35");
        break;

    case AFP_RMVAPPL:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        memcpy(dsi->commands + ofs, creator, 4);
        ofs += 4;
        ofs = t35_add_name(ofs, "t35");
        break;

    case AFP_GETAPPL:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        memcpy(dsi->commands + ofs, creator, 4);
        ofs += 4;
        bitmap = 0;
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        bitmap = htons(1 << FILPBIT_LNAME);
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        break;

    case AFP_ADDCMT:
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        ofs = t35_add_did(dsi->commands, ofs);
        ofs = t35_add_name(ofs, "t35");

        if (ofs & 1) {
            ofs++;
        }

        dsi->commands[ofs++] = 0;
        break;

    case AFP_ADDICON:
        dsi->header.dsi_command = DSIFUNC_WRITE;
        ofs = t35_add_u16(dsi->commands, ofs, bad_ref);
        memcpy(dsi->commands + ofs, creator, 4);
        ofs += 4;
        memcpy(dsi->commands + ofs, "TEXT", 4);
        ofs += 4;
        dsi->commands[ofs++] = 1;
        dsi->commands[ofs++] = 0;
        ofs = t35_add_u32(dsi->commands, ofs, zero);
        bitmap = 0;
        ofs = t35_add_u16(dsi->commands, ofs, bitmap);
        dsi->header.dsi_code = htonl(ofs);
        break;

    default:
        return -1;
    }

    if (ofs < 0) {
        return -1;
    }

    dsi->datalen = ofs;
    dsi->header.dsi_len = htonl(dsi->datalen);

    if (cmd != AFP_ADDICON) {
        dsi->header.dsi_code = 0;
    }

    return ofs;
}

STATIC void test35()
{
    DSI *dsi;
    unsigned int ret;
    unsigned int expected;
    unsigned char cmd;
    uint16_t bad_ref = 0xffff;
    int vol_attrs;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (Mac) {
        test_skipped(T_MAC);
        goto test_exit;
    }

    vol_attrs = get_vol_attrib(VolID);

    for (unsigned int i = 0 ;
            i < sizeof(afp_cmd_with_vol) / sizeof(afp_cmd_with_vol[0]);
            i++) {
        if (!t35_cmd_supported(&afp_cmd_with_vol[i], vol_attrs)) {
            continue;
        }

        cmd = afp_cmd_with_vol[i].cmd;
        expected = ntohl(afp_cmd_with_vol[i].expected);
        FAIL(t35_build_request(dsi, cmd, afp_cmd_with_vol[i].variant, bad_ref) < 0)
        my_dsi_stream_send(dsi, dsi->commands, dsi->datalen);
        my_dsi_cmd_receive(dsi);
        ret = dsi->header.dsi_code;

        if (expected != ret) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED command %3i %s%s\t result %d %s expected %d %s\n",
                        cmd, AfpNum2name(cmd),
                        t35_variant_name(afp_cmd_with_vol[i].variant), ntohl(ret), afp_error(ret),
                        ntohl(expected), afp_error(expected));
            }

            test_failed();
        }
    }

    for (unsigned int i = 0 ;
            i < sizeof(afp_cmd_with_dt) / sizeof(afp_cmd_with_dt[0]);
            i++) {
        if (!t35_cmd_supported(&afp_cmd_with_dt[i], vol_attrs)) {
            continue;
        }

        cmd = afp_cmd_with_dt[i].cmd;
        expected = ntohl(afp_cmd_with_dt[i].expected);
        FAIL(t35_build_request(dsi, cmd, afp_cmd_with_dt[i].variant, bad_ref) < 0)
        my_dsi_stream_send(dsi, dsi->commands, dsi->datalen);
        my_dsi_cmd_receive(dsi);
        ret = dsi->header.dsi_code;

        if (expected != ret) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED command %3i %s\t result %d %s expected %d %s\n", cmd,
                        AfpNum2name(cmd), ntohl(ret), afp_error(ret),
                        ntohl(expected), afp_error(expected));
            }

            test_failed();
        }
    }

test_exit:
    exit_test("Error:test35: illegal volume/desktop ref");
}

/* ----------------------- */
static unsigned char afp_cmd_with_vol_did[] = {
    AFP_COPYFILE, 			/* 5 */
    AFP_CREATEDIR,			/* 6 */
    AFP_CREATEFILE,			/* 7 */
    AFP_DELETE,				/* 8 */
    AFP_OPENDIR,			/* 25 */
    AFP_OPENFORK,           /* 26 */
    AFP_RENAME,				/* 28 */
    AFP_SETDIRPARAM,		/* 29 */
    AFP_SETFILEPARAM, 		/* 30 */
    AFP_GETFLDRPARAM,		/* 34 */
    AFP_SETFLDRPARAM,		/* 35 */
    AFP_ADDAPPL,			/* 53 */
    AFP_RMVAPPL,			/* 54 */
    AFP_ADDCMT,				/* 56 */
    AFP_RMVCMT,				/* 57 */
    AFP_GETCMT,				/* 58 */
};

STATIC void test36()
{
    unsigned int i;
    int ofs;
    uint16_t param = VolID;
    char *name = "t36 dir";
    int  did;
    uint16_t vol = VolID;
    DSI *dsi;
    unsigned char cmd;
    unsigned int ret;
    dsi = &Conn->dsi;
    ENTER_TEST
    memset(dsi->commands, 0, sizeof(dsi->commands));
    did  = FPCreateDir(Conn, vol, DIRDID_ROOT, name);

    if (!did) {
        test_nottested();
        goto test_exit;
    }

    if (FPDelete(Conn, vol, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    for (i = 0 ; i < sizeof(afp_cmd_with_vol_did); i++) {
        memset(dsi->commands, 0, DSI_CMDSIZ);
        dsi->header.dsi_flags = DSIFL_REQUEST;
        dsi->header.dsi_command = DSIFUNC_CMD;
        dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
        ofs = 0;
        cmd = afp_cmd_with_vol_did[i];
        dsi->commands[ofs++] = cmd;
        dsi->commands[ofs++] = 0;
        memcpy(dsi->commands + ofs, &param, sizeof(param));
        ofs += sizeof(param);
        /* directory did */
        memcpy(dsi->commands + ofs, &did, sizeof(did));
        ofs += sizeof(did);
        dsi->datalen = ofs;
        dsi->header.dsi_len = htonl(dsi->datalen);
        dsi->header.dsi_code = 0;
        my_dsi_stream_send(dsi, dsi->commands, dsi->datalen);
        my_dsi_cmd_receive(dsi);
        ret = dsi->header.dsi_code;

        if (ntohl(AFPERR_NOOBJ) != ret) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED command %3i %s\t result %d %s\n", cmd,
                        AfpNum2name(cmd), ntohl(ret), afp_error(ret));
            }

            test_failed();
        }
    }

test_exit:
    exit_test("Error:test36: no folder error (ERR_NOOBJ)");
}


/* -------------------------------------------- */
static unsigned char afp_cmd_with_vol_did1[] = {
    AFP_MOVE,				/* 23 */
    AFP_CREATEID,			/* 39 */
    AFP_EXCHANGEFILE,		/* 42 */
};

STATIC void test37()
{
    int ofs;
    uint16_t param = VolID;
    char *name = "t37 dir";
    int  dir;
    int  did;
    uint16_t vol = VolID;
    DSI *dsi;
    unsigned int ret;
    unsigned char cmd;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (Mac) {
        test_skipped(T_MAC);
        goto test_exit;
    }

    dir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name);

    if (!dir) {
        test_nottested();
        goto fin;
    }

    did  = dir + 1;

    for (unsigned int i = 0 ; i < sizeof(afp_cmd_with_vol_did1); i++) {
        memset(dsi->commands, 0, DSI_CMDSIZ);
        dsi->header.dsi_flags = DSIFL_REQUEST;
        dsi->header.dsi_command = DSIFUNC_CMD;
        dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
        ofs = 0;
        cmd = afp_cmd_with_vol_did1[i];
        dsi->commands[ofs++] = cmd;
        dsi->commands[ofs++] = 0;
        memcpy(dsi->commands + ofs, &param, sizeof(param));
        ofs += sizeof(param);
        memcpy(dsi->commands + ofs, &did, sizeof(did)); /* directory did */
        ofs += sizeof(did);
        dsi->datalen = ofs;
        dsi->header.dsi_len = htonl(dsi->datalen);
        dsi->header.dsi_code = 0;
        my_dsi_stream_send(dsi, dsi->commands, dsi->datalen);
        my_dsi_cmd_receive(dsi);
        ret = dsi->header.dsi_code;

        if (ntohl(AFPERR_NOOBJ) != ret) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED command %3i %s\t result %d %s\n", cmd,
                        AfpNum2name(cmd), ntohl(ret), afp_error(ret));
            }

            test_failed();
        }
    }

fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("Error:test37: no folder error ==> ERR_NOOBJ");
}

/* ----------------------
afp_openfork
afp_createfile
afp_setfilparams
afp_copyfile
afp_createid
afp_exchangefiles
dirlookup
afp_setdirparams
afp_createdir
afp_opendir
afp_getdiracl
afp_setdiracl
afp_openvol
afp_getfildirparams
afp_setfildirparams
afp_delete
afp_moveandrename
afp_enumerate
*/

static void cname_test(char *name)
{
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap = (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) |
                      (1 << DIRPBIT_DID) |
                      (1 << DIRPBIT_ACCESS);
    uint16_t vol = VolID;
    const DSI *dsi;
    dsi = &Conn->dsi;
    FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap))
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (filedir.pdid != 2) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", filedir.pdid, 2);
        }

        test_failed();
    }

    if (strcmp(filedir.lname, name)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %s should be %s\n", filedir.lname, name);
        }

        test_failed();
    }

    /* FIXME: FPEnumerate* uses my_dsi_data_receive. See afphelper.c:delete_directory_tree() */
    FAIL(FPEnumerate(Conn, vol, DIRDID_ROOT, "", 0, bitmap))

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
        test_failed();
        return;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (filedir.pdid != 2) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", filedir.pdid, 2);
        }

        test_failed();
    }

    if (strcmp(filedir.lname, name)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %s should be %s\n", filedir.lname, name);
        }

        test_failed();
    }

    FAIL(ntohl(AFPERR_NOITEM) != FPGetComment(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPCloseVol(Conn, vol))
    vol  = FPOpenVol(Conn, Vol);

    if (vol == 0xffff) {
        test_nottested();
        return;
    }

    if (ntohl(AFPERR_NOITEM) != FPGetComment(Conn, vol, DIRDID_ROOT, name)) {
        test_failed();
    }

    FAIL(FPEnumerate(Conn, vol, DIRDID_ROOT, "", 0, bitmap))

    if (ntohl(AFPERR_NOITEM) != FPGetComment(Conn, vol, DIRDID_ROOT, name)) {
        test_failed();
    }
}

/* ------------------------- */
STATIC void test95()
{
    int dir;
    char *name  = "t95 exchange file";
    char *name1 = "t95 new file name";
    char *name2 = "t95 dir";
    char *pname = "t95 --";
    int pdir;
    int ret;
    uint16_t vol = VolID;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (!(pdir = no_access_folder(vol, DIRDID_ROOT, pname))) {
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name2))) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name2))
    /* sdid bad */
    FAIL(ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, dir, DIRDID_ROOT, name,
            name1))
    /* name bad */
    ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1);

    if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_NOID)) {
        test_failed();
    }

    /* a dir */
    ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, "", name1);

    if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_BADTYPE)) {
        test_failed();
    }

    /* cname unchdirable */
    ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, "t95 --/2.txt",
                         name1);

    if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_ACCESS)) {
        test_failed();
    }

    /* FIXME second time once bar is in the cache */
    FAIL(ntohl(AFPERR_ACCESS) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol,
                                            DIRDID_ROOT, "t95 --/2.txt", "", name1))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
#if 0

    if (!(dir1 = FPCreateDir(Conn, vol, DIRDID_ROOT, name2))) {
        fprintf(stdout, "\tFAILED\n");
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (fork) {
        if (ntohl(AFPERR_DENYCONF) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol,
                DIRDID_ROOT, name, "", name1)) {
            fprintf(stdout, "\tFAILED\n");
        }

        FPCloseFork(Conn, fork);
    }

#endif
    /* sdid bad */
    FAIL(ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name,
            name1))
    /* name bad */
    ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1);

    if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_NOID)) {
        test_failed();
    }

    /* same object */
    FAIL(ntohl(AFPERR_SAMEOBJ) != FPExchangeFile(Conn, vol, DIRDID_ROOT,
            DIRDID_ROOT, name, name))
    /* a dir */
    FAIL(ntohl(AFPERR_BADTYPE) != FPExchangeFile(Conn, vol, DIRDID_ROOT,
            DIRDID_ROOT, name, ""))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1))
    FAIL(FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1))
    delete_folder(vol, DIRDID_ROOT, pname);
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
test_exit:
    exit_test("Error:test95: exchange files");
}

/* ----------------- */
STATIC void test99()
{
    int  dir = 0;
    char *name = "t99 dir no access";
    uint16_t vol = VolID;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (!(dir = no_access_folder(vol, DIRDID_ROOT, name))) {
        goto test_exit;
    }

    cname_test(name);
#if 0
    cname_test("essai permission");
#endif
    delete_folder(vol, DIRDID_ROOT, name);
test_exit:
    exit_test("Error:test99: test folder without access right");
}

/* --------------------- */
STATIC void test100()
{
    int dir;
    char *name = "t100 no obj error";
    char *name1 = "t100 no obj error/none";
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap = (1 << DIRPBIT_ATTR);
    unsigned int ret;
    uint16_t vol = VolID;
    DSI *dsi;
    int dt;
    dsi = &Conn->dsi;
    ENTER_TEST
    dt = FPOpenDT(Conn, vol);
    FAIL(ntohl(AFPERR_NOOBJ) != FPAddComment(Conn, vol, DIRDID_ROOT, name1,
            "essai"))
    FAIL(ntohl(AFPERR_NOOBJ) != FPGetComment(Conn, vol, DIRDID_ROOT, name1))
    FAIL(ntohl(AFPERR_NOOBJ) != FPRemoveComment(Conn, vol, DIRDID_ROOT, name1))
    FAIL(FPCloseDT(Conn, dt))
    filedir.isdir = 1;
    filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
    FAIL(ntohl(AFPERR_NOOBJ) != FPSetDirParms(Conn, vol, DIRDID_ROOT, name1, bitmap,
            &filedir))

    if ((dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name1))) {
        test_failed();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
    } else if (ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
        test_failed();
    }

    dir = FPOpenDir(Conn, vol, DIRDID_ROOT, name1);

    if (dir || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
        test_failed();
    }

    /* FIXME: FPEnumerate* uses my_dsi_data_receive. See afphelper.c:delete_directory_tree() */
    ret = FPEnumerate(Conn, vol, DIRDID_ROOT, name1,
                      (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                      (1 << FILPBIT_FINFO) |
                      (1 << FILPBIT_CDATE) | (1 << FILPBIT_PDID)
                      ,
                      (1 << DIRPBIT_ATTR) |
                      (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                      (1 << DIRPBIT_ACCESS)
                     );

    if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NODIR, AFPERR_NODIR)) {
        test_failed();
    }

    if (ntohl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT,
                                          name1, "", name)) {
        test_failed();
    }

    /* FIXME afp_errno in file.c */
    if (get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) {
        ret = FPCreateID(Conn, vol, DIRDID_ROOT, name1);

        if (ret != ntohl(AFPERR_NOOBJ)) {
            test_failed();
        }
    }

    /* FIXME ? */
    if (ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT,
            name1, name)) {
        test_failed();
    }

    FAIL(ntohl(AFPERR_NOOBJ) != FPSetFileParams(Conn, vol, DIRDID_ROOT, name1,
            bitmap, &filedir))
    FAIL(ntohl(AFPERR_NOOBJ) != FPSetFilDirParam(Conn, vol, DIRDID_ROOT, name1,
            bitmap, &filedir))
    FAIL(ntohl(AFPERR_NOOBJ) != FPRename(Conn, vol, DIRDID_ROOT, name1, name))
    FAIL(ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol, DIRDID_ROOT, name1))
    FAIL(ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT,
            name1, name))
    exit_test("Error:test100: no obj cname error (AFPERR_NOOBJ)");
}

/* --------------------- */
STATIC void test101()
{
    int dir;
    char *name = "t101 no obj error";
    char *ndir = "t101 no";
    char *name1 = "t101 no/none";
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap = (1 << DIRPBIT_ATTR);
    uint16_t vol = VolID;
    unsigned int ret;
    DSI *dsi;
    int  dt;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (!(dir = no_access_folder(vol, DIRDID_ROOT, ndir))) {
        goto test_exit;
    }

    dt = FPOpenDT(Conn, vol);
    FAIL(ntohl(AFPERR_ACCESS) != FPAddComment(Conn, vol, DIRDID_ROOT, name1,
            "essai"))
    ret = FPGetComment(Conn, vol, DIRDID_ROOT, name1);

    if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_ACCESS)) {
        test_failed();
    }

    FAIL(ntohl(AFPERR_ACCESS) != FPRemoveComment(Conn, vol, DIRDID_ROOT, name1))
    FAIL(FPCloseDT(Conn, dt))
    filedir.isdir = 1;
    filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
    FAIL(ntohl(AFPERR_ACCESS) != FPSetDirParms(Conn, vol, DIRDID_ROOT, name1,
            bitmap, &filedir))

    if ((dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name1))) {
        test_failed();
        FPDelete(Conn, vol, DIRDID_ROOT, name1);
    } else if (ntohl(AFPERR_ACCESS) != dsi->header.dsi_code) {
        test_failed();
    }

    dir = FPOpenDir(Conn, vol, DIRDID_ROOT, name1);

    if (dir || ntohl(AFPERR_ACCESS) != dsi->header.dsi_code) {
        test_failed();
    }

    /* FIXME: FPEnumerate* uses my_dsi_data_receive. See afphelper.c:delete_directory_tree() */
    ret = FPEnumerate(Conn, vol, DIRDID_ROOT, name1,
                      (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                      (1 << FILPBIT_FINFO) |
                      (1 << FILPBIT_CDATE) | (1 << FILPBIT_PDID)
                      ,
                      (1 << DIRPBIT_ATTR) |
                      (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                      (1 << DIRPBIT_ACCESS)
                     );

    if (not_valid(ret, /* MAC */AFPERR_NODIR, AFPERR_ACCESS)) {
        test_failed();
    }

    FAIL(ntohl(AFPERR_ACCESS) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol,
                                            DIRDID_ROOT, name1, "", name))

    if (get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) {
        ret = FPCreateID(Conn, vol, DIRDID_ROOT, name1);

        if (ret != ntohl(AFPERR_ACCESS)) {
            test_failed();
        }
    }

    ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name1, name);

    if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_ACCESS)) {
        test_failed();
    }

    FAIL(ntohl(AFPERR_ACCESS) != FPSetFileParams(Conn, vol, DIRDID_ROOT, name1,
            bitmap, &filedir))
    FAIL(ntohl(AFPERR_ACCESS) != FPRename(Conn, vol, DIRDID_ROOT, name1, name))
    FAIL(ntohl(AFPERR_ACCESS) != FPDelete(Conn, vol, DIRDID_ROOT, name1))
    FAIL(ntohl(AFPERR_ACCESS) != FPMoveAndRename(Conn, vol, DIRDID_ROOT,
            DIRDID_ROOT, name1, name))
    delete_folder(vol, DIRDID_ROOT, ndir);
test_exit:
    exit_test("Error:test101: access error cname");
}

/* --------------------- */
static void test_comment(uint16_t vol, int dir, char *name)
{
    int ret;
    ret = FPAddComment(Conn, vol, dir, name, "essai");

    if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
        test_failed();
    }

    ret = FPRemoveComment(Conn, vol, dir, name);

    if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
        test_failed();
    }

    if (!ret) {
        FPAddComment(Conn, vol, dir, name, "essai");
    }
}

/* -------------- */
STATIC void test102()
{
    int dir;
    char *name = "t102 access error";
    char *name1 = "t102 dir --";
    int ret;
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap = (1 << DIRPBIT_ATTR);
    uint16_t vol = VolID;
    DSI *dsi;
    int  dt;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (!(dir = no_access_folder(vol, DIRDID_ROOT, name1))) {
        goto test_exit;
    }

    dt = FPOpenDT(Conn, vol);
    test_comment(vol, DIRDID_ROOT, name1);
    ret = FPGetComment(Conn, vol, DIRDID_ROOT, name1);

    if (not_valid(ret, /* MAC */0, AFPERR_NOITEM)) {
        test_failed();
    }

    FAIL(FPCloseDT(Conn, dt))
    filedir.isdir = 1;
    filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
    FAIL(ntohl(AFPERR_ACCESS) != FPSetDirParms(Conn, vol, DIRDID_ROOT, name1,
            bitmap, &filedir))

    if ((dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name1))) {
        test_failed();
        FPDelete(Conn, vol, DIRDID_ROOT, name1);
    } else if (ntohl(AFPERR_EXIST) != dsi->header.dsi_code) {
        test_failed();
    }

    dir = FPOpenDir(Conn, vol, DIRDID_ROOT, name1);

    if (dir) {
        ret = 0;
    } else {
        ret = dsi->header.dsi_code;
    }

    if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
        test_failed();
    }

    /* FIXME: FPEnumerate* uses my_dsi_data_receive. See afphelper.c:delete_directory_tree() */

    if (ntohl(AFPERR_ACCESS) != FPEnumerate(Conn, vol, DIRDID_ROOT, name1,
                                            (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                                            (1 << FILPBIT_FINFO) |
                                            (1 << FILPBIT_CDATE) | (1 << FILPBIT_PDID)
                                            ,
                                            (1 << DIRPBIT_ATTR) |
                                            (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                                            (1 << DIRPBIT_ACCESS)
                                           )) {
        test_failed();
    }

    FAIL(ntohl(AFPERR_BADTYPE) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol,
            DIRDID_ROOT, name1, "", name))

    if (get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) {
        FAIL(ntohl(AFPERR_BADTYPE) != FPCreateID(Conn, vol, DIRDID_ROOT, name1))
    }

    ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name1, name);

    if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_BADTYPE)) {
        test_failed();
    }

    FAIL(ntohl(AFPERR_BADTYPE) != FPSetFileParams(Conn, vol, DIRDID_ROOT, name1,
            bitmap, &filedir))

    if (FPRename(Conn, vol, DIRDID_ROOT, name1, name)) {
#if 0
        /* FIXME: This operation is allowed on Linux, but rejected on macOS with AFPERR_ACCESS */
        test_failed();
#endif
    } else {
        FPRename(Conn, vol, DIRDID_ROOT, name, name1);
    }

    if (FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name1, name)) {
#if 0
        /* FIXME: This operation is allowed on Linux, but rejected on macOS with AFPERR_ACCESS */
        test_failed();
#endif
    } else {
        FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1);
    }

    ret = FPDelete(Conn, vol, DIRDID_ROOT, name1);

    if (not_valid(ret, 0, AFPERR_ACCESS)) {
        test_failed();
    }

    if (ret) {
        delete_folder(vol, DIRDID_ROOT, name1);
    }

test_exit:
    exit_test("Error:test102: access error but not cname");
}

/* --------------------- */
STATIC void test103()
{
    int dir;
    char *name = "t103 did access error";
    char *name1 = "t130 dir --";
    int ret;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap = (1 << DIRPBIT_DID);
    uint16_t vol = VolID;
    const DSI *dsi;
    int  dt;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (!(dir = no_access_folder(vol, DIRDID_ROOT, name1))) {
        goto test_exit;
    }

    FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name1, 0, bitmap))
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    dir = filedir.did;
    dt = FPOpenDT(Conn, vol);

    if (!Quiet) {
        fprintf(stdout, "\t>>> test_comment()\n");
    }

    test_comment(vol, dir, "");

    if (!Quiet) {
        fprintf(stdout, "\t>>> FPGetComment()\n");
    }

    ret = FPGetComment(Conn, vol, dir, "");

    if (not_valid(ret, /* MAC */AFPERR_ACCESS, AFPERR_NOITEM)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED at FPGetComment: got %d (%s), expected AFPERR_ACCESS or AFPERR_NOITEM\n",
                    ntohl(ret), afp_error(ret));
        }

        test_failed();
    } else if (!Quiet) {
        fprintf(stdout, "\t>>> FPGetComment PASSED: got %d (%s)\n", ntohl(ret),
                afp_error(ret));
    }

    FAIL(FPCloseDT(Conn, dt))
    filedir.isdir = 1;
    filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
    bitmap = (1 << DIRPBIT_ATTR);
    ret = FPSetDirParms(Conn, vol, dir, "", bitmap, &filedir);

    if (ntohl(AFPERR_ACCESS) != ret) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED at FPSetDirParms: got %d (%s), expected AFPERR_ACCESS\n",
                    ntohl(ret), afp_error(ret));
        }

        test_failed();
    }

    /* FIXME: FPEnumerate* uses my_dsi_data_receive. See afphelper.c:delete_directory_tree() */
    ret = FPEnumerate(Conn, vol, dir, "",
                      (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                      (1 << FILPBIT_FINFO) |
                      (1 << FILPBIT_CDATE) | (1 << FILPBIT_PDID),
                      (1 << DIRPBIT_ATTR) |
                      (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                      (1 << DIRPBIT_ACCESS));

    if (ntohl(AFPERR_ACCESS) != ret) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED at FPEnumerate: got %d (%s), expected AFPERR_ACCESS\n",
                    ntohl(ret), afp_error(ret));
        }

        test_failed();
    }

    if (!Quiet) {
        fprintf(stdout, "\t>>> FPCopyFile()\n");
    }

    ret = FPCopyFile(Conn, vol, dir, vol, DIRDID_ROOT, "", "", name);

    if (not_valid(ret, AFPERR_PARAM, AFPERR_BADTYPE)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED at FPCopyFile: got %d (%s), expected AFPERR_PARAM or AFPERR_BADTYPE\n",
                    ntohl(ret), afp_error(ret));
        }

        test_failed();
    } else if (!Quiet) {
        fprintf(stdout, "\t>>> FPCopyFile PASSED\n");
    }

    if (get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) {
        FAIL(ntohl(AFPERR_BADTYPE) != FPCreateID(Conn, vol, dir, ""))
    }

    if (!Quiet) {
        fprintf(stdout, "\t>>> FPExchangeFile()\n");
    }

    ret = FPExchangeFile(Conn, vol, dir, DIRDID_ROOT, "", name);

    if (not_valid(ret, AFPERR_NOOBJ, AFPERR_BADTYPE)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED at FPExchangeFile: got %d (%s), expected AFPERR_NOOBJ or AFPERR_BADTYPE\n",
                    ntohl(ret), afp_error(ret));
        }

        test_failed();
    } else if (!Quiet) {
        fprintf(stdout, "\t>>> FPExchangeFile PASSED\n");
    }

    ret = FPSetFileParams(Conn, vol, dir, "", bitmap, &filedir);

    if (not_valid(ret, AFPERR_PARAM, AFPERR_BADTYPE)) {
        test_failed();
    }

    if (FPRename(Conn, vol, dir, "", name)) {
#if 0
        /* FIXME: This operation is allowed on Linux, but rejected on macOS with AFPERR_ACCESS */
        test_failed();
#endif
    } else {
        if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
            test_failed();
        }

        FPRename(Conn, vol, DIRDID_ROOT, name, name1);
    }

    if (FPMoveAndRename(Conn, vol, dir, DIRDID_ROOT, "", name)) {
#if 0
        /* FIXME: This operation is allowed on Linux, but rejected on macOS with AFPERR_ACCESS */
        test_failed();
#endif
    } else {
        if (!Quiet) {
            fprintf(stdout, "\t>>> FPMoveAndRename succeeded\n");

            /* Check filesystem state */
            if (Path[0] != '\0') {
                char oldpath[1024], newpath[1024];
                snprintf(oldpath, sizeof(oldpath), "%s/%s", Path, name1);
                snprintf(newpath, sizeof(newpath), "%s/%s", Path, name);
                fprintf(stdout, "\t>>> OLD name \"%s\": %s\n",
                        oldpath, access(oldpath, F_OK) == 0 ? "EXISTS" : "NOT FOUND");
                fprintf(stdout, "\t>>> NEW name \"%s\": %s\n",
                        newpath, access(newpath, F_OK) == 0 ? "EXISTS" : "NOT FOUND");
            }
        }

        ret = FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap);

        if (ret) {
            if (!Quiet) {
                fprintf(stdout,
                        "\tFAILED at FPGetFileDirParams after MoveAndRename: got %d (%s)\n",
                        ntohl(ret), afp_error(ret));
            }

            test_failed();
        }

        FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1);
    }

    ret = FPDelete(Conn, vol, dir, "");

    if (not_valid(ret, 0, AFPERR_ACCESS)) {
        test_failed();
    }

    if (ret) {
        delete_folder(vol, DIRDID_ROOT, name1);
    }

test_exit:
    exit_test("Error:test103: did access error");
}

/* --------------------- */
STATIC void test105()
{
    int dir;
    unsigned int err;
    char *name = "t105 bad did";
    char *name1 = "t105 no obj error/none";
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap = (1 << DIRPBIT_ATTR);
    uint16_t vol = VolID;
    DSI *dsi;
    int ret;
    int  dt;
    dsi = &Conn->dsi;
    ENTER_TEST
    dir = 0;
    err = ntohl(AFPERR_PARAM);
    dt = FPOpenDT(Conn, vol);
    FAIL(err != FPAddComment(Conn, vol, dir, name1, "essai"))
    FAIL(err != FPGetComment(Conn, vol, dir, name1))
    FAIL(err != FPRemoveComment(Conn, vol, dir, name1))
    FAIL(FPCloseDT(Conn, dt))
    filedir.isdir = 1;
    filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
    FAIL(err != FPSetDirParms(Conn, vol, dir, name1, bitmap, &filedir))

    if ((dir = FPCreateDir(Conn, vol, dir, name1))) {
        test_failed();
        FPDelete(Conn, vol, dir, name1);
    } else if (err != dsi->header.dsi_code) {
        test_failed();
    }

    dir = FPOpenDir(Conn, vol, dir, name1);

    if (dir || err != dsi->header.dsi_code) {
        test_failed();
    }

    /* FIXME: FPEnumerate* uses my_dsi_data_receive. See afphelper.c:delete_directory_tree() */

    if (err  != FPEnumerate(Conn, vol, dir, name1,
                            (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                            (1 << FILPBIT_FINFO) |
                            (1 << FILPBIT_CDATE) | (1 << FILPBIT_PDID)
                            ,
                            (1 << DIRPBIT_ATTR) |
                            (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                            (1 << DIRPBIT_ACCESS)
                           )) {
        test_failed();
    }

    FAIL(err != FPCopyFile(Conn, vol, dir, vol, DIRDID_ROOT, name1, "", name))

    /* FIXME afp_errno in file.c */
    if (get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) {
        if (err != FPCreateID(Conn, vol, dir, name1)) {
            test_failed();
        }
    }

    ret = FPExchangeFile(Conn, vol, dir, DIRDID_ROOT, name1, name);

    if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_PARAM)) {
        test_failed();
    }

    FAIL(err != FPSetFileParams(Conn, vol, dir, name1, bitmap, &filedir))
    FAIL(err != FPRename(Conn, vol, dir, name1, name))
    FAIL(err != FPDelete(Conn, vol, dir, name1))
    FAIL(err != FPMoveAndRename(Conn, vol, dir, DIRDID_ROOT, name1, name))
    exit_test("Error:test105: bad DID in call");
}

/* -------------------------- */
/* FIXME: afpd crash in dircache_search_by_did() */
STATIC void test170()
{
    uint16_t bitmap = 0;
    char *name = "test170.txt";
    char *name1 = "newtest170.txt";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    int fork;
    int dir = DIRDID_ROOT_PARENT;
    unsigned int ret;
    uint16_t vol = VolID;
    const DSI *dsi;
    int  dt;
    dsi = &Conn->dsi;
    ENTER_TEST
    /* ---- fork.c ---- */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT_PARENT, "",
                      OPENACC_WR | OPENACC_RD);

    if (fork || htonl(AFPERR_PARAM) != dsi->header.dsi_code) {
        test_failed();

        if (fork) {
            FPCloseFork(Conn, fork);
        }

        goto test_exit;
    }

    /* ---- file.c ---- */
    FAIL(htonl(AFPERR_PARAM) != FPCreateFile(Conn, vol, 0, DIRDID_ROOT_PARENT, ""))
    bitmap = (1 << FILPBIT_MDATE);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
        goto test_exit;
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        filedir.isdir = 0;
        FAIL(htonl(AFPERR_PARAM) != FPSetFileParams(Conn, vol, DIRDID_ROOT_PARENT, "",
                bitmap, &filedir))
    }

    /* -------------------- */
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))

    if (htonl(AFPERR_PARAM) != FPCopyFile(Conn, vol, DIRDID_ROOT_PARENT, vol,
                                          DIRDID_ROOT, "", "", name1)) {
        test_failed();
        FPDelete(Conn, vol, DIRDID_ROOT, name1);
        goto test_exit;
    }

    FAIL(htonl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol,
                                           DIRDID_ROOT_PARENT, name, "", ""))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))

    /* -------------------- */
    if (get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) {
        ret = FPCreateID(Conn, vol, DIRDID_ROOT_PARENT, "");
        FAIL(htonl(AFPERR_NOOBJ) != ret && htonl(AFPERR_PARAM) != ret)
    }

    /* -------------------- */
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1))
    ret = FPExchangeFile(Conn, vol, DIRDID_ROOT_PARENT, dir, "", name1);

    if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_BADTYPE, AFPERR_NOOBJ)) {
        test_failed();
        goto fin;
    }

    ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT_PARENT, name, "");

    if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_BADTYPE, AFPERR_NOOBJ)) {
        test_failed();
        goto fin;
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
    /* ---- directory.c ---- */
    filedir.isdir = 1;
    FAIL(ntohl(AFPERR_NOOBJ) != FPSetDirParms(Conn, vol, DIRDID_ROOT_PARENT, "",
            bitmap, &filedir))
    /* ---------------- */
    dir  = FPCreateDir(Conn, vol, DIRDID_ROOT_PARENT, "");

    if (dir || ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
        test_failed();
        goto test_exit;
    }

    /* ---------------- */
    dir = FPOpenDir(Conn, vol, DIRDID_ROOT_PARENT, "");

    if (dir || ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
        test_failed();
        goto test_exit;
    }

    /* ---- filedir.c ---- */

    if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol, DIRDID_ROOT_PARENT, "",
            0,
            (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
            (1 << DIRPBIT_UID) |
            (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS))
       ) {
        test_failed();
        goto test_exit;
    }

    /* ---------------- */
    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
        goto test_exit;
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

        if (ntohl(AFPERR_NOOBJ) != FPSetFilDirParam(Conn, vol, DIRDID_ROOT_PARENT, "",
                bitmap, &filedir)) {
            test_failed();
            goto test_exit;
        }
    }

    /* ---------------- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPRename(Conn, vol, DIRDID_ROOT_PARENT, "", name1))
    /* ---------------- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol, DIRDID_ROOT_PARENT, ""))
    /* ---------------- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT_PARENT,
            DIRDID_ROOT, "", name1))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    FAIL(ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT,
            DIRDID_ROOT_PARENT, name, ""))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))

    /* ---- enumerate.c ---- */
    /* FIXME: FPEnumerate* uses my_dsi_data_receive. See afphelper.c:delete_directory_tree() */
    if (ntohl(AFPERR_NOOBJ) != FPEnumerate(Conn, vol, DIRDID_ROOT_PARENT, "",
                                           (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                                           (1 << FILPBIT_FINFO) |
                                           (1 << FILPBIT_CDATE) | (1 << FILPBIT_PDID)
                                           ,
                                           (1 << DIRPBIT_ATTR) |
                                           (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                                           (1 << DIRPBIT_ACCESS)
                                          )) {
        test_failed();
        goto test_exit;
    }

    /* ---- desktop.c ---- */
    dt = FPOpenDT(Conn, vol);
    FAIL(ntohl(AFPERR_NOOBJ) != FPAddComment(Conn, vol, DIRDID_ROOT_PARENT, "",
            "Comment"))
    FAIL(ntohl(AFPERR_NOOBJ) != FPGetComment(Conn, vol, DIRDID_ROOT_PARENT, ""))
    FAIL(ntohl(AFPERR_NOOBJ) != FPRemoveComment(Conn, vol, DIRDID_ROOT_PARENT, ""))
    FAIL(FPCloseDT(Conn, dt))
fin:
    /* Cleanup - don't fail test if files already deleted during test */
    FPDelete(Conn, vol, DIRDID_ROOT, name);
    FPDelete(Conn, vol, DIRDID_ROOT, name1);
test_exit:
    exit_test("Error:test170: cname error did=1 name=\"\"");
}

/* -------------------------- */
/* Tests FPMoveAndRename into subdirectory - validates cache consistency after rename */
STATIC void test171()
{
    uint16_t bitmap = 0;
    char *tname = "test171";
    char *name = "test171.txt";
    char *name1 = "newtest171.txt";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    int tdir = DIRDID_ROOT_PARENT;
    int fork;
    int dir = DIRDID_ROOT_PARENT;
    unsigned int ret;
    uint16_t vol = VolID;
    const DSI *dsi;
    int  dt;
    dsi = &Conn->dsi;
    ENTER_TEST
    /* ---- fork.c ---- */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, tdir, tname,
                      OPENACC_WR | OPENACC_RD);

    if (fork || htonl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
        test_failed();

        if (fork) {
            FPCloseFork(Conn, fork);
        }

        goto test_exit;
    }

    /* ---- file.c ---- */
    FAIL(htonl(AFPERR_NOOBJ) != FPCreateFile(Conn, vol, 0, tdir, tname))
    bitmap = (1 << FILPBIT_MDATE);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
        goto test_exit;
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        filedir.isdir = 0;
        FAIL(htonl(AFPERR_NOOBJ) != FPSetFileParams(Conn, vol, tdir, tname, bitmap,
                &filedir))
    }

    /* -------------------- */
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))

    if (htonl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, tdir, vol, DIRDID_ROOT, tname,
                                          "", name1)) {
        test_failed();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
        goto fin;
    }

    FAIL(htonl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, tdir, name,
                                           "", tname))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))

    /* -------------------- */
    if (get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) {
        ret = FPCreateID(Conn, vol, tdir, tname);

        if (htonl(AFPERR_NOOBJ) != ret && htonl(AFPERR_PARAM) != ret) {
            test_failed();
            goto fin;
        }
    }

    /* -------------------- */
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1))
    FAIL(ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, tdir, dir, tname, name1))
    FAIL(ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, DIRDID_ROOT, tdir, name,
            tname))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
    /* ---- directory.c ---- */
    filedir.isdir = 1;
    FAIL(ntohl(AFPERR_NOOBJ) != FPSetDirParms(Conn, vol, tdir, tname, bitmap,
            &filedir))
    /* ---------------- */
    dir  = FPCreateDir(Conn, vol, tdir, tname);

    if (dir || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
        test_failed();
    }

    /* ---------------- */
    dir = FPOpenDir(Conn, vol, tdir, tname);
    FAIL(dir || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code)
    /* ---- filedir.c ---- */

    if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol, tdir, tname, 0,
            (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
            (1 << DIRPBIT_UID) |
            (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS))
       ) {
        test_failed();
    }

    /* ---------------- */
    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        FAIL(ntohl(AFPERR_NOOBJ) != FPSetFilDirParam(Conn, vol, tdir, tname, bitmap,
                &filedir))
    }

    /* ---------------- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPRename(Conn, vol, tdir, tname, name1))
    /* ---------------- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol, tdir, tname))
    /* ---------------- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, tdir, DIRDID_ROOT, tname,
            name1))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    FAIL(ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, tdir, name,
            tname))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    /* ---- enumerate.c ---- */
    /* FIXME: FPEnumerate* uses my_dsi_data_receive. See afphelper.c:delete_directory_tree() */
    ret = FPEnumerate(Conn, vol, tdir, tname,
                      (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                      (1 << FILPBIT_FINFO) |
                      (1 << FILPBIT_CDATE) | (1 << FILPBIT_PDID)
                      ,
                      (1 << DIRPBIT_ATTR) |
                      (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                      (1 << DIRPBIT_ACCESS)
                     );

    if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NODIR, AFPERR_NODIR)) {
        test_failed();
    }

    /* ---- desktop.c ---- */
    dt = FPOpenDT(Conn, vol);
    FAIL(ntohl(AFPERR_NOOBJ) != FPAddComment(Conn, vol, tdir, tname, "Comment"))
    FAIL(ntohl(AFPERR_NOOBJ) != FPGetComment(Conn, vol, tdir, tname))
    FAIL(ntohl(AFPERR_NOOBJ) != FPRemoveComment(Conn, vol, tdir, tname))
    FAIL(FPCloseDT(Conn, dt))
fin:
    /* Cleanup - don't fail test if files already deleted during test */
    FPDelete(Conn, vol, DIRDID_ROOT, name);
    FPDelete(Conn, vol, DIRDID_ROOT, name1);
test_exit:
    exit_test("Error:test171: cname error did=1 name=bad name");
}

/* -------------------------- */
/* Tests FPMoveAndRename with tdir = 0 - validates cache consistency with special DID */
STATIC void test173()
{
    uint16_t bitmap = 0;
    char *tname = "test173";
    char *name = "test173.txt";
    char *name1 = "newtest173.txt";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    int tdir = 0;
    int fork;
    int dir = DIRDID_ROOT_PARENT;
    unsigned int ret;
    uint16_t vol = VolID;
    const DSI *dsi;
    int  dt;
    dsi = &Conn->dsi;
    ENTER_TEST
    /* ---- fork.c ---- */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, tdir, tname,
                      OPENACC_WR | OPENACC_RD);

    if (fork || htonl(AFPERR_PARAM) != dsi->header.dsi_code) {
        test_failed();

        if (fork) {
            FPCloseFork(Conn, fork);
        }

        goto test_exit;
    }

    /* ---- file.c ---- */
    FAIL(htonl(AFPERR_PARAM) != FPCreateFile(Conn, vol, 0, tdir, tname))
    bitmap = (1 << FILPBIT_MDATE);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
        goto fin;
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        filedir.isdir = 0;
        FAIL(htonl(AFPERR_PARAM) != FPSetFileParams(Conn, vol, tdir, tname, bitmap,
                &filedir))
    }

    /* -------------------- */
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))

    if (htonl(AFPERR_PARAM) != FPCopyFile(Conn, vol, tdir, vol, DIRDID_ROOT, tname,
                                          "", name1)) {
        test_failed();
        goto fin;
    }

    FAIL(htonl(AFPERR_PARAM) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, tdir, name,
                                           "", tname))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))

    /* -------------------- */
    if (get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) {
        ret = FPCreateID(Conn, vol, tdir, tname);
        FAIL(htonl(AFPERR_PARAM) != ret)
    }

    /* -------------------- */
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1))
    ret = FPExchangeFile(Conn, vol, tdir, dir, tname, name1);

    if (htonl(AFPERR_PARAM) != ret
            && not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_PARAM)) {
        test_failed();
        goto fin;
    }

    FAIL(ntohl(AFPERR_PARAM) != FPExchangeFile(Conn, vol, DIRDID_ROOT, tdir, name,
            tname))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
    /* ---- directory.c ---- */
    filedir.isdir = 1;
    FAIL(ntohl(AFPERR_PARAM) != FPSetDirParms(Conn, vol, tdir, tname, bitmap,
            &filedir))
    /* ---------------- */
    dir  = FPCreateDir(Conn, vol, tdir, tname);

    if (dir || ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
        test_failed();
        goto test_exit;
    }

    /* ---------------- */
    dir = FPOpenDir(Conn, vol, tdir, tname);

    if (dir || ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
        test_failed();
        goto test_exit;
    }

    /* ---- filedir.c ---- */

    if (ntohl(AFPERR_PARAM) != FPGetFileDirParams(Conn, vol, tdir, tname, 0,
            (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
            (1 << DIRPBIT_UID) |
            (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS))
       ) {
        test_failed();
        goto test_exit;
    }

    /* ---------------- */
    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
        goto test_exit;
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        FAIL(ntohl(AFPERR_PARAM) != FPSetFilDirParam(Conn, vol, tdir, tname, bitmap,
                &filedir))
    }

    /* ---------------- */
    FAIL(ntohl(AFPERR_PARAM) != FPRename(Conn, vol, tdir, tname, name1))
    /* ---------------- */
    FAIL(ntohl(AFPERR_PARAM) != FPDelete(Conn, vol, tdir, tname))
    /* ---------------- */
    FAIL(ntohl(AFPERR_PARAM) != FPMoveAndRename(Conn, vol, tdir, DIRDID_ROOT, tname,
            name1))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    FAIL(ntohl(AFPERR_PARAM) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, tdir, name,
            tname))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))

    /* ---- enumerate.c ---- */
    /* FIXME: FPEnumerate* uses my_dsi_data_receive. See afphelper.c:delete_directory_tree() */
    if (ntohl(AFPERR_PARAM) != FPEnumerate(Conn, vol, tdir, tname,
                                           (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                                           (1 << FILPBIT_FINFO) |
                                           (1 << FILPBIT_CDATE) | (1 << FILPBIT_PDID)
                                           ,
                                           (1 << DIRPBIT_ATTR) |
                                           (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                                           (1 << DIRPBIT_ACCESS)
                                          )) {
        test_failed();
        goto test_exit;
    }

    /* ---- desktop.c ---- */
    dt = FPOpenDT(Conn, vol);
    FAIL(ntohl(AFPERR_PARAM) != FPAddComment(Conn, vol, tdir, tname, "Comment"))
    FAIL(ntohl(AFPERR_PARAM) != FPGetComment(Conn, vol, tdir, tname))
    FAIL(ntohl(AFPERR_PARAM) != FPRemoveComment(Conn, vol, tdir, tname))
    FAIL(FPCloseDT(Conn, dt))
    /* ---- appl.c ---- */
fin:
    /* Cleanup - don't fail test if files already deleted during test */
    FPDelete(Conn, vol, DIRDID_ROOT, name);
    FPDelete(Conn, vol, DIRDID_ROOT, name1);
test_exit:
    exit_test("Error:test173: did error did=0 name=test173 name");
}

/* -------------------------- */
STATIC void test174()
{
    uint16_t bitmap = 0;
    char *tname = "test174";
    char *name = "test174.txt";
    char *name1 = "newtest174.txt";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    int tdir;
    int fork;
    int dir = DIRDID_ROOT_PARENT;
    uint16_t vol2;
    unsigned int ret;
    uint16_t vol = VolID;
    const DSI *dsi = &Conn->dsi;
    DSI *dsi2;
    int  dt;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    tdir  = FPCreateDir(Conn, vol, DIRDID_ROOT, tname);

    if (!tdir) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPGetFileDirParams(Conn, vol, tdir, "", 0,
                            (1 << DIRPBIT_PDINFO) | (1 << DIRPBIT_OFFCNT)))
    FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0,
                            (1 << DIRPBIT_PDINFO) | (1 << DIRPBIT_OFFCNT)))
    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_failed();
    }

    if (FPDelete(Conn2, vol2,  DIRDID_ROOT, tname)) {
        test_failed();
        FPDelete(Conn, vol, tdir, "");
        FPCloseVol(Conn2, vol2);
        goto test_exit;
    }

    FPCloseVol(Conn2, vol2);
    /* ---- fork.c ---- */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, tdir, tname,
                      OPENACC_WR | OPENACC_RD);

    if (fork || htonl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
        test_failed();

        if (fork) {
            FPCloseFork(Conn, fork);
        }
    }

    /* ---- file.c ---- */
    FAIL(htonl(AFPERR_NOOBJ) != FPCreateFile(Conn, vol, 0, tdir, tname))
    bitmap = (1 << FILPBIT_MDATE);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        filedir.isdir = 0;
        FAIL(htonl(AFPERR_NOOBJ) != FPSetFileParams(Conn, vol, tdir, tname, bitmap,
                &filedir))
    }

    /* -------------------- */
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))

    if (htonl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, tdir, vol, DIRDID_ROOT, tname,
                                          "", name1)) {
        test_failed();
        FPDelete(Conn, vol, DIRDID_ROOT, name1);
    }

    FAIL(htonl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, tdir, name,
                                           "", tname))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))

    /* -------------------- */
    if (get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) {
        ret = FPCreateID(Conn, vol, tdir, tname);

        if (htonl(AFPERR_NOOBJ) != ret && htonl(AFPERR_PARAM) != ret) {
            test_failed();
        }
    }

    /* -------------------- */
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1))
    ret = FPExchangeFile(Conn, vol, tdir, dir, tname, name1);

    if (ntohl(AFPERR_NOOBJ) != ret) {
        if (ret == htonl(AFPERR_PARAM)) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED (IGNORED) not always the same error code!\n");
            }

            test_skipped(T_NONDETERM);
        } else {
            test_failed();
        }
    }

    FAIL(ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, DIRDID_ROOT, tdir, name,
            tname))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
    /* ---- directory.c ---- */
    filedir.isdir = 1;
    FAIL(ntohl(AFPERR_NOOBJ) != FPSetDirParms(Conn, vol, tdir, tname, bitmap,
            &filedir))
    /* ---------------- */
    dir  = FPCreateDir(Conn, vol, tdir, tname);

    if (dir || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
        test_failed();
    }

    /* ---------------- */
    dir = FPOpenDir(Conn, vol, tdir, tname);

    if (dir || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
        test_failed();
    }

    /* ---- filedir.c ---- */

    if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol, tdir, tname, 0,
            (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
            (1 << DIRPBIT_UID) |
            (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS))
       ) {
        test_failed();
    }

    /* ---------------- */
    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        FAIL(ntohl(AFPERR_NOOBJ) != FPSetFilDirParam(Conn, vol, tdir, tname, bitmap,
                &filedir))
    }

    /* ---------------- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPRename(Conn, vol, tdir, tname, name1))
    /* ---------------- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol, tdir, tname))
    /* ---------------- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, tdir, DIRDID_ROOT, tname,
            name1))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    FAIL(ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, tdir, name,
            tname))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))

    /* ---- enumerate.c ---- */
    /* FIXME: FPEnumerate* uses my_dsi_data_receive. See afphelper.c:delete_directory_tree() */
    if (ntohl(AFPERR_NODIR) != FPEnumerate(Conn, vol, tdir, tname,
                                           (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                                           (1 << FILPBIT_FINFO) |
                                           (1 << FILPBIT_CDATE) | (1 << FILPBIT_PDID)
                                           ,
                                           (1 << DIRPBIT_ATTR) |
                                           (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                                           (1 << DIRPBIT_ACCESS)
                                          )) {
        test_failed();
    }

    /* ---- desktop.c ---- */
    dt = FPOpenDT(Conn, vol);
    FAIL(ntohl(AFPERR_NOOBJ) != FPAddComment(Conn, vol, tdir, tname, "Comment"))
    FAIL(ntohl(AFPERR_NOOBJ) != FPGetComment(Conn, vol, tdir, tname))
    FAIL(ntohl(AFPERR_NOOBJ) != FPRemoveComment(Conn, vol, tdir, tname))
    FAIL(FPCloseDT(Conn, dt))
test_exit:
    exit_test("Error:test174: did error two users from parent folder did=<deleted> name=test174 name");
}

/* ----------- */
void Error_test()
{
    ENTER_TESTSET
    test35();
    test36();
    test37();
    test95();
    test99();
    test100();
    test101();
    test102();
    test103();
    test105();
    test170();
    test171();
    test173();
    test174();
}
