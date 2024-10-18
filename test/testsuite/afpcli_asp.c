#include "afpclient.h"

/* -------------------------------------------- */
int OpenClientSocket(char* host,int port)
{
int sock;
struct sockaddr_in server;
struct hostent* hp;
int attr;

	server.sin_family=AF_INET;
	server.sin_port=htons((unsigned short)port);

	hp=gethostbyname(host);
 	if(!hp) {
    unsigned long int addr=inet_addr(host);
    	if(addr!=-1)
       		hp=gethostbyaddr((char*)addr,sizeof(addr),AF_INET);

    	if(!hp) {
       		fprintf(stdout,"Unknown host '%s' for server [%s]\n.",host);
       		return(-1);
      	}
   	}

 	memcpy((char*)&server.sin_addr,(char*)hp->h_addr,sizeof(server.sin_addr));

 	sock=socket(PF_INET,SOCK_STREAM,0);
 	if(sock==-1) {
    	fprintf(stdout,"Failed to create client sockets.\n");
    	return(-1);
   	}
    attr = 1;
    setsockopt(sock, SOL_TCP, TCP_NODELAY, &attr, sizeof(attr));

 	if(connect(sock ,(struct sockaddr*)&server,sizeof(server))==-1) {
    	close(sock);
    	sock=-1;
    	fprintf(stdout,"Failed to connect socket.\n");
   	}

	 return(sock);
}
/* -------------------------------------------------------*/
/* read raw data. return actual bytes read. this will wait until
 * it gets length bytes */
size_t my_asp_stream_read(ASP *asp, void *data, const size_t length)
{
  size_t stored;
  ssize_t len;

  stored = 0;
  while (stored < length) {
    if ((len = read(asp->socket, (uint8_t *) data + stored,
                    length - stored)) == -1 && errno == EINTR)
      continue;

    if (len > 0)
      stored += len;
    else {/* eof or error */
      fprintf(stdout, "asp_stream_read(%d): %s\n", len, (len < 0)?strerror(errno):"EOF");
      break;
    }
  }

  asp->read_count += stored;
  return stored;
}

/* -------------------------------------------------------*/
/* read data. function on success. 0 on failure. data length gets
 * stored in length variable. this should really use size_t's, but
 * that would require changes elsewhere. */
int my_asp_stream_receive(ASP *asp, void *buf, const int ilength,
                       int *rlength)
{
  char block[DSI_BLOCKSIZ];

  /* read in the header */
  if (my_asp_stream_read(asp, block, sizeof(block)) != sizeof(block))
    return 0;

  asp->header.asp_flags = block[0];
  asp->header.asp_command = block[1];
  memcpy(&asp->header.asp_requestID, block + 2, sizeof(asp->header.asp_requestID));
  memcpy(&asp->header.asp_code, block + 4, sizeof(asp->header.asp_code));
  memcpy(&asp->header.asp_len, block + 8, sizeof(asp->header.asp_len));
  memcpy(&asp->header.asp_reserved, block + 12,sizeof(asp->header.asp_reserved));
  asp->serverID = ntohs(asp->header.asp_requestID);

  /* make sure we don't over-write our buffers. */
  *rlength = min(ntohl(asp->header.asp_len), ilength);
  if (my_asp_stream_read(asp, buf, *rlength) != *rlength)
    return 0;

  return block[1];
}

/* ======================================================= */
size_t my_asp_stream_write(ASP *asp, void *data, const size_t length)
{
size_t written;
ssize_t len;

  written = 0;
  while (written < length) {
    if (((len = write(asp->socket, (uint8_t *) data + written,
		      length - written)) == -1 && errno == EINTR) ||
	!len)
      continue;

    if (len < 0) {
      fprintf(stdout, "asp_stream_write: %s\n", strerror(errno));
      break;
    }

    written += len;
  }

  asp->write_count += written;
  return written;
}

/* --------------------------------------------------- */
/* write data. 0 on failure. this assumes that asp_len will never
 * cause an overflow in the data buffer. */
