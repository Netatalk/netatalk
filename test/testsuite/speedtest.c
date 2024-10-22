#include "specs.h"
#include <dlfcn.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* For compiling os OS X */
#ifndef MAP_ANONYMOUS
#ifdef MAP_ANON
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

uint16_t VolID;
uint16_t VolID2;
static DSI *dsi;
CONN *Conn;

int ExitCode = 0;

DSI *Dsi;
char Data[30000];
char *Buffer;
struct timeval Timer_start;
struct timeval Timer_end;
#define KILOBYTE 1024
#define MEGABYTE (KILOBYTE*KILOBYTE)

/* ------------------------------- */
static char    *Server = "localhost";
static int     Proto = 0;
static int     Port = 548;
static char    *Password = "";
char    *Vol = "";
char    *Vol2 = "";
char    *User;
int     Version = 21;
static char    *Test = "Write";
static char    *Filename;

static int Count = 1;
static off_t Size = 64* MEGABYTE;
static size_t Quantum = 0;
static int Request = 1;
static int Req = 1;
static int Delete = 0;
static int Sparse = 0;
static int Local = 0;
static int Flush = 1;
static int Direct = 0;

/* not used */
CONN *Conn2;
int  Mac = 0;

struct vfs {
	unsigned int (*getfiledirparams)(CONN *, uint16_t , int , char *, uint16_t, uint16_t);
	unsigned int (*createdir)(CONN *, uint16_t , int , char *);
	unsigned int (*createfile)(CONN *, uint16_t , char , int , char *);
	uint16_t    (*openfork)(CONN *, uint16_t, int, uint16_t , int , char *, int );
	unsigned int (*writeheader)(DSI *, uint16_t , int , int , char *, char );
	unsigned int (*writefooter)(DSI *, uint16_t , int , int , char *, char );
	unsigned int (*flushfork)(CONN *, uint16_t );
	unsigned int (*closefork)(CONN *, uint16_t );
	unsigned int (*delete)(CONN *, uint16_t , int  , char *);
	unsigned int (*setforkparam)(CONN *, uint16_t ,  uint16_t , off_t );
	unsigned int (*write)(CONN *, uint16_t , long long , int , char *, char );
	unsigned int (*read)(CONN *, uint16_t , long long , int , char *);
	unsigned int (*readheader)(DSI *, uint16_t , int , int , char *);
	unsigned int (*readfooter)(DSI *, uint16_t , int , int , char *);
	unsigned int (*copyfile)(CONN *, uint16_t , int , uint16_t , int , char *, char *, char *);
	uint16_t    (*openvol)(CONN *, char *);
	unsigned int (*closevol)(CONN *conn, uint16_t vol);
};

struct vfs VFS = {
FPGetFileDirParams,
FPCreateDir,
FPCreateFile,
FPOpenFork,
FPWriteHeader,
FPWriteFooter,
FPFlushFork,
FPCloseFork,
FPDelete,
FPSetForkParam,
FPWrite,
FPRead,
FPReadHeader,
FPReadFooter,
FPCopyFile,
FPOpenVol,
FPCloseVol
};

/* very small heaps for Posix access */
#define  MAXDIR 10
#define MAXVOL 3
static char *Dir_heap[MAXVOL][MAXDIR];

static char *Vol_heap[MAXVOL];

/* -------------- */
uint16_t local_openvol(CONN *conn, char *vol)
{
uint16_t i;
struct stat st;

	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"Open Vol %s \n\n", vol);
	}

	if (stat(vol, &st)) {
		return 0xffff;
	}

	for (i= 0; i< MAXVOL; i++) {
		if (Vol_heap[i] == NULL) {
			if (chdir(vol)) {
				break;
			}
			Vol_heap[i] = strdup(vol);
			Dir_heap[i][2] = strdup(vol);
			return i;
		}
	}
	return 0xffff;
}

/* ------------------------------- */
unsigned int local_closevol(CONN *conn, uint16_t vol)
{
	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"Close Vol %d\n\n", vol);
	}
	return ntohl(AFP_OK);
}

/* ------------- */
static char temp[MAXPATHLEN +1];
static int local_chdir(uint16_t vol, int did)
{
	if (vol > MAXVOL || did > MAXDIR) {
		return -1;
	}
	if (!Vol_heap[vol] || !Dir_heap[vol][did]) {
		return -1;
	}
	if (chdir(Dir_heap[vol][did])) {
		return -1;
	}
	return 0;
}

/* ------------- */
unsigned int local_createdir(CONN *conn, uint16_t vol, int did , char *name)
{
unsigned int i;

	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"Create Directory Vol %d did : 0x%x <%s>\n\n", vol, ntohl(did), name);
	}
	did = ntohl(did);

	if (local_chdir(vol, did) < 0) {
		return 0;
	}
	for (i= 3; i< MAXDIR; i++) {
		if (Dir_heap[vol][i] == NULL) {
			if (mkdir(name, 0777)) {
				return ntohl(AFPERR_NOOBJ);
			}
			if (chdir (name) || NULL ==  getcwd(temp, sizeof(temp))) {
				return 0;
			}
			Dir_heap[vol][i] = strdup(temp);
			return htonl(i);
		}
	}
	return 0;
}

