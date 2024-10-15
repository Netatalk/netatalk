/* ------------------------- */
void test154()
{
char *name  = "t154.txt";
uint16_t bitmap = 0;
int fork = 0, fork1 = 0;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test154: no adouble create/open/write file\n");

	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){ 
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	
	sprintf(temp, "%s/.AppleDouble/%s", Path, name);
	if (!unlink(temp)) {
	    if (Noadouble) {
			fprintf(stdout,"\tFAILED unlink %s was there\n", temp);
			sprintf(temp, "%s/.AppleDouble", Path);
			if (rmdir(temp)) {
				fprintf(stdout,"\tFAILED rmdir %s  %s\n", temp, strerror(errno));
			}
		}
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name, 
		OPENACC_WR | OPENACC_RD| OPENACC_DWR| OPENACC_DRD);
	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
		goto end;
	}		

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, 
		OPENACC_WR | OPENACC_RD| OPENACC_DWR| OPENACC_DRD);

	if (!fork1) {
		fprintf(stdout,"\tFAILED\n");
		goto end;
	}

	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 19696)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPFlushFork(Conn, fork)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPSetForkParam(Conn, fork1, (1<<FILPBIT_RFLEN), 2336)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPFlushFork(Conn, fork1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPWrite(Conn, fork, 0, 19696, Data, 0 )) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPWrite(Conn, fork1, 0, 2336, Data, 0 )) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPGetForkParam(Conn, fork1, 0x442)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPRead(Conn, fork, 0, 2000, Data)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 19696)) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (FPCloseFork(Conn, fork)) {
		fprintf(stdout,"\tFAILED\n");
	}
	fork = 0;
	if (FPSetForkParam(Conn, fork1, (1<<FILPBIT_RFLEN), 2336)) {
		fprintf(stdout,"\tFAILED\n");
	}

end:
	if (fork && FPCloseFork(Conn, fork)) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (fork1 && FPCloseFork(Conn, fork1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) { 
		fprintf(stdout,"\tFAILED\n");
	}
	if (Noadouble) {
		sprintf(temp, "%s/.AppleDouble", Path);
		if (rmdir(temp)) {
			fprintf(stdout,"\tFAILED rmdir %s  %s\n", temp, strerror(errno));
		}
	}

}

/* ------------------------- */
void test165()
{
char *name  = "t165.txt";
uint16_t bitmap = 0;
int fork = 0, fork1 = 0;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test165: no adouble getcomment\n");

	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){ 
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	
	sprintf(temp, "%s/.AppleDouble/%s", Path, name);
	if (!unlink(temp)) {
	    if (Noadouble) {
			fprintf(stdout,"\tFAILED unlink %s was there\n", temp);
			sprintf(temp, "%s/.AppleDouble", Path);
			if (rmdir(temp)) {
				fprintf(stdout,"\tFAILED rmdir %s  %s\n", temp, strerror(errno));
			}
		}
	}
	if (ntohl(AFPERR_ACCESS) != FPAddComment(Conn, vol,  DIRDID_ROOT , name,"essai")) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) { 
		fprintf(stdout,"\tFAILED\n");
	}
	if (Noadouble) {
		sprintf(temp, "%s/.AppleDouble", Path);
		if (!rmdir(temp)) {
			fprintf(stdout,"\tFAILED rmdir %s  no error\n", temp);
		}
	}
}

/* ------------------------- */
void test168()
{
int  dir;
char *name = "t168 setfilparam noadouble";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)| (1<<FILPBIT_CDATE) | 
					(1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE);

    fprintf(stdout,"===================\n");
    fprintf(stdout,"t168: test set file setfilparam noadouble\n");

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	sprintf(temp, "%s/.AppleDouble/%s", Path, name);
	if (!unlink(temp)) {
	    if (Noadouble) {
			fprintf(stdout,"\tFAILED unlink %s was there\n", temp);
			sprintf(temp, "%s/.AppleDouble", Path);
			if (rmdir(temp)) {
				fprintf(stdout,"\tFAILED rmdir %s  %s\n", temp, strerror(errno));
			}
		}
	}

	filedir.isdir = 0;
    bitmap = (1<<FILPBIT_MDATE);

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0)) {
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
 		if (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir)) {
			fprintf(stdout,"\tFAILED\n");
 		}

	}
	bitmap = (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)| (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0)) {
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
 		if (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir)) {
			fprintf(stdout,"\tFAILED\n");
 		}

	}
	if (FPDelete(Conn, vol,  DIRDID_ROOT , name)) { 
		fprintf(stdout,"\tFAILED\n");
		return;
	}
}

/* ------------------------- */
void test176()
{
int  dir;
char *name = "t176 setdirparam noadouble";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"t176: test set dir setdirparam noadouble\n");

	if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT , name))) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	sprintf(temp, "%s/%s/.AppleDouble/.Parent", Path, name);
	if (!unlink(temp)) {
	    if (Noadouble) {
			fprintf(stdout,"\tFAILED unlink %s was there\n", temp);
			sprintf(temp, "%s/%s/.AppleDouble", Path, name);
			if (rmdir(temp)) {
				fprintf(stdout,"\tFAILED rmdir %s  %s\n", temp, strerror(errno));
			}
		}
	}

	filedir.isdir = 1;
    bitmap = (1<<DIRPBIT_MDATE);

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, 0,bitmap)) {
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
 		if (FPSetDirParms(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir)) {
			fprintf(stdout,"\tFAILED\n");
 		}

	}
	bitmap = (1<< DIRPBIT_ATTR) |(1<<DIRPBIT_FINFO)| (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, 0, bitmap)) {
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
 		if (FPSetDirParms(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir)) {
			fprintf(stdout,"\tFAILED\n");
 		}

	}

	if (htonl(AFPERR_BITMAP) !=FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, 0, 0xffff)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT , name)) { fprintf(stdout,"\tFAILED\n");}
}

/* ------------------------- */
void test179()
{
int  dir;
char *name = "t179 test dir noadouble";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap =  (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE)
|(1 << DIRPBIT_ACCESS) | (1<<DIRPBIT_FINFO)| (1<<DIRPBIT_UID) | (1 << DIRPBIT_GID) ;


    fprintf(stdout,"===================\n");
    fprintf(stdout,"t179: test dir noadouble\n");

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, 0,bitmap )) {
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
 		if (FPSetDirParms(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir)) {
			fprintf(stdout,"\tFAILED\n");
 		}
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT , name)) { 
		fprintf(stdout,"\tFAILED\n");
	}
}

