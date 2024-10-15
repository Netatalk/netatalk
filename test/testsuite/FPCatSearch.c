/* ----------------------------------------------
*/
#include "specs.h"


/* ------------------------- */
STATIC void test225()
{
uint16_t bitmap = (1<<FILPBIT_ATTR);
char *name = "t225 file.txt";
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
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPCatSearch:test225: Catalog search\n");

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

	memset(pos, 0, sizeof(pos));
	memset(&filedir, 0, sizeof(filedir));
	filedir.attr = 0x01a0;			/* various lock attributes */
	
	FAIL( htonl(AFPERR_BITMAP) != FPCatSearch(Conn, vol, 10, pos, 0,  /* d_bitmap*/ 0, bitmap, &filedir, &filedir))

	filedir.attr = 0x01a0;			/* various lock attributes */
	ret = FPCatSearch(Conn, vol, 10, pos, 0x42,  /* d_bitmap*/ 0, bitmap, &filedir, &filedir);
	if (ret != htonl(AFPERR_EOF)) {
		failed();
	}
	memcpy(&temp, dsi->data + 20, sizeof(temp));
	temp = ntohl(temp);
	if (temp) {
		fprintf(stdout,"\tFAILED want 0 get %d\n", temp);
		failed_nomsg();
	}

	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	filedir.attr = 0x01a0 | ATTRBIT_SETCLR ;
 	FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir)) 

	memset(&filedir, 0, sizeof(filedir));
	/* ------------------- */
	filedir.attr = 0x01a0;			/* lock attributes */
	ret  = FPCatSearch(Conn, vol, 10, pos, 0x42,  /* d_bitmap*/ 0, bitmap, &filedir, &filedir);
	if (ret != htonl(AFPERR_EOF)) {
		failed();
	}
	memcpy(&temp, dsi->data + 20, sizeof(temp));
	temp = ntohl(temp);
	if (temp != 1) {
		fprintf(stdout,"\tFAILED want 1 get %d\n", temp);
		failed_nomsg();
	}

	/* ------------------- */
	filedir.attr = 0x0100;			/* lock attributes */
	ret  = FPCatSearch(Conn, vol, 10, pos, 0x42,  /* d_bitmap*/ 0, bitmap, &filedir, &filedir);
	if (ret != htonl(AFPERR_EOF)) {
		failed();
	}
	memcpy(&temp, dsi->data + 20, sizeof(temp));
	temp = ntohl(temp);
	if (temp != 1) {
		fprintf(stdout,"\tFAILED want 1 get %d\n", temp);
		failed_nomsg();
	}
	/* -------------------- */
#if 1
	memset(&filedir, 0, sizeof(filedir));
	memset(&filedir2, 0, sizeof(filedir2));
	filedir.lname = "Data";

	ret = FPCatSearch(Conn, vol, 20, pos, 0x42, 0x42, 0x80000000UL| (1<< FILPBIT_LNAME), &filedir, &filedir2);
	while (!ret) {
		memcpy(pos, dsi->data ,16);
		ret = FPCatSearch(Conn, vol, 20, pos, 0x42, 0x42, 0x80000000UL| (1<< FILPBIT_LNAME), &filedir, &filedir2);
	}
	
	/* -------------------- */
#endif
	memset(&filedir, 0, sizeof(filedir));
	filedir.attr = 0x01a0;
 	FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir)) 
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name)) 
test_exit:
	exit_test("test225");
}

/* ----------- */
void FPCatSearch_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPCatSearch page 110\n");
	test225();
}

