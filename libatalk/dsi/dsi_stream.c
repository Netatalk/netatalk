/*
 * Copyright (c) 1998 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 *
 * this file provides the following functions:
 * dsi_stream_write:    just write a bunch of bytes.
 * dsi_stream_read:     just read a bunch of bytes.
 * dsi_stream_send:     send a DSI header + data.
 * dsi_stream_receive:  read a DSI header + data.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <atalk/logger.h>
#include <atalk/dsi.h>
#include <netatalk/endian.h>
#include <atalk/util.h>

#define min(a,b)  ((a) < (b) ? (a) : (b))

#ifndef MSG_MORE
#define MSG_MORE 0x8000
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0x40
#endif

/*
 * afpd is sleeping too much while trying to send something.
 * May be there's no reader or the reader is also sleeping in write,
 * look if there's some data for us to read, hopefully it will wake up
 * the reader so we can write again.
 *
 * @returns 0 when is possible to send again, -1 on error
 */
static int dsi_peek(DSI *dsi)
{
    static int warned = 0;
    fd_set readfds, writefds;
    int    len;
    int    maxfd;
    int    ret;

    LOG(log_debug, logtype_dsi, "dsi_peek");

    maxfd = dsi->socket + 1;

    while (1) {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        if (dsi->eof < dsi->end) {
            /* space in read buffer */
            FD_SET( dsi->socket, &readfds);
        } else {
            if (!warned) {
                warned = 1;
                LOG(log_note, logtype_dsi, "dsi_peek: readahead buffer is full, possibly increase -dsireadbuf option");
                LOG(log_note, logtype_dsi, "dsi_peek: dsireadbuf: %d, DSI quantum: %d, effective buffer size: %d",
                    dsi->dsireadbuf,
                    dsi->server_quantum ? dsi->server_quantum : DSI_SERVQUANT_DEF,
                    dsi->end - dsi->buffer);
            }
        }

        FD_SET( dsi->socket, &writefds);

        /* No timeout: if there's nothing to read nor nothing to write,
         * we've got nothing to do at all */
        if ((ret = select( maxfd, &readfds, &writefds, NULL, NULL)) <= 0) {
            if (ret == -1 && errno == EINTR)
                /* we might have been interrupted by out timer, so restart select */
                continue;
            /* give up */
            LOG(log_error, logtype_dsi, "dsi_peek: unexpected select return: %d %s",
                ret, ret < 0 ? strerror(errno) : "");
            return -1;
        }

        if (FD_ISSET(dsi->socket, &writefds)) {
            /* we can write again */
            LOG(log_debug, logtype_dsi, "dsi_peek: can write again");
            break;
        }

        /* Check if there's sth to read, hopefully reading that will unblock the client */
        if (FD_ISSET(dsi->socket, &readfds)) {
            len = dsi->end - dsi->eof; /* it's ensured above that there's space */

            if ((len = read(dsi->socket, dsi->eof, len)) <= 0) {
                if (len == 0) {
                    LOG(log_error, logtype_dsi, "dsi_peek: EOF");
                    return -1;
                }
                LOG(log_error, logtype_dsi, "dsi_peek: read: %s", strerror(errno));
                if (errno == EAGAIN)
                    continue;
                return -1;
            }
            LOG(log_debug, logtype_dsi, "dsi_peek: read %d bytes", len);

            dsi->eof += len;
        }
    }

    return 0;
}

/* 
 * Return all bytes up to count from dsi->buffer if there are any buffered there
 */
static size_t from_buf(DSI *dsi, u_int8_t *buf, size_t count)
{
    size_t nbe = 0;

    if (dsi->buffer == NULL)
        /* afpd master has no DSI buffering */
        return 0;

    LOG(log_maxdebug, logtype_dsi, "from_buf: %u bytes", count);
    
    nbe = dsi->eof - dsi->start;

    if (nbe > 0) {
        nbe = min((size_t)nbe, count);
        memcpy(buf, dsi->start, nbe);
        dsi->start += nbe;

        if (dsi->eof == dsi->start)
            dsi->start = dsi->eof = dsi->buffer;
    }

    LOG(log_debug, logtype_dsi, "from_buf(read: %u, unread:%u , space left: %u): returning %u",
        dsi->start - dsi->buffer, dsi->eof - dsi->start, dsi->end - dsi->eof, nbe);

    return nbe;
}