/* -------------- */
unsigned int local_getfiledirparams(CONN *conn, uint16_t vol, int did , char *name, uint16_t f_bitmap, uint16_t d_bitmap)
{
struct stat st;

	did = ntohl(did);
	if (local_chdir(vol, did) < 0) {
		return ntohl(AFPERR_NOOBJ);
	}
	if (*name != 0) {
		if (!stat(name, &st)) {
			return ntohl(AFP_OK);
		}
	}
	return ntohl(AFPERR_NOOBJ);
}

/* ------------- */
unsigned int local_delete(CONN *conn, uint16_t vol, int did , char *name)
{
	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"FPDelete Vol %d did : 0x%x <%s>\n\n", vol, ntohl(did), name);
	}
	did = ntohl(did);
	if (local_chdir(vol, did) < 0) {
		return ntohl(AFPERR_PARAM);
	}
	if (*name != 0) {
		if (unlink(name)) {
			return ntohl(AFPERR_NOOBJ);
		}
	}
	else if (rmdir(Dir_heap[vol][did])) {
		return ntohl(AFPERR_NOOBJ);
	}
	return ntohl(AFP_OK);
}

/* ------------------------- */
unsigned int local_createfile(CONN *conn, uint16_t vol, char type, int did , char *name)
{
int fd;

	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"Create File %s Vol %d did : 0x%x <%s>\n\n", (type )?"HARD":"SOFT", vol, ntohl(did), name);
	}
	did = ntohl(did);
	if (local_chdir(vol, did) < 0) {
		return ntohl(AFPERR_PARAM);
	}
	fd = open(name, O_RDWR| O_CREAT, 0666);
	if (fd == -1) {
		return ntohl(AFPERR_NOOBJ);
	}
	close(fd);
	return ntohl(AFP_OK);
}

/* ------------------------- */
#ifndef O_DIRECT
/* XXX hack */
#define O_DIRECT 040000
#endif

uint16_t local_openfork(CONN *conn, uint16_t vol, int type, uint16_t bitmap, int did , char *name, int access)
{
int fd;
int flags = O_RDWR;

	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"Open Fork %s Vol %d did : 0x%x <%s> access %x\n\n", (type == OPENFORK_DATA)?"data":"resource",
						vol, ntohl(did), name, access);
	}
	if (Direct) {
		flags |= O_DIRECT;
	}
	did = ntohl(did);
	if (local_chdir(vol, did) < 0) {
		return ntohl(AFPERR_PARAM);
	}
	fd = open(name, flags, 0666);
	if (fd == -1) {
		return ntohl(AFPERR_NOOBJ);
	}
	return fd;
}

/* ------------------------------- */
unsigned int local_writeheader(DSI *dsi, uint16_t fork, int offset, int size, char *data, char whence)
{
	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"send write header fork %d  offset %d size %d from 0x%x\n\n", fork , offset, size, (unsigned)whence);
	}
	if (lseek(fork, offset, SEEK_SET) == (off_t)-1) {
		return ntohl(AFPERR_EOF);
	}
	if (write(fork, data, size) != size) {
		return ntohl(AFPERR_EOF);
	}
	return ntohl(AFP_OK);
}

/* ------------------------------- */
unsigned int local_writefooter(DSI *dsi, uint16_t fork, int offset, int size, char *data, char whence)
{
	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"get write footer fork %d  offset %d size %d from 0x%x\n\n", fork , offset, size, (unsigned)whence);
	}
	return ntohl(AFP_OK);
}

/* ------------------------------- */
unsigned int local_flushfork(CONN *conn, uint16_t fork)
{
	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"Flush fork %d\n\n", fork);
	}
	if (fsync(fork) <0 ) {
		return ntohl(AFPERR_PARAM);
	}
	return ntohl(AFP_OK);
}

/* ------------------------------- */
unsigned int local_closefork(CONN *conn, uint16_t fork)
{
	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"Close Fork %d\n\n", fork);
	}
	if (close(fork)) {
		return ntohl(AFPERR_PARAM);
	}
	return ntohl(AFP_OK);
}

/* ------------------------------- */
unsigned int local_setforkparam(CONN *conn, uint16_t fork,  uint16_t bitmap, off_t size)
{
	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"Set Fork param fork %d bitmap 0x%x size %lld\n\n", fork, bitmap,size);
	}
	if (ftruncate(fork, size)) {
		return ntohl(AFPERR_PARAM);
	}
	return ntohl(AFP_OK);
}

/* ------------------------------- */
unsigned int local_write(CONN *conn, uint16_t fork, long long offset, int size, char *data, char whence)
{
	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"write fork %d  offset %d size %d from 0x%x\n\n", fork , offset, size, (unsigned)whence);
	}
	if (lseek(fork, offset, SEEK_SET) == (off_t)-1) {
		return ntohl(AFPERR_EOF);
	}
	if (write(fork, data, size) != size) {
		return ntohl(AFPERR_EOF);
	}
	return ntohl(AFP_OK);
}

