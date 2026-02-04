/*
 * Copyright (c) 2026, Andy Lemin (andylemin)
 * Credits; Based on work by Netatalk contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "speedtest_local_vfs.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "afpcmd.h"

/* For compiling on OS X and platforms without O_DIRECT */
#ifndef O_DIRECT
/* XXX hack */
#define O_DIRECT 040000
#endif

/* Exported global variables */
char *Dir_heap[MAXVOL][MAXDIR];
char *Vol_heap[MAXVOL];
int Local_VFS_Quiet = 0;
int Local_VFS_Direct = 1;

/* Note: Many Local VFS functions have CONN/DSI parameters for compatibility with VFS
 * function pointer table (to match AFP function signatures), but these parameters
 * are unused since Local mode operates on file descriptors, not AFP connections.
 * Similarly, some no-op functions have all parameters unused. All unused parameters
 * are marked with _U_ attribute to satisfy static analyzers. */

/*!
 * @brief Change to directory in Local VFS heap
*/
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

/*!
 * @brief Open directory as volume in Local mode
*/
uint16_t local_openvol(CONN *conn _U_, char *vol)
{
    int fd;

    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "Open Vol %s \n\n", vol);
    }

    fd = open(vol, O_RDONLY | O_DIRECTORY);

    if (fd < 0) {
        fprintf(stderr, "Error: Failed to open directory '%s': %s\n", vol,
                strerror(errno));
        return 0xffff;
    }

    for (uint16_t i = 0; i < MAXVOL; i++) {
        if (Vol_heap[i] == NULL) {
            if (fchdir(fd) < 0) {
                close(fd);
                break;
            }

            close(fd);
            Vol_heap[i] = strdup(vol);
            Dir_heap[i][2] = strdup(vol);
            return i;
        }
    }

    return 0xffff;
}

/*!
 * @brief Close volume and cleanup heaps
*/
unsigned int local_closevol(CONN *conn _U_, uint16_t vol)
{
    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "Close Vol %d\n\n", vol);
    }

    /* Clean up volume heap to allow reopening in size sweep mode */
    if (vol < MAXVOL && Vol_heap[vol] != NULL) {
        free(Vol_heap[vol]);
        Vol_heap[vol] = NULL;

        /* Clean up directory heap for this volume */
        for (uint16_t i = 0; i < MAXDIR; i++) {
            if (Dir_heap[vol][i] != NULL) {
                free(Dir_heap[vol][i]);
                Dir_heap[vol][i] = NULL;
            }
        }
    }

    return ntohl(AFP_OK);
}

/*!
 * @brief Create directory via mkdir() and add to heap
*/
unsigned int local_createdir(CONN *conn _U_, uint16_t vol, int did, char *name)
{
    int dirfd;

    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "Create Directory Vol %d did : 0x%x <%s>\n\n", vol, ntohl(did),
                name);
    }

    did = ntohl(did);

    if (local_chdir(vol, did) < 0) {
        return 0;
    }

    for (unsigned int i = 3; i < MAXDIR; i++) {
        if (Dir_heap[vol][i] == NULL) {
            char temp[MAXPATHLEN + 1];

            if (mkdir(name, 0750) != 0) {
                fprintf(stderr, "Error: Failed to create directory '%s': %s\n", name,
                        strerror(errno));
                return ntohl(AFPERR_NOOBJ);
            }

            dirfd = open(name, O_RDONLY | O_DIRECTORY);

            if (dirfd < 0) {
                return 0;
            }

            if (fchdir(dirfd) != 0) {
                close(dirfd);
                return 0;
            }

            if (getcwd(temp, sizeof(temp)) == NULL) {
                close(dirfd);
                return 0;
            }

            close(dirfd);
            Dir_heap[vol][i] = strdup(temp);
            return htonl(i);
        }
    }

    return 0;
}

