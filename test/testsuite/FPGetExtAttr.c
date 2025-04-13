/* ----------------------------------------------
*/
#include "specs.h"
#include <inttypes.h>


/* ------------------------- */
STATIC void test399()
{
uint16_t vol = VolID;
char *file="test399_file";
char *attr_name="test399_attribute";
int remerror = AFPERR_NOITEM;

	ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }
	if (adouble == AD_V2) {
		test_skipped(T_ADEA);
		goto test_exit;
	}
    if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_EXTATTRS)) {
        test_skipped(T_EA);
        goto test_exit;
    }

    /* AFP 3.4 sends AFPERR_NOITEM instead of AFPERR_MISC */
    if (Conn->afp_version < 34) {
        remerror = AFPERR_MISC;
    }

    if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , file)) {
    	test_nottested();
        goto test_exit;
    }

	FAIL(FPSetExtAttr(Conn,vol, DIRDID_ROOT, 2, file, attr_name, "test399_data"))
	FAIL(FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 4096, file, attr_name))

	/* check create flag */
	if (ntohl(AFPERR_EXIST) != FPSetExtAttr(Conn, vol, DIRDID_ROOT, 2, file, attr_name, "test399_newdata"))
		test_failed();

	FAIL(FPListExtAttr(Conn,vol, DIRDID_ROOT , 0, 4096, file))

	FAIL(FPSetExtAttr(Conn, vol, DIRDID_ROOT, 4, file, attr_name, "test399_newdata"))
	FAIL(FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 4096, file, attr_name))
	FAIL(FPRemoveExtAttr(Conn,vol, DIRDID_ROOT , 0, file, attr_name))
	if (ntohl(remerror) != FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 4096, file, attr_name)) {
		test_failed();
	}
	if (ntohl(AFPERR_MISC) != FPRemoveExtAttr(Conn,vol, DIRDID_ROOT , 0, file, attr_name))
		test_failed();

    FPDelete(Conn, vol,  DIRDID_ROOT , file);

test_exit:
	exit_test("FPGetExtAttr:test399: check Extended Attributes Support");
}

/* ------------------------- */
STATIC void test416()
{
uint16_t vol = VolID;
char *file="test416_file";
char *file1="test416 new file";
char *attr_name="test416_attribute";

	ENTER_TEST

    if (Conn->afp_version < 32) {
        test_skipped(T_AFP32);
        goto test_exit;
    }
	if (adouble == AD_V2) {
		test_skipped(T_ADEA);
		goto test_exit;
	}
    if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_EXTATTRS)) {
        test_skipped(T_EA);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , file)) {
    	test_nottested();
        goto test_exit;
    }

	FAIL(FPSetExtAttr(Conn,vol, DIRDID_ROOT, 2, file, attr_name, "test416_data"))
	FAIL(FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 4096, file, attr_name))

	FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, file, "", file1))

	FAIL(FPListExtAttr(Conn,vol, DIRDID_ROOT , 0, 4096, file))

	FAIL(FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 4096, file, attr_name))
	FAIL(FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 4096, file1, attr_name))

	FAIL(FPRemoveExtAttr(Conn,vol, DIRDID_ROOT , 0, file1, attr_name))

    FAIL(FPDelete(Conn, vol,  DIRDID_ROOT , file))
    FAIL(FPDelete(Conn, vol,  DIRDID_ROOT , file1))

test_exit:
	exit_test("FPGetExtAttr:test416: copy file with Extended Attributes");
}

/* ------------------------- */
STATIC void test432()
{
	uint16_t vol = VolID;
	const DSI *dsi;
	char *file="test432_file";
	char *attr_name="test432_attribute";
	uint32_t attrlen;
	char *attrdata = "1234567890";
	size_t attrdata_len = 10;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}
	if (adouble == AD_V2) {
		test_skipped(T_ADEA);
		goto test_exit;
	}
	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_EXTATTRS)) {
		test_skipped(T_EA);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , file)) {
		test_nottested();
		goto test_exit;
	}

	FAIL(FPSetExtAttr(Conn,vol, DIRDID_ROOT, 2, file, attr_name, attrdata))

	/*
	 * The xattr we set is 10 bytes large. The req_count must be
	 * at least 10 bytes + 6 bytes. The 6 bytes are the AFP reply
	 * packets bitmap and length field.
	 *
	 * This means that Apple's protocol implementation doesn't
	 * support returning truncated xattrs. From the spec it would
	 * be possible to get just the first byte of an xattr by
	 * specifying an req_count of 7.
	 */
	FAIL ( htonl(AFPERR_PARAM) != FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 1, file, attr_name));
	FAIL ( htonl(AFPERR_PARAM) != FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 6, file, attr_name));
	FAIL ( htonl(AFPERR_PARAM) != FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 7, file, attr_name));
	FAIL ( htonl(AFPERR_PARAM) != FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 15, file, attr_name));

	FAIL( FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 16, file, attr_name) );

	FAIL( FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 17, file, attr_name) );
	memcpy(&attrlen, dsi->data + 2, 4);
	attrlen = ntohl(attrlen);
	if (attrlen != attrdata_len) {
		test_failed();
	}

	FPDelete(Conn, vol,  DIRDID_ROOT , file);

test_exit:
	exit_test("FPGetExtAttr:test432: set and get Extended Attributes");
}

/* ----------- */
void FPGetExtAttr_test()
{
    ENTER_TESTSET

    test399();
	test416();
    test432();
}