int my_asp_stream_send(ASP *asp, void *buf, size_t length)
{
  char block[DSI_BLOCKSIZ];
  sigset_t oldset;

  block[0] = asp->header.asp_flags;
  block[1] = asp->header.asp_command;

  memcpy(block + 2, &asp->header.asp_requestID, sizeof(asp->header.asp_requestID));
  memcpy(block + 4, &asp->header.asp_code, sizeof(asp->header.asp_code));
  memcpy(block + 8, &asp->header.asp_len, sizeof(asp->header.asp_len));
  memcpy(block + 12, &asp->header.asp_reserved, sizeof(asp->header.asp_reserved));

  if (!length) { /* just write the header */
    length = (my_asp_stream_write(asp, block, sizeof(block)) == sizeof(block));
    return length; /* really 0 on failure, 1 on success */
  }

  /* write the header then data */
  if ((my_asp_stream_write(asp, block, sizeof(block)) != sizeof(block)) ||
      (my_asp_stream_write(asp, buf, length) != length)) {
    return 0;
  }

  return 1;
}

/* -------------------------------------
   OK
*/
void my_asp_tickle(ASP asp, const uint8_t sid, struct sockaddr_at *sat)
{
  struct atp_block atpb;
  char buf[ASP_HDRSIZ];

  buf[ 0 ] = ASPFUNC_TICKLE;
  buf[ 1 ] = sid;
  buf[ 2 ] = buf[ 3 ] = 0;

  atpb.atp_saddr = sat;
  atpb.atp_sreqdata = buf;
  atpb.atp_sreqdlen = sizeof(buf);
  atpb.atp_sreqto = 0;
  atpb.atp_sreqtries = 1;
  if ( atp_sreq( asp->asp_atp, &atpb, 0, 0 ) < 0 ) {
  }
}

/* ------------------------------------- */
int my_asp_receive(ASP *x)
{
int ret;

	while (1) {
		ret = my_asp_stream_receive(x, x->commands, DSI_CMDSIZ, &x->cmdlen);
		if (ret == DSIFUNC_ATTN) {
			continue;
		}
		else if (ret == DSIFUNC_TICKLE) {
			my_asp_tickle(x);
		}
		else
			break;
	}
	return ret;
}

/* ------------------------------- */
static void SendInit(ASP *asp)
{
	memset(asp->commands, 0, DSI_CMDSIZ);
	asp->header.asp_flags = DSIFL_REQUEST;
	asp->header.asp_command = DSIFUNC_CMD;
	asp->header.asp_requestID = htons(asp_clientID(asp));
}

/* ------------------------------- */
static int  SendCmd(ASP *asp, int cmd)
{
int ofs;

	SendInit(asp);
	ofs = 0;
	asp->commands[ofs++] = cmd;

	asp->datalen = ofs;
	asp->header.asp_len = htonl(asp->datalen);
	asp->header.asp_code = 0; // htonl(err);

   	my_asp_stream_send(asp, asp->commands, asp->datalen);
}

/* ------------------------------- */
static int  SendCmdWithU16(ASP *asp, int cmd, uint16_t param)
{
int ofs;

	SendInit(asp);
	ofs = 0;
	asp->commands[ofs++] = cmd;
	asp->commands[ofs++] = 0;
	memcpy(asp->commands +ofs, &param, sizeof(param));
	ofs += sizeof(param);

	asp->datalen = ofs;
	asp->header.asp_len = htonl(asp->datalen);
	asp->header.asp_code = 0; // htonl(err);

   	my_asp_stream_send(asp, asp->commands, asp->datalen);

}