/*!
 * @brief Check file/directory existence via stat()
*/
unsigned int local_getfiledirparams(CONN *conn _U_, uint16_t vol, int did,
                                    char *name, uint16_t f_bitmap _U_, uint16_t d_bitmap _U_)
{
    struct stat st;
    did = ntohl(did);
    /* Suppress unused warning */
    (void)f_bitmap;
    (void)d_bitmap;

    if (local_chdir(vol, did) < 0) {
        return ntohl(AFPERR_NOOBJ);
    }

    if (*name != 0 && !stat(name, &st)) {
        return ntohl(AFP_OK);
    }

    return ntohl(AFPERR_NOOBJ);
}

/*!
 * @brief Delete file/directory via unlink()/rmdir()
*/
unsigned int local_delete(CONN *conn _U_, uint16_t vol, int did, char *name)
{
    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "FPDelete Vol %d did : 0x%x <%s>\n\n", vol, ntohl(did), name);
    }

    did = ntohl(did);

    if (local_chdir(vol, did) < 0) {
        return ntohl(AFPERR_PARAM);
    }

    if (*name != 0) {
        if (unlink(name)) {
            fprintf(stderr, "Error: Failed to delete file '%s': %s\n", name,
                    strerror(errno));
            return ntohl(AFPERR_NOOBJ);
        }
    } else if (rmdir(Dir_heap[vol][did])) {
        fprintf(stderr, "Error: Failed to delete directory '%s': %s\n",
                Dir_heap[vol][did] ? Dir_heap[vol][did] : "(null)", strerror(errno));
        return ntohl(AFPERR_NOOBJ);
    }

    return ntohl(AFP_OK);
}

/*!
 * @brief Create file via open()
*/
unsigned int local_createfile(CONN *conn _U_, uint16_t vol, char type, int did,
                              char *name)
{
    int fd;

    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "Create File %s Vol %d did : 0x%x <%s>\n\n",
                type ? "HARD" : "SOFT", vol, ntohl(did), name);
    }

    did = ntohl(did);

    if (local_chdir(vol, did) < 0) {
        return ntohl(AFPERR_PARAM);
    }

    fd = open(name, O_RDWR | O_CREAT, 0640);

    if (fd == -1) {
        fprintf(stderr, "Error: Failed to create file '%s': %s\n", name,
                strerror(errno));
        return ntohl(AFPERR_NOOBJ);
    }

    close(fd);
    return ntohl(AFP_OK);
}

/*!
 * @brief Open file fork and return file descriptor as fork handle
 */
uint16_t local_openfork(CONN *conn _U_, uint16_t vol, int type,
                        uint16_t bitmap _U_,
                        int did, char *name, int access)
{
    int fd;
    int flags = O_RDWR;

    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "Open Fork %s Vol %d did : 0x%x <%s> access %x\n\n",
                (type == OPENFORK_DATA) ? "data" : "resource",
                vol, ntohl(did), name, access);
    }

    if (Local_VFS_Direct) {
        flags |= O_DIRECT;
    }

    did = ntohl(did);

    if (local_chdir(vol, did) < 0) {
        return (uint16_t) ntohl(AFPERR_PARAM);
    }

    fd = open(name, flags, 0640);

    if (fd == -1) {
        fprintf(stderr, "Error: Failed to open file '%s': %s\n", name, strerror(errno));
        return (uint16_t) ntohl(AFPERR_NOOBJ);
    }

    /* In Local mode, return the fd itself (it IS the fork) */
    return (uint16_t)(fd & 0xFFFF);
}

/*!
 * @brief Write data via lseek() + write() syscalls
 */
unsigned int local_writeheader(DSI *dsi _U_, uint16_t fork, int offset,
                               int size,
                               char *data, char whence)
{
    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "send write header fork %d  offset %d size %d from 0x%x\n\n",
                fork, offset, size, (unsigned)whence);
    }

    if (lseek(fork, offset, SEEK_SET) == (off_t) -1) {
        return ntohl(AFPERR_EOF);
    }

    if (write(fork, data, size) != size) {
        return ntohl(AFPERR_EOF);
    }

    return ntohl(AFP_OK);
}