/* ------------------------------- */
unsigned int local_read(CONN *conn, uint16_t fork, long long offset, int size, char *data)
{
	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"read fork %d  offset %d size %d\n\n", fork , offset, size);
	}
	if (lseek(fork, offset, SEEK_SET) == (off_t)-1) {
		return ntohl(AFPERR_EOF);
	}
	if (read(fork, data, size) != size) {
		return ntohl(AFPERR_EOF);
	}
	return ntohl(AFP_OK);
}

/* ------------------------------- */
unsigned int local_readheader(DSI *dsi, uint16_t fork, int offset, int size, char *data)
{
	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"send read header fork %d  offset %d size %d\n\n", fork , offset, size);
	}
	return ntohl(AFP_OK);
}

/* ------------------------------- */
unsigned int local_readfooter(DSI *dsi, uint16_t fork, int offset, int size, char *data)
{
	if (!Quiet) {
		fprintf(stdout,"---------------------\n");
		fprintf(stdout,"get read reply fork %d  offset %d size %d\n\n", fork , offset, size);
	}
	if (lseek(fork, offset, SEEK_SET) == (off_t)-1) {
		return ntohl(AFPERR_EOF);
	}
	if (read(fork, data, size) != size) {
		return ntohl(AFPERR_EOF);
	}
	return ntohl(AFP_OK);
}

/* ------------------------------- */
unsigned int local_copyfile(struct CONN *conn, uint16_t svol, int sdid, uint16_t dvol, int ddid, char *src, char *buf, char *dst)
{
	return ntohl(AFPERR_PARAM);
}

/* ------------------------------- */
struct vfs local_VFS = {
local_getfiledirparams,
local_createdir,
local_createfile,
local_openfork,
local_writeheader,
local_writefooter,
local_flushfork,
local_closefork,
local_delete,
local_setforkparam,
local_write,
local_read,
local_readheader,
local_readfooter,
local_copyfile,
local_openvol,
local_closevol
};


/* =============================== */
static void press_enter(char *s)
{
    if (!Interactive)
	return;

    if (s)
	fprintf(stdout, "--> Performing: %s\n", s);
    fprintf(stdout, "Press <ENTER> to continue.\n");

    while (fgetc(stdin) != '\n')
	;
}

/* ------------------------- */
void failed(void)
{
	fprintf(stdout,"\tFAILED\n");
	if (!ExitCode)
		ExitCode = 1;
}

/* ------------------------- */
void fatal_failed(void)
{
	fprintf(stdout,"\tFAILED\n");
	exit(1);
}
/* ------------------------- */
void nottested(void)
{
	fprintf(stdout,"\tNOT TESTED\n");
	if (!ExitCode)
		ExitCode = 2;
}

/* --------------------------------- */
int is_there(CONN *conn, uint16_t vol, int did, char *name)
{
	return VFS.getfiledirparams(conn, vol,  did, name,
	         (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID)
	         ,
	         (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID)
		);
}

/* ------------------ */
unsigned long long delta (void)
{
unsigned long long s, e;

	s  = Timer_start.tv_sec;
    s *= 1000000;
    s += Timer_start.tv_usec;

	e  = Timer_end.tv_sec;
    e *= 1000000;
    e += Timer_end.tv_usec;

 return e -s;
}

/* ------------------ */
static void header(void)
{
	fprintf(stdout, "run\t microsec\t  KB/s\n");
}

/* ------------------ */
static void timer_footer(void)
{
unsigned long long d;

	gettimeofday(&Timer_end, NULL);
	d = delta();
	fprintf(stdout, "%9lld\t%.2f\n", d, ((float)Size*MEGABYTE/(float)d)/KILOBYTE);
}