/* ==============================================
	spec violation in netatalk
	FPlogout ==> aspclose
*/
int AFPopenLogin(ASP *asp, char *vers, char *uam, char *usr, char *pwd)
{
uint32_t i = 0;
uint8_t len;
int ofs;

	/* DSIOpenSession */
	asp->header.asp_flags = DSIFL_REQUEST;
	asp->header.asp_command = DSIFUNC_OPEN;
	asp->header.asp_requestID = htons(asp_clientID(asp));

	asp->cmdlen = 2 + sizeof(i);
	asp->commands[0] = DSIOPT_ATTNQUANT;
  	asp->commands[1] = sizeof(i);
  	i = htonl(DSI_DEFQUANT);
  	memcpy(asp->commands + 2, &i, sizeof(i));
	my_asp_send(asp);
	/* -------------- */
	my_asp_receive(asp);
#if 0
	dump_header(asp);
	if (asp->header.asp_command == DSIFUNC_OPEN) {
		dump_open(asp);
	}
#endif
	/* -------------- */
	SendInit(asp);
	ofs = 0;
	asp->commands[ofs++] = AFP_LOGIN;

	len = strlen(vers);
	asp->commands[ofs++] = len;
	strncpy(&asp->commands[ofs], vers, len);
	ofs += len;

	len = strlen(uam);
	asp->commands[ofs++] = len;
	strncpy(&asp->commands[ofs], uam, len);
	ofs += len;

	len = strlen(usr);
	if (len) {
		asp->commands[ofs++] = len;
		strncpy(&asp->commands[ofs], usr, len);
		ofs += len;

		len = strlen(pwd);
		if (ofs & 1) {
			asp->commands[ofs++] = 0;
		}

		strncpy(&asp->commands[ofs], pwd, len);
		ofs += PASSWDLEN;
	}
	asp->datalen = ofs;
	asp->header.asp_len = htonl(asp->datalen);
	asp->header.asp_code = 0; // htonl(err);

   	my_asp_stream_send(asp, asp->commands, asp->datalen);
	/* ------------------ */
	my_asp_receive(asp);
	return(asp->header.asp_code);
}

/* --------------------------- */
int AFPChangePW(ASP *asp, char *uam, char *usr, char *opwd, char *pwd)
{
uint32_t i = 0;
uint8_t len;
int ofs;

	SendInit(asp);
	ofs = 0;
	asp->commands[ofs++] = AFP_CHANGEPW;
	asp->commands[ofs++] = 0;

	len = strlen(uam);
	asp->commands[ofs++] = len;
	strncpy(&asp->commands[ofs], uam, len);
	ofs += len;
	if (ofs & 1) {
		asp->commands[ofs++] = 0;
	}

	len = strlen(usr);
	if (len) {
		asp->commands[ofs++] = len;
		strncpy(&asp->commands[ofs], usr, len);
		ofs += len;

		len = strlen(opwd);
		if (ofs & 1) {
			asp->commands[ofs++] = 0;
		}

		strncpy(&asp->commands[ofs], opwd, len);
		ofs += PASSWDLEN;

		len = strlen(pwd);

		strncpy(&asp->commands[ofs], pwd, len);
		ofs += PASSWDLEN;
	}
	asp->datalen = ofs;
	asp->header.asp_len = htonl(asp->datalen);
	asp->header.asp_code = 0; // htonl(err);

   	my_asp_stream_send(asp, asp->commands, asp->datalen);
	/* ------------------ */
	my_asp_receive(asp);
	return(asp->header.asp_code);
}
/* ------------------------------- */
int AFPLogOut(ASP *asp)
{
	SendCmd(asp,AFP_LOGOUT);
	my_asp_receive(asp);
	return(asp->header.asp_code);
}

/* ------------------------------- */
int AFPGetSrvrInfo(ASP *asp)
{
	SendCmd(asp,AFP_GETSRVINFO);
	my_asp_receive(asp);
	return(asp->header.asp_code);
}

/* ------------------------------- */
int AFPGetSrvrParam(ASP *asp)
{
	SendCmd(asp,AFP_GETSRVPARAM);
	my_asp_receive(asp);
	return(asp->header.asp_code);
}

