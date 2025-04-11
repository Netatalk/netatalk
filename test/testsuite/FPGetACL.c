/* ----------------------------------------------
*/
#include "specs.h"
#include <inttypes.h>

/* ------------------------- */
STATIC void test398()
{
uint16_t vol = VolID;
DSI *dsi;
char *file="test398_file";

    dsi = &Conn->dsi;

    ENTER_TEST
    if (Conn->afp_version < 30) {
    	test_skipped(T_AFP3);
        goto test_exit;
    }

    if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_ACLS)) {
       test_skipped(T_ACL);
       goto test_exit;
    }

    if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , file)) {
    	test_nottested();
        goto test_exit;
    }

	FAIL(FPGetACL(Conn,vol, DIRDID_ROOT , 7, file))
/*
	FPGetACL(Conn,vol, DIRDID_ROOT , 7, "testdir");
	FPGetACL(Conn,vol, DIRDID_ROOT , 8, "test");
	FPGetACL(Conn,vol, DIRDID_ROOT , 23, "test");
	FPGetACL(Conn,vol, DIRDID_ROOT , 23, "test2");
	FPGetACL(Conn,vol, DIRDID_ROOT , 23, "test3");
	FPGetACL(Conn,vol, DIRDID_ROOT , 23, "testdir");
*/
    FPDelete(Conn, vol,  DIRDID_ROOT , file);
test_exit:
	exit_test("FPGetACL:test398: check ACL support");
}


/* ------------------------- */
STATIC void test399()
{
uint16_t vol = VolID;
uint16_t bitmap;
DSI *dsi;
char *file="test399_file";
char *attr_name="test399_attribute";
int remerror = AFPERR_NOITEM;

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
        test_skipped(T_UTF8);
        goto test_exit;
    }
    /* AFP 3.4 sends AFPERR_NOITEM instead of AFPERR_MISC */
    if (Conn->afp_version < 34) {
        remerror = AFPERR_MISC;
    }

    bitmap = (1<< FILPBIT_PDID) | (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_RFLEN);

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

STATIC void test432()
{
	uint16_t vol = VolID;
	uint16_t bitmap;
	DSI *dsi;
	char *file="test432_file";
	char *attr_name="test432_attribute";
	int ret;
	uint32_t attrlen;
	char *attrdata = "1234567890";

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
		test_skipped(T_UTF8);
		goto test_exit;
	}

	bitmap = (1<< FILPBIT_PDID) | (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_RFLEN);

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
	if (attrlen != strlen(attrdata)) {
		test_failed();
	}

	FPDelete(Conn, vol,  DIRDID_ROOT , file);

test_exit:
	exit_test("FPGetExtAttr:test432: set and get Extended Attributes");
}

/* ----------- */
void FPGetACL_test()
{
    ENTER_TESTSET

    test398();
    test399();
    test432();
}