/* ------------------ */
void Write(void)
{
int dir = 0;
int fork = 0;
int id = getpid();
static char temp[MAXPATHLEN];
int vol = VolID;
off_t  offset;
off_t  offset_r;
off_t  written;
off_t  written_r;
size_t nbe;
size_t nbe_r;
int i;
int push;
DSI *dsi;

	fprintf(stdout, "Write quantum %d, size %lld\n", Quantum, Size);
	header();

	sprintf(temp,"WriteTest-%d", id);

	if (ntohl(AFPERR_NOOBJ) != is_there(Conn, VolID, DIRDID_ROOT, temp)) {
		nottested();
		return;
	}
	if (!(dir = VFS.createdir(Conn,vol, DIRDID_ROOT , temp))) {
		nottested();
		goto fin;
	}
	if (VFS.createfile(Conn, vol,  0, dir , "File")){
		failed();
		goto fin;
	}

	dsi = &Conn->dsi;
	for (i = 1; i <= Count; i++) {
		fork = VFS.openfork(Conn, vol, OPENFORK_DATA , (1<<FILPBIT_FNUM), dir, "File", OPENACC_WR |OPENACC_RD| OPENACC_DWR| OPENACC_DRD);
		if (!fork) {
			failed();
			goto fin1;
		}
		fprintf(stdout,"%d\t", i);
		gettimeofday(&Timer_start, NULL);
		nbe = nbe_r = Quantum;
		written = written_r = Size;
		offset = offset_r = 0;
		push = 0;
		while (written ) {
		    if (written < Quantum) {
	    	    nbe = written;
		    }
		    if (push < Request && VFS.writeheader(dsi, fork, offset, nbe, Buffer, 0)) {
				failed();
				goto fin1;
			}
			written -= nbe;
			offset += nbe;
			push++;
		    if (push >= Request) {
		    	if (written_r < Quantum) {
	    	    	nbe_r = written_r;
		    	}
				if (VFS.writefooter(dsi, fork, offset_r, nbe_r, Buffer, 0)) {
		    		failed();
		    		goto fin1;
				}
				push--;
				written_r -= nbe_r;
				offset_r += nbe_r;
		    }
		}
		while (push) {
		    if (written_r < Quantum) {
	    		nbe_r = written_r;
		    }
			if (VFS.writefooter(dsi, fork, offset_r, nbe_r, Buffer, 0)) {
		    	failed();
		    	goto fin1;
			}
			push--;
			written_r -= nbe_r;
			offset_r += nbe_r;
		}
		if (Flush && VFS.flushfork(Conn, fork)) {
			failed();
			goto fin1;
		}
		timer_footer();
		if (VFS.closefork(Conn,fork)) {
			failed();
			goto fin;
		}
		fork = 0;

		if (Delete) {
			if (VFS.delete(Conn, vol,  dir, "File") || VFS.createfile(Conn, vol,  0, dir , "File")){
				failed();
				goto fin;
			}
		}
	}

fin1:
	if (fork && VFS.closefork(Conn,fork)) {failed();}
	if (VFS.delete(Conn, vol,  dir, "File")) {failed();}
fin:
	if (VFS.delete(Conn, vol,  dir, "")) {failed();}
    fprintf(stdout, "\n");
	return;
}

/* ------------------------ */
int init_fork(int fork)
{
off_t  written;
off_t  offset;
size_t nbe;

	if (Sparse) {
		/* assume server will create a sparse file */
		if (VFS.setforkparam(Conn, fork, (1<<FILPBIT_DFLEN), Size))
			return -1;
		return 0;
	}
	nbe = Quantum;
	written = Size;
	offset = 0;
	while (written ) {
		if (written < Quantum) {
	    	nbe = written;
		}
		if (VFS.write(Conn, fork, offset, nbe, Buffer, 0 )) {
			return -1;
		}
		written -= nbe;
		offset += nbe;
	}
	if (Flush && VFS.flushfork(Conn, fork)) {
		return -1;
	}
	return 0;
}

/* ------------------ */
static int getfd(CONN *conn, int fork)
{
DSI *dsi;

	if (Local) {
		return fork;
	}
	dsi = &Conn->dsi;
	return dsi->socket;
}

#if 0
/* ------------------ */
static int blocking_mode(CONN *conn, int fork, const int mode)
{
DSI *dsi;
int adr = mode;
int ret;

    if (Local) {
    	if (mode) {
    		 adr = O_NONBLOCK;
    	}
    	ret = fcntl(fork, F_SETFL , adr);
    }
    else {
		dsi = &Conn->dsi;
		ret = ioctl(dsi->socket, FIONBIO, &adr);
	}
	return ret;
}
#endif

