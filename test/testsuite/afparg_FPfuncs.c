#include "specs.h"

void FPCopyFile_arg(char **argv)
{
    uint16_t vol = VolID;

    fprintf(stdout,"======================\n");
    fprintf(stdout,"FPCopyFile with args:\n");

    fprintf(stdout,"source: \"%s\" -> dest: \"%s\"\n", argv[0], argv[1]);

	FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, argv[0], "", argv[1]))

}

/* ----------- */
void FPResolveID_arg(char **argv)
{
    int argc = 0;
    unsigned int ret, ofs = 3 * sizeof( uint16_t );
    uint16_t bitmap = ( 1 << FILPBIT_PDINFO );
    uint32_t id;
    DSI *dsi;
    struct afp_filedir_parms filedir;

    dsi = &Conn->dsi;

    fprintf(stdout,"======================\n");
    fprintf(stdout,"FPResolveID with args:\n");

    id = atoi(argv[0]);
    fprintf(stdout,"Trying to resolve id %u\n", id);

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
        return;
	}

	if ( !(get_vol_attrib(VolID) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		return;
	}


	if (!(get_vol_attrib(VolID) & VOLPBIT_ATTR_FILEID) ) {
		test_skipped(T_ID);
		return;
	}

	if ( FPResolveID(Conn, VolID, htonl(id), bitmap) ) {
        failed();
        return;
    }
	filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + 2, bitmap, 0);

    fprintf(stdout,"Resolved ID %d to: '%s'\n", id, filedir.utf8_name);
}

static void handler()
{
    return;
}

void FPLockrw_arg(char **argv)
{
    uint16_t vol = VolID;
    int fork;
    struct sigaction action;
    int toopen;

    if (argv[0][0] == 'd')
        toopen = OPENFORK_DATA;
    else
        toopen = OPENFORK_RSCS;

    fprintf(stdout,"======================\n");
    fprintf(stdout,"FPOpen with read/write lock\n");

    fprintf(stdout,"source: \"%s\"\n", argv[1]);

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    if ((sigaction(SIGINT, &action, NULL) < 0)) {
		nottested();
		goto test_exit;
    }

	fork = FPOpenFork(Conn, vol, toopen, 0, DIRDID_ROOT, argv[1],
                      OPENACC_RD | OPENACC_WR | OPENACC_DRD | OPENACC_DWR);
	if (!fork) {
		nottested();
		goto fin;
	}

    pause();

	FAIL (FPCloseFork(Conn,fork))

fin:
test_exit:
    action.sa_handler = SIG_DFL;
    (void)sigaction(SIGINT, &action, NULL);
}

void FPLockw_arg(char **argv)
{
    uint16_t vol = VolID;
    int fork;
    struct sigaction action;
    int toopen;

    if (argv[0][0] == 'd')
        toopen = OPENFORK_DATA;
    else
        toopen = OPENFORK_RSCS;


    fprintf(stdout,"======================\n");
    fprintf(stdout,"FPOpen with write lock\n");

    fprintf(stdout,"source: \"%s\"\n", argv[1]);

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    if ((sigaction(SIGINT, &action, NULL) < 0)) {
		nottested();
		goto test_exit;
    }

	fork = FPOpenFork(Conn, vol, toopen, 0, DIRDID_ROOT, argv[1],
                      OPENACC_RD | OPENACC_DWR);
	if (!fork) {
		nottested();
		goto fin;
	}

    pause();

	FAIL (FPCloseFork(Conn,fork))

fin:
test_exit:
    action.sa_handler = SIG_DFL;
    (void)sigaction(SIGINT, &action, NULL);
}