/* -------------------------------  */
int AFPCloseVol(ASP *asp, uint16_t vol)
{
	SendCmdWithU16(asp, AFP_CLOSEVOL, vol);
	my_asp_receive(asp);
	return(asp->header.asp_code);
}

/* -------------------------------  */
int AFPCloseDT(ASP *asp, uint16_t vol)
{
	SendCmdWithU16(asp, AFP_CLOSEDT, vol);
	my_asp_receive(asp);
	return(asp->header.asp_code);
}

/* ------------------------------- */
int AFPCloseFork(ASP *asp, uint16_t vol)
{
	SendCmdWithU16(asp, AFP_CLOSEFORK, vol);
	my_asp_receive(asp);
	return(asp->header.asp_code);
}

/* ------------------------------- */
int AFPFlush(ASP *asp, uint16_t vol)
{
	SendCmdWithU16(asp, AFP_FLUSH, vol);
	my_asp_receive(asp);
	return(asp->header.asp_code);
}

/* -------------------------------  */
int AFPFlushFork(ASP *asp, uint16_t vol)
{
	SendCmdWithU16(asp, AFP_FLUSHFORK, vol);
	my_asp_receive(asp);
	return(asp->header.asp_code);
}

/* -------------------------------  */
uint16_t AFPOpenVol(ASP *asp, char *vol)
{
int ofs;
uint16_t bitmap,result, volID = 0;
int len;

	SendInit(asp);
	ofs = 0;
	asp->commands[ofs++] = AFP_OPENVOL;
	asp->commands[ofs++] = 0;

 	bitmap = htons( (1<<VOLPBIT_VID) );
  	memcpy(asp->commands + ofs, &bitmap, sizeof(bitmap));
	ofs += 2;

	len = strlen(vol);
	asp->commands[ofs++] = len;
	strncpy(&asp->commands[ofs], vol, len);
	ofs += len;
    if ((len + 1) & 1) /* pad to an even boundary */
        ofs++;

	asp->datalen = ofs;
	asp->header.asp_len = htonl(asp->datalen);
	asp->header.asp_code = 0; // htonl(err);

   	my_asp_stream_send(asp, asp->commands, asp->datalen);
	/* ------------------ */
	my_asp_receive(asp);
	if (!asp->header.asp_code) {
		ofs = 0;
		memcpy(&result, asp->commands, sizeof(result));
		ofs += sizeof(result);
		memcpy(&result, asp->commands +ofs, sizeof(result));
		volID = result;
	}
	return(volID);
}

/* ---------------------
	FIXME for now, replace only / with 0. Should deal with ..
*/
static void u2mac(uint8_t *dst, char *name, int len)
{
	while (len) {
		if (*name == '/') {
			*dst = 0;
		}
		else {
			*dst = *name;
		}
		dst++;
		name++;
		len--;
	}
}

/* ------------------------- */
int  AFPCreateFile(ASP *asp, uint16_t vol, char type, int did , char *name)
{
int ofs;
int len;

	SendInit(asp);
	ofs = 0;
	asp->commands[ofs++] = AFP_CREATEFILE;
	asp->commands[ofs++] = type;

	memcpy(asp->commands +ofs, &vol, sizeof(vol));	/* volume */
	ofs += sizeof(vol);

	memcpy(asp->commands +ofs, &did, sizeof(did));  /* directory did */
	ofs += sizeof(did);

	asp->commands[ofs++] = 2;		/* long name */
	len = strlen(name);
	asp->commands[ofs++] = len;
	u2mac(&asp->commands[ofs], name, len);
	ofs += len;

	asp->datalen = ofs;
	asp->header.asp_len = htonl(asp->datalen);
	asp->header.asp_code = 0; // htonl(err);

   	my_asp_stream_send(asp, asp->commands, asp->datalen);
	/* ------------------ */
	my_asp_receive(asp);
	return(asp->header.asp_code);
}