/* ------------------ */
void Copy(void)
{
int dir = 0;
int dir2 = 0;
int fork = 0;
int fork2 = 0;
int fork_fd;
int fork2_fd;
int id = getpid();
static char temp[MAXPATHLEN];
int vol = VolID;
int vol2 = VolID2;
off_t  written;
off_t  written_r;
off_t  written_w;
off_t  offset = 0;
off_t  offset_r = 0;
off_t  offset_w = 0;
size_t nbe;
size_t nbe_r;
size_t nbe_w;
int i;
int push;
DSI *dsi;
int max = Request;
int cnt = 0;

	dsi = &Conn->dsi;
	fprintf(stdout, "Copy qantum %d, size %lld %s\n", Quantum, Size, Sparse?"sparse file":"");
	header();

	sprintf(temp,"CopyTest-%d", id);
	if (!Filename) {
		if (ntohl(AFPERR_NOOBJ) != is_there(Conn, VolID, DIRDID_ROOT, temp)) {
			nottested();
			return;
		}
	}
	else {
		dir = DIRDID_ROOT;
		if (ntohl(AFP_OK) != is_there(Conn, VolID, dir, Filename)) {
			nottested();
			return;
		}
	}
	if (VolID != VolID2 && ntohl(AFPERR_NOOBJ) != is_there(Conn, VolID2, DIRDID_ROOT, temp)) {
		nottested();
		return;
	}

	if (!Filename && !(dir = VFS.createdir(Conn,vol, DIRDID_ROOT , temp))) {
		nottested();
		goto fin;
	}
	if (VolID == VolID2) {
		dir2 = dir;
	}
	else if (!(dir2 = VFS.createdir(Conn, vol2, DIRDID_ROOT , temp))) {
		nottested();
		goto fin;
	}
	if (!Filename) {
		strcpy(temp, "Source");
		if (VFS.createfile(Conn, vol,  0, dir , temp)){
			failed();
			goto fin;
		}
	}
	else {
		strcpy(temp, Filename);
	}
	if (VFS.createfile(Conn, vol2,  0, dir2 , "Destination")){
		failed();
		goto fin;
	}

	for (i = 1; i <= Count; i++) {
		fork = VFS.openfork(Conn, vol, OPENFORK_DATA , (1<<FILPBIT_FNUM), dir, temp, OPENACC_WR |OPENACC_RD| OPENACC_DWR| OPENACC_DRD);
		if (!fork) {
			failed();
			goto fin1;
		}
		fork2 = VFS.openfork(Conn, vol2, OPENFORK_DATA , (1<<FILPBIT_FNUM), dir2, "Destination", OPENACC_WR |OPENACC_RD| OPENACC_DWR| OPENACC_DRD);
		if (!fork2) {
			failed();
			goto fin1;
		}
		if (!Filename && (i == 1 || Delete)) {
			if (init_fork(fork)) {
				failed();
				goto fin1;
			}
		}
		fork_fd = getfd(Conn, fork);
		fork2_fd = getfd(Conn, fork2);

		fprintf(stdout,"%d\t", i);

		gettimeofday(&Timer_start, NULL);
		nbe = nbe_r = nbe_w = Quantum;
		written = written_r = written_w = Size;
		offset = offset_r = offset_w = 0;
		push = 0;
		cnt = 0;
		while (written ) {
		    if (written < Quantum) {
	    	    nbe = written;
		    }
		    cnt++;
		    if (push < max && VFS.readheader(dsi, fork, offset, nbe, Buffer)) {
		    	failed();
		    	goto fin1;
		    }
			written -= nbe;
			offset += nbe;
		    push++;
		    if (push >= max) {
		    	if (written_r < Quantum) {
	    	    	nbe_r = written_r;
		    	}
				if (VFS.readfooter(dsi, fork, offset_r, nbe_r, Buffer)) {
		    		failed();
		    		goto fin1;
				}
		    	if (VFS.writeheader(dsi, fork2, offset_r, nbe_r, Buffer, 0)) {
					failed();
					goto fin1;
				}
				if (cnt >= max*2 -1 ) {
					if (VFS.writefooter(dsi, fork2, offset_w, nbe_w, Buffer, 0)) {
		    			failed();
		    			goto fin1;
		    		}
					written_w -= nbe_w;
					offset_w += nbe_w;
				}
				push--;
				written_r -= nbe_r;
				offset_r += nbe_r;
		    }
		}
		while (push) {
		    if (written_r < Quantum) {
	    		nbe_r = written_r;
		    }
			if (VFS.readfooter(dsi, fork, offset_r, nbe_r, Buffer)) {
		    	failed();
		    	goto fin1;
			}
			if (VFS.writeheader(dsi, fork2, offset_r, nbe_r, Buffer, 0)) {
				failed();
				goto fin1;
			}
			if (VFS.writefooter(dsi, fork2, offset_w, nbe_w, Buffer, 0)) {
		    	failed();
		    	goto fin1;
			}
			push--;
			written_r -= nbe_r;
			offset_r += nbe_r;
			written_w -= nbe_w;
			offset_w += nbe_w;
		}
		for (push = 1; push < max; push++) {
		    if (written_w < Quantum) {
	    		nbe_w = written_w;
		    }
			if (VFS.writefooter(dsi, fork2, offset_w, nbe_w, Buffer, 0)) {
		    	failed();
		    	goto fin1;
			}
			written_w -= nbe_w;
			offset_w += nbe_w;
		}
		timer_footer();
		if (VFS.closefork(Conn,fork)) {
			failed();
			goto fin1;
		}
		fork = 0;
		if (VFS.closefork(Conn,fork2)) {
			failed();
			goto fin1;
		}
		fork2 = 0;

		if (Delete) {
			if (!Filename && VFS.delete(Conn, vol,  dir, temp)) {
				goto fin;
			}
			if (VFS.delete(Conn, vol2,  dir2, "Destination")) {
				goto fin;
			}

			if (!Filename && VFS.createfile(Conn, vol,  0, dir , temp)){
				failed();
				goto fin;
			}
			if (VFS.createfile(Conn, vol2,  0, dir2 , "Destination")){
				failed();
				goto fin;
			}
		}
	}

fin1:
	if (fork && VFS.closefork(Conn,fork)) {failed();}
	if (fork2 && VFS.closefork(Conn,fork2)) {failed();}
	if (!Filename && VFS.delete(Conn, vol,  dir, temp)) {failed();}
	if (VFS.delete(Conn, vol2,  dir2, "Destination")) {failed();}
fin:
	if (!Filename && dir && VFS.delete(Conn, vol,  dir, "")) {failed();}
	if (dir2 && dir2 != dir && VFS.delete(Conn, vol2,  dir2, "")) {failed();}

	return;
}