/*
 * Get bytes from buffer dsi->buffer or read from socket
 *
 * 1. Check if there are bytes in the the dsi->buffer buffer.
 * 2. Return bytes from (1) if yes.
 *    Note: this may return fewer bytes then requested in count !!
 * 3. If the buffer was empty, read from the socket.
 */
static ssize_t buf_read(DSI *dsi, u_int8_t *buf, size_t count)
{
    ssize_t len;

    LOG(log_maxdebug, logtype_dsi, "buf_read(%u bytes)", count);

    if (!count)
        return 0;

    len = from_buf(dsi, buf, count); /* 1. */
    if (len)
        return len;             /* 2. */
  
    len = readt(dsi->socket, buf, count, 0, 1); /* 3. */

    LOG(log_maxdebug, logtype_dsi, "buf_read(%u bytes): got: %d", count, len);

    return len;
}

/*
 * Get "length" bytes from buffer and/or socket. In order to avoid frequent small reads
 * this tries to read larger chunks (8192 bytes) into a buffer.
 */
static size_t dsi_buffered_stream_read(DSI *dsi, u_int8_t *data, const size_t length)
{
  size_t len;
  size_t buflen;

  LOG(log_maxdebug, logtype_dsi, "dsi_buffered_stream_read: %u bytes", length);
  
  len = from_buf(dsi, data, length); /* read from buffer dsi->buffer */
  dsi->read_count += len;
  if (len == length) {          /* got enough bytes from there ? */
      return len;               /* yes */
  }

  /* fill the buffer with 8192 bytes or until buffer is full */
  buflen = min(8192, dsi->end - dsi->eof);
  if (buflen > 0) {
      ssize_t ret;
      ret = read(dsi->socket, dsi->eof, buflen);
      if (ret > 0)
          dsi->eof += ret;
  }

  /* now get the remaining data */
  if ((buflen = dsi_stream_read(dsi, data + len, length - len)) != length - len)
      return 0;
  len += buflen;

  return len;
}

/* ---------------------------------------
*/
static void block_sig(DSI *dsi)
{
  dsi->in_write++;
}

/* ---------------------------------------
*/
static void unblock_sig(DSI *dsi)
{
  dsi->in_write--;
}

/*********************************************************************************
 * Public functions
 *********************************************************************************/

/*!
 * Communication error with the client, enter disconnected state
 *
 * 1. close the socket
 * 2. set the DSI_DISCONNECTED flag, remove possible sleep flags
 *
 * @returns  0 if successfully entered disconnected state
 *          -1 if ppid is 1 which means afpd master died
 *             or euid == 0 ie where still running as root (unauthenticated session)
 */
int dsi_disconnect(DSI *dsi)
{
    LOG(log_note, logtype_dsi, "dsi_disconnect: entering disconnected state");
    dsi->proto_close(dsi);          /* 1 */
    dsi->flags &= ~(DSI_SLEEPING | DSI_EXTSLEEP); /* 2 */
    dsi->flags |= DSI_DISCONNECTED;
    if (geteuid() == 0)
        return -1;
    return 0;
}

/* ------------------------------
 * write raw data. return actual bytes read. checks against EINTR
 * aren't necessary if all of the signals have SA_RESTART
 * specified. */
