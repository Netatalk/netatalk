/* ----------------------------------------------
*/
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

/* ------------------------- */
STATIC void test225()
{
    uint16_t bitmap = (1 << FILPBIT_ATTR);
    char *name = "t225 file.txt";
    uint16_t vol = VolID;
    uint32_t temp;
    const DSI *dsi = &Conn->dsi;
    char pos[16];
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    struct afp_filedir_parms filedir2;
    unsigned int ret;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    memset(pos, 0, sizeof(pos));
    memset(&filedir, 0, sizeof(filedir));
    filedir.attr = 0x01a0;			/* various lock attributes */
    FAIL(htonl(AFPERR_BITMAP) != FPCatSearch(Conn, vol, 10, pos,
            0, /* d_bitmap */ 0, bitmap, &filedir, &filedir))
    filedir.attr = 0x01a0;			/* various lock attributes */
    ret = FPCatSearch(Conn, vol, 10, pos, 0x42, /* d_bitmap */ 0,
                      bitmap, &filedir, &filedir);

    if (ret != htonl(AFPERR_EOF)) {
        test_failed();
    }

    memcpy(&temp, dsi->data + 20, sizeof(temp));
    temp = ntohl(temp);

    if (temp) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED want 0 get %d\n", temp);
        }

        test_failed();
    }

    filedir.isdir = 0;
    afp_filedir_unpack(Conn, &filedir, dsi->data + ofs, bitmap, 0);
    filedir.attr = 0x01a0 | ATTRBIT_SETCLR ;
    FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
    memset(&filedir, 0, sizeof(filedir));
    /* ------------------- */
    filedir.attr = 0x01a0;			/* lock attributes */
    ret  = FPCatSearch(Conn, vol, 10, pos, 0x42, /* d_bitmap*/ 0, bitmap,
                       &filedir, &filedir);

    if (ret != htonl(AFPERR_EOF)) {
        test_failed();
    }

    memcpy(&temp, dsi->data + 20, sizeof(temp));
    temp = ntohl(temp);

    if (temp != 1) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED want 1 get %d\n", temp);
        }

        test_failed();
    }

    /* ------------------- */
    filedir.attr = 0x0100;			/* lock attributes */
    ret  = FPCatSearch(Conn, vol, 10, pos, 0x42, /* d_bitmap */ 0, bitmap, &filedir,
                       &filedir);

    if (ret != htonl(AFPERR_EOF)) {
        test_failed();
    }

    memcpy(&temp, dsi->data + 20, sizeof(temp));
    temp = ntohl(temp);

    if (temp != 1) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED want 1 get %d\n", temp);
        }

        test_failed();
    }

    /* -------------------- */
    memset(&filedir, 0, sizeof(filedir));
    memset(&filedir2, 0, sizeof(filedir2));
    filedir.lname = "Data";
    ret = FPCatSearch(Conn, vol, 20, pos, 0x42, 0x42,
                      0x80000000UL | (1 << FILPBIT_LNAME), &filedir, &filedir2);

    while (!ret) {
        memcpy(pos, dsi->data, 16);
        ret = FPCatSearch(Conn, vol, 20, pos, 0x42, 0x42,
                          0x80000000UL | (1 << FILPBIT_LNAME), &filedir, &filedir2);
    }

    /* -------------------- */
    memset(&filedir, 0, sizeof(filedir));
    filedir.attr = 0x01a0;
    FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPCatSearch:test225: Catalog search");
}

/* ------------------------- */
STATIC void test551()
{
    uint16_t vol = VolID;
    uint16_t bitmap;
    uint16_t spec_len;
    uint32_t temp;
    uint32_t rbitmap;
    char pos[16];
    int ofs;
    DSI *dsi;
    ENTER_TEST
    dsi = &Conn->dsi;
    memset(pos, 0, sizeof(pos));
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_CATSEARCH;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    temp = htonl(10);
    memcpy(dsi->commands + ofs, &temp, sizeof(temp));
    ofs += sizeof(temp);
    temp = 0;
    memcpy(dsi->commands + ofs, &temp, sizeof(temp));
    ofs += sizeof(temp);
    memcpy(dsi->commands + ofs, pos, sizeof(pos));
    ofs += sizeof(pos);
    bitmap = htons(0x42);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    bitmap = 0;
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    rbitmap = htonl(1 << FILPBIT_ATTR);
    memcpy(dsi->commands + ofs, &rbitmap, sizeof(rbitmap));
    ofs += sizeof(rbitmap);
    spec_len = 0;
    memcpy(dsi->commands + ofs, &spec_len, sizeof(spec_len));
    ofs += sizeof(spec_len);
    memcpy(dsi->commands + ofs, &spec_len, sizeof(spec_len));
    ofs += sizeof(spec_len);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    dsi_data_receive(dsi);

    if (dsi->header.dsi_code != htonl(AFPERR_PARAM)) {
        test_failed();
    }

    exit_test("FPCatSearch:test551: truncated search-spec payload");
}

/* ----------- */
void FPCatSearch_test()
{
    ENTER_TESTSET
    test225();
    test551();
}