/* ------------------ */
void ServerCopy(void)
{
int dir = 0;
int dir2 = 0;
int fork = 0;
int id = getpid();
static char temp[MAXPATHLEN];
int vol = VolID;
int vol2 = VolID2;
int i;

	fprintf(stdout, "ServerCopy qantum %d, size %lld %s\n", Quantum, Size, Sparse?"sparse file":"");
	header();

	sprintf(temp,"ServerCopyTest-%d", id);
	if (ntohl(AFPERR_NOOBJ) != is_there(Conn, VolID, DIRDID_ROOT, temp)) {
		nottested();
		return;
	}
	if (VolID != VolID2 && ntohl(AFPERR_NOOBJ) != is_there(Conn, VolID2, DIRDID_ROOT, temp)) {
		nottested();
		return;
	}

	if (!(dir = VFS.createdir(Conn,vol, DIRDID_ROOT , temp))) {
		nottested();
		goto fin;
	}
	if (VolID == VolID2) {
		dir2 = dir;
	}
	else if (!(dir2 = VFS.createdir(Conn, vol2, DIRDID_ROOT , temp))) {
		nottested();
		goto fin;
	}

	if (VFS.createfile(Conn, vol,  0, dir , "Source")){
		failed();
		goto fin;
	}
	for (i = 1; i <= Count; i++) {
		fork = VFS.openfork(Conn, vol, OPENFORK_DATA , (1<<FILPBIT_FNUM), dir, "Source", OPENACC_WR |OPENACC_RD );
		if (!fork) {
			failed();
			goto fin1;
		}
		if (i == 1 || Delete) {
			if (init_fork(fork)) {
				failed();
				goto fin1;
			}
		}
		if (VFS.closefork(Conn,fork)) {
			failed();
			goto fin1;
		}
		fork = 0;
		fprintf(stdout,"%d\t", i);
		gettimeofday(&Timer_start, NULL);
		if (VFS.copyfile(Conn, vol, dir, vol2, dir2, "Source", "", "Destination")) {
			failed();
			goto fin1;
		}
		timer_footer();
		if (Delete) {
			if (VFS.delete(Conn, vol,  dir, "Source")) {
				failed();
				goto fin;
			}
			if (VFS.createfile(Conn, vol,  0, dir , "Source")){
				failed();
				goto fin;
			}
		}
		if (VFS.delete(Conn, vol2,  dir2, "Destination")) {
			failed();
			goto fin;
		}
	}

fin1:
	if (fork && VFS.closefork(Conn,fork)) {failed();}
	if (VFS.delete(Conn, vol,  dir, "Source")) {failed();}
	VFS.delete(Conn, vol2,  dir2, "Destination");
fin:
	if (dir && VFS.delete(Conn, vol,  dir, "")) {failed();}
	if (dir2 && dir2 != dir && VFS.delete(Conn, vol2,  dir2, "")) {failed();}

	return;
}

/* ------------------ */
void Read(void)
{
int dir = 0;
int fork = 0;
int id = getpid();
static char temp[MAXPATHLEN];
int vol = VolID;
off_t  written;
off_t  written_r;
off_t  offset = 0;
off_t  offset_r = 0;
size_t nbe;
size_t nbe_r;
DSI *dsi;
int i;
int push;

	fprintf(stdout, "Read qantum %d, size %lld %s\n", Quantum, Size, Sparse?"sparse file":"");
	header();

	if (!Filename) {
		sprintf(temp,"ReadTest-%d", id);

		if (ntohl(AFPERR_NOOBJ) != is_there(Conn, VolID, DIRDID_ROOT, temp)) {
			nottested();
			return;
		}

		if (!(dir = VFS.createdir(Conn,vol, DIRDID_ROOT , temp))) {
			nottested();
			goto fin;
		}
		if (VFS.createfile(Conn, vol,  0, dir , "File")){
			failed();
			goto fin;
		}
		strcpy(temp, "File");
	}
	else {
		dir = DIRDID_ROOT;
		strcpy(temp, Filename);
		if (ntohl(AFP_OK) != is_there(Conn, VolID, dir, temp)) {
			nottested();
			return;
		}
	}
	dsi = &Conn->dsi;
	for (i = 1; i <= Count; i++) {
		fork = VFS.openfork(Conn, vol, OPENFORK_DATA , (1<<FILPBIT_FNUM), dir, temp, OPENACC_WR |OPENACC_RD| OPENACC_DWR| OPENACC_DRD);
		if (!fork) {
			failed();
			goto fin1;
		}
		if (!Filename && (i == 1 || Delete)) {
			if (init_fork(fork)) {
				failed();
				goto fin1;
			}
		}
		nbe = nbe_r = Quantum;
		written = Size;
		written_r = Size;
		offset = 0;
		offset_r = 0;
		push = 0;
		fprintf(stdout,"%d\t", i);
		gettimeofday(&Timer_start, NULL);
		while (written ) {
		    if (written < Quantum) {
	    	    nbe = written;
		    }
		    if (push < Request && VFS.readheader(dsi, fork, offset, nbe, Buffer)) {
		    	failed();
		    	goto fin1;
		    }
			written -= nbe;
			offset += nbe;
		    push++;
		    if (push >= Request) {
		    	if (written_r < Quantum) {
	    	    	nbe_r = written_r;
		    	}
				if (VFS.readfooter(dsi, fork, offset_r, nbe_r, Buffer)) {
		    		failed();
		    		goto fin1;
				}
				push--;
				written_r -= nbe_r;
				offset_r += nbe_r;
		    }
		}
		while (push) {
		    if (written_r < Quantum) {
	    		nbe_r = written_r;
		    }
			if (VFS.readfooter(dsi, fork, offset_r, nbe_r, Buffer)) {
		    	failed();
		    	goto fin1;
			}
			push--;
			written_r -= nbe_r;
			offset_r += nbe_r;
		}
		timer_footer();

		if (VFS.closefork(Conn,fork)) {failed();}
		fork = 0;

		if (!Filename && Delete) {
			if (VFS.delete(Conn, vol,  dir, temp)) {
				goto fin;
			}

			if (VFS.createfile(Conn, vol,  0, dir , temp)){
				failed();
				goto fin;
			}
		}
	}

fin1:
	if (fork && VFS.closefork(Conn,fork)) {failed();}
	if (!Filename && VFS.delete(Conn, vol,  dir, temp)) {failed();}
fin:
	if (!Filename && VFS.delete(Conn, vol,  dir, "")) {failed();}

	return;
}