/* ------------------------------- */
int AFPWrite(ASP *asp, uint16_t fork, int offset, int size, char *data, char whence)
{
int ofs;
int rsize;

	SendInit(asp);
	ofs = 0;
	asp->commands[ofs++] = AFP_WRITE;
	asp->commands[ofs++] = whence;			/* 0 SEEK_SET, 0x80 SEEK_END */

	memcpy(asp->commands +ofs, &fork, sizeof(fork));	/* fork num */
	ofs += sizeof(fork);

	offset = htonl(offset);
	memcpy(asp->commands +ofs, &offset, sizeof(offset));
	ofs += sizeof(offset);

	rsize = htonl(size);
	memcpy(asp->commands +ofs, &rsize, sizeof(rsize));
	ofs += sizeof(rsize);

	asp->datalen = ofs;		// 12
	asp->header.asp_len = htonl(asp->datalen +size);
	asp->header.asp_code = htonl(ofs); // htonl(err);

   	my_asp_stream_send(asp, asp->commands, ofs);
	my_asp_stream_write(asp, data, size);
	my_asp_receive(asp);
	return(asp->header.asp_code);
}


/* -------------------------------
	type : ressource or data
*/
uint16_t  AFPOpenFork(ASP *asp, uint16_t vol, char type, uint16_t bitmap, int did , char *name,uint16_t access)
{
int ofs;
int len;
uint16_t ofork = 0, result;

	SendInit(asp);
	ofs = 0;
	asp->commands[ofs++] = AFP_OPENFORK;
	asp->commands[ofs++] = type;

	memcpy(asp->commands +ofs, &vol, sizeof(vol));	/* volume */
	ofs += sizeof(vol);

	memcpy(asp->commands +ofs, &did, sizeof(did));  /* directory did */
	ofs += sizeof(did);

	bitmap = htons(bitmap);
	memcpy(asp->commands +ofs, &bitmap, sizeof(bitmap));  /* bitmap */
	ofs += sizeof(bitmap);

	access =  htons (access  );
	memcpy(asp->commands +ofs, &access, sizeof(access));  /* access right */
	ofs += sizeof(access);

	asp->commands[ofs++] = 2;		/* long name */
	len = strlen(name);
	asp->commands[ofs++] = len;
	u2mac(&asp->commands[ofs], name, len);
	ofs += len;

	asp->datalen = ofs;
	asp->header.asp_len = htonl(asp->datalen);
	asp->header.asp_code = 0; // htonl(err);

   	my_asp_stream_send(asp, asp->commands, asp->datalen);
	/* ------------------ */
	my_asp_receive(asp);
	if (!asp->header.asp_code) {
		ofs = 0;
		memcpy(&result, asp->commands, sizeof(result));			/* bitmap */
		ofs += sizeof(result);
		memcpy(&result, asp->commands +ofs, sizeof(result));
		ofork = result;
	}
	return ofork;
}

/* ------------------------------- */
int AFPDelete(ASP *asp, uint16_t vol, int did , char *name)
{
int ofs;
uint16_t bitmap;
int len;

	SendInit(asp);
	ofs = 0;
	asp->commands[ofs++] = AFP_DELETE;
	asp->commands[ofs++] = 0;

	memcpy(asp->commands +ofs, &vol, sizeof(vol));
	ofs += sizeof(vol);

	memcpy(asp->commands +ofs, &did, sizeof(did));
	ofs += sizeof(did);

	asp->commands[ofs++] = 2;		/* long name */
	len = strlen(name);
	asp->commands[ofs++] = len;
	u2mac(&asp->commands[ofs], name, len);
	ofs += len;

	asp->datalen = ofs;
	asp->header.asp_len = htonl(asp->datalen);
	asp->header.asp_code = 0; // htonl(err);

   	my_asp_stream_send(asp, asp->commands, asp->datalen);
	/* ------------------ */
	my_asp_receive(asp);

	return(asp->header.asp_code);
}