ssize_t dsi_stream_write(DSI *dsi, void *data, const size_t length, int mode)
{
  size_t written;
  ssize_t len;
  unsigned int flags = 0;

  dsi->in_write++;
  written = 0;

  LOG(log_maxdebug, logtype_dsi, "dsi_stream_write: sending %u bytes", length);

  if (dsi->flags & DSI_DISCONNECTED)
      return -1;

  while (written < length) {
      len = send(dsi->socket, (u_int8_t *) data + written, length - written, flags);
      if (len >= 0) {
          written += len;
          continue;
      }

      if (errno == EINTR)
          continue;

      if (errno == EAGAIN || errno == EWOULDBLOCK) {
          LOG(log_debug, logtype_dsi, "dsi_stream_write: send: %s", strerror(errno));

          if (mode == DSI_NOWAIT && written == 0) {
              /* DSI_NOWAIT is used by attention give up in this case. */
              written = -1;
              goto exit;
          }

          /* Try to read sth. in order to break up possible deadlock */
          if (dsi_peek(dsi) != 0) {
              written = -1;
              goto exit;
          }
          /* Now try writing again */
          continue;
      }

      LOG(log_error, logtype_dsi, "dsi_stream_write: %s", strerror(errno));
      written = -1;
      goto exit;
  }

  dsi->write_count += written;

exit:
  dsi->in_write--;
  return written;
}


/* ---------------------------------
*/
#ifdef WITH_SENDFILE
ssize_t dsi_stream_read_file(DSI *dsi, int fromfd, off_t offset, const size_t length)
{
  size_t written;
  ssize_t len;

  LOG(log_maxdebug, logtype_dsi, "dsi_stream_read_file: sending %u bytes", length);

  if (dsi->flags & DSI_DISCONNECTED)
      return -1;

  dsi->in_write++;
  written = 0;

  while (written < length) {
    len = sys_sendfile(dsi->socket, fromfd, &offset, length - written);
        
    if (len < 0) {
      if (errno == EINTR)
          continue;
      if (errno == EINVAL || errno == ENOSYS)
          return -1;
          
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
          if (dsi_peek(dsi)) {
              /* can't go back to blocking mode, exit, the next read
                 will return with an error and afpd will die.
              */
              break;
          }
          continue;
      }
      LOG(log_error, logtype_dsi, "dsi_stream_read_file: %s", strerror(errno));
      break;
    }
    else if (!len) {
        /* afpd is going to exit */
        errno = EIO;
        return -1; /* I think we're at EOF here... */
    }
    else 
        written += len;
  }

  dsi->write_count += written;
  dsi->in_write--;
  return written;
}
#endif


/*
 * Essentially a loop around buf_read() to ensure "length" bytes are read
 * from dsi->buffer and/or the socket.
 *
 * @returns length on success, some value smaller then length indicates an error
 */
size_t dsi_stream_read(DSI *dsi, void *data, const size_t length)
{
  size_t stored;
  ssize_t len;

  if (dsi->flags & DSI_DISCONNECTED)
      return 0;

  LOG(log_maxdebug, logtype_dsi, "dsi_stream_read(%u bytes)", length);

  stored = 0;
  while (stored < length) {
      len = buf_read(dsi, (u_int8_t *) data + stored, length - stored);
      if (len == -1 && (errno == EINTR || errno == EAGAIN)) {
          LOG(log_maxdebug, logtype_dsi, "dsi_stream_read: select read loop");
          continue;
      } else if (len > 0) {
          stored += len;
      } else { /* eof or error */
          /* don't log EOF error if it's just after connect (OSX 10.3 probe) */
#if 0
          if (errno == ECONNRESET)
              dsi->flags |= DSI_GOT_ECONNRESET;
#endif
          if (len || stored || dsi->read_count) {
              if (! (dsi->flags & DSI_DISCONNECTED)) {
                  LOG(log_error, logtype_dsi, "dsi_stream_read: len:%d, %s",
                      len, (len < 0) ? strerror(errno) : "unexpected EOF");
              }
              return 0;
          }
          break;
      }
  }

  dsi->read_count += stored;

  LOG(log_maxdebug, logtype_dsi, "dsi_stream_read(%u bytes): got: %u", length, stored);
  return stored;
}

/* ---------------------------------------
 * write data. 0 on failure. this assumes that dsi_len will never
 * cause an overflow in the data buffer. 
 */
