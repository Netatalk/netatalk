/* ----------------------------------------------
*/
#include "specs.h"


/* ------------------------- */
STATIC void test227()
{
uint16_t bitmap = (1<<FILPBIT_ATTR);
uint16_t bitmap2;
char *name = "t227 file.txt";
uint16_t vol = VolID;
uint32_t temp;
DSI *dsi;
char pos[16];
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
struct afp_filedir_parms filedir2;
unsigned int ret;

	enter_test();
	dsi = &Conn->dsi;

	memset(pos, 0, sizeof(pos));
	memset(&filedir, 0, sizeof(filedir));

	filedir.attr = 0x01a0;			/* various lock attributes */
	ret = FPCatSearchExt(Conn, vol, 10, pos, 0,  /* d_bitmap*/ 0, bitmap, &filedir, &filedir);

	if (Conn->afp_version < 30) {
		if (htonl(AFPERR_NOOP) != ret) {
			failed();
		}
		else {
			test_skipped(T_AFP3);
		}
		goto test_exit;
	}
	if (htonl(AFPERR_BITMAP) != ret) {
		failed();
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

	ret = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  /* d_bitmap*/ 0, bitmap, &filedir, &filedir);
	if (ret != htonl(AFPERR_EOF)) {
		failed();
	}
	memcpy(&temp, dsi->data + 20, sizeof(temp));
	temp = ntohl(temp);
	if (temp) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED want 0 get %d\n", temp);
		}
		failed_nomsg();
	}

	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	filedir.attr = 0x01a0 | ATTRBIT_SETCLR ;
 	FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir))

	memset(&filedir, 0, sizeof(filedir));
	/* ------------------- */
	filedir.attr = 0x01a0;			/* lock attributes */
	ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  /* d_bitmap*/ 0, bitmap, &filedir, &filedir);
	if (ret != htonl(AFPERR_EOF)) {
		failed();
	}
	memcpy(&temp, dsi->data + 20, sizeof(temp));
	temp = ntohl(temp);
	if (temp != 1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED want 1 get %d\n", temp);
		}
		failed_nomsg();
	}

	/* ------------------- */
	filedir.attr = 0x0100;			/* lock attributes */
	ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  /* d_bitmap*/ 0, bitmap, &filedir, &filedir);
	if (ret != htonl(AFPERR_EOF)) {
		failed();
	}
	memcpy(&temp, dsi->data + 20, sizeof(temp));
	temp = ntohl(temp);
	if (temp != 1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED want 1 get %d\n", temp);
		}
		failed_nomsg();
	}
#if 1
	/* -------------------- */
	memset(&filedir, 0, sizeof(filedir));
	memset(&filedir2, 0, sizeof(filedir2));
	filedir.lname = "Data";

	ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  0x42, 0x80000000UL| (1<< FILPBIT_LNAME), &filedir, &filedir2);
	while (!ret ) {
		memcpy(pos, dsi->data ,16);
		ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  0x42, 0x80000000UL| (1<< FILPBIT_LNAME), &filedir, &filedir2);
	}
	/* -------------------- */
	memset(pos, 0, sizeof(pos));
	ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  0x42, 0x80000000UL| (1<< FILPBIT_LNAME), &filedir, &filedir2);
	if (!ret ) {
		memcpy(pos, dsi->data ,16);
		ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  0x42, 0x80000000UL| (1<< FILPBIT_LNAME), &filedir, &filedir2);
	}

	/* -------------------- */
	memset(pos, 0, sizeof(pos));
	filedir.lname = "test";
	ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  0x42, 0x80000000UL| (1<< FILPBIT_LNAME), &filedir, &filedir2);
	if (!ret ) {
		memcpy(pos, dsi->data ,16);
		ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  0x42, 0x80000000UL| (1<< FILPBIT_LNAME), &filedir, &filedir2);
	}
	/* -------------------- */
	memset(pos, 0, sizeof(pos));
	filedir.utf8_name = "test";
	bitmap2 = (1<< FILPBIT_PDINFO)| (1 << FILPBIT_PDID);
	ret  = FPCatSearchExt(Conn, vol, 10, pos, bitmap2,  bitmap2, 0x80000000UL| (1<< FILPBIT_PDINFO), &filedir, &filedir2);
	while (!ret ) {
		memcpy(pos, dsi->data ,16);
		ret  = FPCatSearchExt(Conn, vol, 10, pos, bitmap2,  bitmap2, 0x80000000UL| (1<< FILPBIT_PDINFO), &filedir, &filedir2);
	}
#endif
	/* -------------------- */
	memset(&filedir, 0, sizeof(filedir));
	filedir.attr = 0x01a0;
 	FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("FPCatSearchExt:test227: Catalog search");
}

/* ----------- */
void FPCatSearchExt_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPCatSearchExt page 117\n");
    fprintf(stdout,"-------------------\n");
	test227();
}