#define BROKEN_DL
#ifdef BROKEN_DL
int test_to_run(char *s)
{
	if (!strcmp("Write",s)) {
		return 0;
	}
	if (!strcmp("Read",s)) {
		return 1;
	}
	if (!strcmp("Copy",s)) {
		return 2;
	}
	if (!strcmp("ServerCopy",s)) {
		return 3;
	}
	return -1;
}
#endif

/* ----------- */
static void run_one(char *name)
{
char *token;
char *tp = strdup(name);
#ifdef BROKEN_DL
int  fn;
#else
char *error;
void *handle = NULL;
void (*fn)(void) = NULL;
#endif

    token = strtok(tp, ",");

#ifdef BROKEN_DL
	fn = test_to_run(token);
	if (fn == -1) {
	    nottested();
	    return;
	}
#else
	handle = dlopen (NULL, RTLD_LAZY);
    if (handle) {
		fn = dlsym(handle, token);
		if ((error = dlerror()) != NULL)  {
			fprintf (stdout, "%s\n", error);
		}
    }
    else {
        fprintf (stdout, "%s\n", dlerror());
    }
    if (!handle || !fn) {
	    nottested();
	    return;
	}
#endif
	dsi = &Conn->dsi;
	press_enter("Opening volume.");
	VolID = VFS.openvol(Conn, Vol);
	if (VolID == 0xffff) {
		nottested();
		return;
	}
	if (*Vol2) {
		VolID2 = VFS.openvol(Conn, Vol2);
		if (VolID2 == 0xffff) {
			nottested();
			return;
		}
	}
	else {
	    VolID2 = VolID;
	}
	/* check server quantum size */
	if (!Quantum) {
	    Quantum = dsi->server_quantum;
	}
	else if (Quantum > dsi->server_quantum) {
		fprintf(stdout,"\t server quantum (%d) too small\n", dsi->server_quantum);
		return;
	}
	if (Direct) {
		/* assume correctly aligned for O_DIRECT */
		Buffer = mmap(NULL, Quantum, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	}
	else {
		Buffer = malloc(Quantum);
	}
	if (!Buffer) {
		fprintf(stdout,"\t can't allocate (%d) bytes\n", Quantum);
		return;
	}

	/* loop */
	while (token ) {
		press_enter(token);
#ifdef BROKEN_DL
		switch(fn) {
		case 0:
			Write();
			break;
		case 1:
			Read();
			break;
		case 2:
			Copy();
			break;
		case 3:
			ServerCopy();
			break;
		}
#else
	    (*fn)();
#endif
	    token = strtok(NULL, ",");
#ifdef BROKEN_DL
		if (token) {
			fn = test_to_run(token);
			if (fn == -1) {
				fprintf (stdout, "%s undefined test\n", token);
			}
		}
#else
	    if (token && handle) {
			fn = dlsym(handle, token);
			if ((error = dlerror()) != NULL)  {
				fprintf (stdout, "%s\n", error);
			}
	    }
#endif
	}

#ifndef BROKEN_DL
	if (handle)
		dlclose(handle);
#endif
	VFS.closevol(Conn,VolID);
	if (*Vol2) {
		VFS.closevol(Conn,VolID2);
	}
}

/* =============================== */
void usage( char * av0 )
{
    fprintf( stdout, "usage:\t%s [-h host] [-p port] [-s vol] [-S Vol2] [-u user] [-w password] [-f test] [-c count] "
    "[-d size] [-q quantum] [-F file] [-234LvViyea] \n", av0 );
    fprintf( stdout,"\t-h\tserver host name (default localhost)\n");
    fprintf( stdout,"\t-p\tserver port (default 548)\n");
    fprintf( stdout,"\t-s\tvolume to mount (default home)\n");
    fprintf( stdout,"\t-S\tsecond volume to mount (default none)\n");
    fprintf( stdout,"\t-u\tuser name (default uid)\n");

    fprintf( stdout,"\t-L\tuse posix calls (default AFP calls)\n");
    fprintf( stdout,"\t-D\twith -L use O_DIRECT in open flags (default no)\n");

    fprintf( stdout,"\t-w\tpassword (default none)\n");
    fprintf( stdout,"\t-1\tAFP 2.1 version (default)\n");
    fprintf( stdout,"\t-2\tAFP 2.2 version\n");
    fprintf( stdout,"\t-3\tAFP 3.0 version\n");
    fprintf( stdout,"\t-4\tAFP 3.1 version\n");
    fprintf( stdout,"\t-5\tAFP 3.2 version\n");
    fprintf( stdout,"\t-6\tAFP 3.3 version\n");
    fprintf( stdout,"\t-7\tAFP 3.4 version\n");

    fprintf( stdout,"\t-c\trun test count times\n");
    fprintf( stdout,"\t-d\tfile size (Mbytes, default 64)\n");
    fprintf( stdout,"\t-q\tpacket size (Kbytes, default server quantum)\n");
    fprintf( stdout,"\t-r\tnumber of outstanding requests (default 1)\n");
    fprintf( stdout,"\t-R\tnumber of not interleaved outstanding requests (default 1)\n");
    fprintf( stdout,"\t-y\tuse a new file for each run (default same file)\n");
    fprintf( stdout,"\t-e\tsparse file (default no)\n");
    fprintf( stdout,"\t-a\tdon't flush to disk after write (default yes)\n");
    fprintf( stdout,"\t-F\tread from file in volume root folder (default create a temporary file)\n");

    fprintf( stdout,"\t-v\tverbose (default no)\n");
    fprintf( stdout,"\t-V\tvery verbose (default no)\n");

    fprintf( stdout,"\t-f\ttest to run (Read, Write, Copy, ServerCopy, default Write)  \n");
    fprintf( stdout,"\t-i\tinteractive mode, prompts before every test (debug purposes)\n");
    exit (1);
}

char *vers = "AFPVersion 2.1";
char *uam = "Cleartxt Passwrd";
/* ------------------------------- */
int main( int ac, char **av )
{
int cc;

	Quiet = 1;
    while (( cc = getopt( ac, av, "Vv1234567h:p:s:S:u:d:w:f:ic:""o:q:r:yeLDaF:R:" )) != EOF ) {
        switch ( cc ) {
		case 'd':
			Size = atoi(optarg) * MEGABYTE;
			break;
		case 'q':
			Quantum = atoi(optarg) *KILOBYTE;
			break;
		case 'r':
			Request = atoi(optarg);
			break;
		case 'R':
			Req = atoi(optarg);
			break;
		case 'y':
			Delete = 1;
			break;
		case 'e':
			Sparse = 1;
			break;
		case 'L':
			Local = 1;
			break;
		case 'D':
			Direct = 1;
			break;
		case 'a':
			Flush = 0;
			break;
		case 'c':
			Count = atoi(optarg);
			break;
		case 'F':
            Filename = strdup(optarg);
            break;

        case '1':
			vers = "AFPVersion 2.1";
			Version = 21;
			break;
        case '2':
			vers = "AFP2.2";
			Version = 22;
			break;
        case '3':
			vers = "AFPX03";
			Version = 30;
			break;
        case '4':
			vers = "AFP3.1";
			Version = 31;
			break;
        case '5':
			vers = "AFP3.2";
			Version = 32;
			break;
        case '6':
			vers = "AFP3.3";
			Version = 33;
			break;
        case '7':
			vers = "AFP3.4";
			Version = 34;
			break;
        case 'n':
            Proto = 1;
            break;
        case 'h':
            Server = strdup(optarg);
            break;
        case 's':
            Vol = strdup(optarg);
            break;
        case 'S':
            Vol2 = strdup(optarg);
            break;
        case 'u':
            User = strdup(optarg);
            break;
        case 'w':
            Password = strdup(optarg);
            break;
        case 'f' :
            Test = strdup(optarg);
            break;
        case 'p' :
            Port = atoi( optarg );
            if (Port <= 0) {
                fprintf(stdout, "Bad port.\n");
                exit(1);
            }
            break;
	case 'v':
		Quiet = 0;
		break;
	case 'V':
		Quiet = 0;
		Verbose = 1;
		break;
	case 'i':
		Interactive = 1;
		break;

        default :
            usage( av[ 0 ] );
        }
    }

	/**************************
	 Connection */

    if ((Conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
    	return 1;
    }
    Conn->type = Proto;
    if (Local) {
    	Dsi = &Conn->dsi;
		dsi = Dsi;
		dsi->server_quantum = 512* KILOBYTE;
		VFS = local_VFS;

    } else {
	    if (!Proto) {
		int sock;
    		Dsi = &Conn->dsi;
			dsi = Dsi;
		    sock = OpenClientSocket(Server, Port);
        	if ( sock < 0) {
	    		return 2;
	        }
    	 	Dsi->protocol = DSI_TCPIP;
	    	Dsi->socket = sock;
	    }
    	else {
		}

	    /* login */
    	if (Version >= 30) {
			FPopenLoginExt(Conn, vers, uam, User, Password);
		}
		else {
			FPopenLogin(Conn, vers, uam, User, Password);
		}
	}
	Conn->afp_version = Version;

	/*********************************
	*/
	if (Verbose)
		Quiet = 0;
	run_one(Test);
	if (!Local)
   		FPLogOut(Conn);

	return ExitCode;
}