int dsi_stream_send(DSI *dsi, void *buf, size_t length)
{
  char block[DSI_BLOCKSIZ];
  struct iovec iov[2];
  size_t towrite;
  ssize_t len;

  LOG(log_maxdebug, logtype_dsi, "dsi_stream_send: %u bytes",
      length ? length : sizeof(block));

  if (dsi->flags & DSI_DISCONNECTED)
      return 0;

  block[0] = dsi->header.dsi_flags;
  block[1] = dsi->header.dsi_command;
  memcpy(block + 2, &dsi->header.dsi_requestID, 
	 sizeof(dsi->header.dsi_requestID));
  memcpy(block + 4, &dsi->header.dsi_code, sizeof(dsi->header.dsi_code));
  memcpy(block + 8, &dsi->header.dsi_len, sizeof(dsi->header.dsi_len));
  memcpy(block + 12, &dsi->header.dsi_reserved,
	 sizeof(dsi->header.dsi_reserved));

  if (!length) { /* just write the header */
    length = (dsi_stream_write(dsi, block, sizeof(block), 0) == sizeof(block));
    return length; /* really 0 on failure, 1 on success */
  }
  
  /* block signals */
  block_sig(dsi);
  iov[0].iov_base = block;
  iov[0].iov_len = sizeof(block);
  iov[1].iov_base = buf;
  iov[1].iov_len = length;
  
  towrite = sizeof(block) + length;
  dsi->write_count += towrite;
  while (towrite > 0) {
      if (((len = writev(dsi->socket, iov, 2)) == -1 && errno == EINTR) || (len == 0))
          continue;
    
      if ((size_t)len == towrite) /* wrote everything out */
          break;
      else if (len < 0) { /* error */
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
              if (!dsi_peek(dsi)) {
                  continue;
              }
          }
          LOG(log_error, logtype_dsi, "dsi_stream_send: %s", strerror(errno));
          unblock_sig(dsi);
          return 0;
      }
    
      towrite -= len;
      if (towrite > length) { /* skip part of header */
          iov[0].iov_base = (char *) iov[0].iov_base + len;
          iov[0].iov_len -= len;
      } else { /* skip to data */
          if (iov[0].iov_len) {
              len -= iov[0].iov_len;
              iov[0].iov_len = 0;
          }
          iov[1].iov_base = (char *) iov[1].iov_base + len;
          iov[1].iov_len -= len;
      }
  }
  
  unblock_sig(dsi);
  return 1;
}


/* ---------------------------------------
 * read data. function on success. 0 on failure. data length gets
 * stored in length variable. this should really use size_t's, but
 * that would require changes elsewhere. */
int dsi_stream_receive(DSI *dsi, void *buf, const size_t ilength,
		       size_t *rlength)
{
  char block[DSI_BLOCKSIZ];

  LOG(log_maxdebug, logtype_dsi, "dsi_stream_receive: %u bytes", ilength);

  if (dsi->flags & DSI_DISCONNECTED)
      return 0;

  /* read in the header */
  if (dsi_buffered_stream_read(dsi, (u_int8_t *)block, sizeof(block)) != sizeof(block)) 
    return 0;

  dsi->header.dsi_flags = block[0];
  dsi->header.dsi_command = block[1];
  /* FIXME, not the right place, 
     but we get a server disconnect without reason in the log
  */
  if (!block[1]) {
      LOG(log_error, logtype_dsi, "dsi_stream_receive: invalid packet, fatal");
      return 0;
  }

  memcpy(&dsi->header.dsi_requestID, block + 2, 
	 sizeof(dsi->header.dsi_requestID));
  memcpy(&dsi->header.dsi_code, block + 4, sizeof(dsi->header.dsi_code));
  memcpy(&dsi->header.dsi_len, block + 8, sizeof(dsi->header.dsi_len));
  memcpy(&dsi->header.dsi_reserved, block + 12,
	 sizeof(dsi->header.dsi_reserved));
  dsi->clientID = ntohs(dsi->header.dsi_requestID);
  
  /* make sure we don't over-write our buffers. */
  *rlength = min(ntohl(dsi->header.dsi_len), ilength);
  if (dsi_stream_read(dsi, buf, *rlength) != *rlength) 
    return 0;

  return block[1];
}