/*!
 * @brief No-op write footer (AFP compatibility)
 */
unsigned int local_writefooter(DSI *dsi _U_, uint16_t fork, int offset,
                               int size,
                               char *data _U_, char whence)
{
    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "get write footer fork %d  offset %d size %d from 0x%x\n\n",
                fork, offset, size, (unsigned)whence);
    }

    return ntohl(AFP_OK);
}

/*!
 * @brief Flush file data to disk via fsync()
*/
unsigned int local_flushfork(CONN *conn _U_, uint16_t fork)
{
    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "Flush fork %d\n\n", fork);
    }

    if (fsync(fork) < 0) {
        return ntohl(AFPERR_PARAM);
    }

    return ntohl(AFP_OK);
}

/*!
 * @brief Close file descriptor
*/
unsigned int local_closefork(CONN *conn _U_, uint16_t fork)
{
    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "Close Fork %d\n\n", fork);
    }

    if (close(fork)) {
        return ntohl(AFPERR_PARAM);
    }

    return ntohl(AFP_OK);
}

/*!
 * @brief Set file size via ftruncate()
*/
unsigned int local_setforkparam(CONN *conn _U_, uint16_t fork,  uint16_t bitmap,
                                off_t size)
{
    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "Set Fork param fork %d bitmap 0x%x size %ld\n\n", fork, bitmap,
                size);
    }

    if (ftruncate(fork, size)) {
        return ntohl(AFPERR_PARAM);
    }

    return ntohl(AFP_OK);
}

/*!
 * @brief Write data to file via lseek() + write()
 */
unsigned int local_write(CONN *conn _U_, uint16_t fork, long long offset,
                         int size,
                         char *data, char whence)
{
    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "write fork %d  offset %lld size %d from 0x%x\n\n", fork,
                offset, size, (unsigned)whence);
    }

    if (lseek(fork, offset, SEEK_SET) == (off_t) -1) {
        return ntohl(AFPERR_EOF);
    }

    if (write(fork, data, size) != size) {
        return ntohl(AFPERR_EOF);
    }

    return ntohl(AFP_OK);
}

/*!
 * @brief Read data from file via lseek() + read()
 */
unsigned int local_read(CONN *conn _U_, uint16_t fork, long long offset,
                        int size,
                        char *data)
{
    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "read fork %d  offset %lld size %d\n\n", fork, offset, size);
    }

    if (lseek(fork, offset, SEEK_SET) == (off_t) -1) {
        return ntohl(AFPERR_EOF);
    }

    if (read(fork, data, size) != size) {
        return ntohl(AFPERR_EOF);
    }

    return ntohl(AFP_OK);
}

/*!
 * @brief No-op read header (AFP compatibility)
*/
unsigned int local_readheader(DSI *dsi _U_, uint16_t fork, int offset, int size,
                              char *data _U_)
{
    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "send read header fork %d  offset %d size %d\n\n", fork, offset,
                size);
    }

    return ntohl(AFP_OK);
}

/*!
 * @brief Read data footer via lseek() + read()
*/
unsigned int local_readfooter(DSI *dsi _U_, uint16_t fork, int offset, int size,
                              char *data)
{
    if (!Local_VFS_Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "get read reply fork %d  offset %d size %d\n\n", fork, offset,
                size);
    }

    if (lseek(fork, offset, SEEK_SET) == (off_t) -1) {
        return ntohl(AFPERR_EOF);
    }

    if (read(fork, data, size) != size) {
        return ntohl(AFPERR_EOF);
    }

    return ntohl(AFP_OK);
}

/*!
 * @brief Copy file operation (not implemented, returns error)
 */
unsigned int local_copyfile(struct CONN *conn _U_, uint16_t svol _U_,
                            int sdid _U_,
                            uint16_t dvol _U_, int ddid _U_, char *src _U_, char *buf _U_, char *dst _U_)
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
