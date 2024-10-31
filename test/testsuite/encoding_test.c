#include "specs.h"
#include <fcntl.h>

static char temp[MAXPATHLEN];
static char *extascii[24] = {
	" ?\"#$%&'", // substituted: !
	"()*+,-.?", // substituted: /
	"01234567",
	"89?;<=>?", // substituted: :
	"@ABCDEFG",
	"HIJKLMNO",
	"PQRSTUVW",
	"XYZ[\\]^_",
	"`abcdefg",
	"hijklmno",
	"pqrstuvw",
	"xyz{|}~¡",
	"¢£¤¥¦§¨©",
	"ª«¬­®¯°±",
	"²³´µ¶·¸¹",
	"º»¼½¾¿ÀÁ",
	"ÂÃÄÅÆÇÈÉ",
	"ÊËÌÍÎÏÐÑ",
	"ÒÓÔÕÖ×ØÙ",
	"ÚÛÜÝÞßàá",
	"âãäåæçèé",
	"êëìíîïðñ",
	"òóôõö÷øù",
	"úûüýþÿ"
};

/* ------------------
 * client encoding western 1 byte [1..255]
 * NOTE: This test was original conceived to test
 * Extended ASCII encoding.
 * On today's systems this encoding doesn't exist in practice.
 * So, this test tests the equivalent Unicode character instead.
*/
static void test_western()
{
uint16_t vol = VolID;
uint16_t f_bitmap;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
char *result;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (Conn->afp_version >= 30) {
		f_bitmap = (1<<FILPBIT_PDINFO);
	}
	else {
		f_bitmap = (1<<FILPBIT_LNAME);
	}
    for (int i = 0; i < 24; i++) {
		if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , extascii[i])) {
			test_nottested();
			goto test_exit;
		}
		if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, extascii[i], f_bitmap, 0)) {
			test_nottested();
			goto test_exit;
		}
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, f_bitmap, 0);
		result = (Conn->afp_version >= 30)?filedir.utf8_name:filedir.lname;
		if (strcmp(result, extascii[i])) {
			test_failed();
			goto test_exit;
		}
		if (FPDelete(Conn, vol, DIRDID_ROOT, extascii[i])) {
			test_nottested();
			goto test_exit;
		}
	}
	if (Path[0] != '\0') {
	int fd;

		sprintf(temp,"%s/:test", Path);
		fd = open(temp, O_RDWR | O_CREAT, 0666);
		if (fd < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to create %s :%s\n", temp, strerror(errno));
			}
			test_failed();
			goto test_exit;
		}
		else {
			close(fd);
		}
	}
test_exit:
	exit_test("Encoding:western");
}

/* ----------- */
void Encoding_test()
{
    ENTER_TESTSET
	test_western();
}