/* ------------------------------- */
int AFPRead(ASP *asp, uint16_t fork, int offset, int size, char *data)
{
int ofs;
int rsize;

	SendInit(asp);
	ofs = 0;
	asp->commands[ofs++] = AFP_READ;
	asp->commands[ofs++] = 0;

	memcpy(asp->commands +ofs, &fork, sizeof(fork));	/* fork num */
	ofs += sizeof(fork);

	offset = htonl(offset);
	memcpy(asp->commands +ofs, &offset, sizeof(offset));
	ofs += sizeof(offset);

	size = htonl(size);
	memcpy(asp->commands +ofs, &size, sizeof(size));
	ofs += sizeof(size);
	asp->commands[ofs++] = 0;	/* NewLineMask */
	asp->commands[ofs++] = 0;	/* NewLineChar */

	asp->datalen = ofs;
	asp->header.asp_len = htonl(asp->datalen);
	asp->header.asp_code = 0; // htonl(err);

   	my_asp_stream_send(asp, asp->commands, asp->datalen);
	/* ------------------ */
#if 0
	my_asp_stream_receive(asp, asp->data, DSI_DATASIZ, &asp->datalen);
	dump_header(asp);
	rsize =  ntohl(asp->header.asp_len);
	rsize -= asp->datalen;
	while (rsize > 0) {
	int len = min(rsize, DSI_DATASIZ);
		if (my_asp_stream_read(asp, asp->data, len) != len) {
			break;
		}
		rsize -= len;
	}
#endif
	my_asp_receive(asp);
	memcpy(data, asp->commands, asp->cmdlen);
	rsize =  ntohl(asp->header.asp_len);
	rsize -= asp->cmdlen;
	my_asp_stream_read(asp, data +asp->cmdlen, rsize);
	return(asp->header.asp_code);
}

/* -------------------------------- */
int  AFPCreateDir(ASP *asp, uint16_t vol, int did , char *name)
{
int ofs;
int len;
int dir = 0;

	SendInit(asp);
	ofs = 0;
	asp->commands[ofs++] = AFP_CREATEDIR;
	asp->commands[ofs++] = 0;

	memcpy(asp->commands +ofs, &vol, sizeof(vol));
	ofs += sizeof(vol);

	memcpy(asp->commands +ofs, &did, sizeof(did));
	ofs += sizeof(did);

	asp->commands[ofs++] = 2;		/* long name */
	len = strlen(name);
	asp->commands[ofs++] = len;
	u2mac(&asp->commands[ofs], name, len);
	ofs += len;

	asp->datalen = ofs;
	asp->header.asp_len = htonl(asp->datalen);
	asp->header.asp_code = 0; // htonl(err);

   	my_asp_stream_send(asp, asp->commands, asp->datalen);
	/* ------------------ */
	my_asp_receive(asp);
	if (!asp->header.asp_code) {
		ofs = 0;
		memcpy(&dir, asp->commands, sizeof(dir));			/* did */
		ofs += sizeof(dir);
		fprintf(stdout,"directory ID %d\n", ntohl(dir));
	}
	return(dir);
}

/* ------------------------------- */
uint16_t  AFPGetForkParam(ASP *asp, uint16_t fork, uint16_t bitmap)
{
int ofs;
uint16_t result;

	SendInit(asp);
	ofs = 0;
	asp->commands[ofs++] = AFP_GETFORKPARAM;
	asp->commands[ofs++] = 0;

	memcpy(asp->commands +ofs, &fork, sizeof(fork));	/* fork num */
	ofs += sizeof(fork);

	bitmap = htons(bitmap);
	memcpy(asp->commands +ofs, &bitmap, sizeof(bitmap));  /* bitmap */
	ofs += sizeof(bitmap);

	asp->datalen = ofs;
	asp->header.asp_len = htonl(asp->datalen);
	asp->header.asp_code = 0; // htonl(err);

   	my_asp_stream_send(asp, asp->commands, asp->datalen);
	/* ------------------ */
	my_asp_receive(asp);
	if (!asp->header.asp_code) {
	}
	return 0;
}
