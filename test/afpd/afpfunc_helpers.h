/*
  Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

/*!
 * @file
 * @brief AFP command invocation and DSI session helpers for component tests
 */

#ifndef AFPFUNC_HELPERS
#define AFPFUNC_HELPERS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/dsi.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/queue.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afp_config.h"
#include "dircache.h"
#include "directory.h"
#include "file.h"
#include "filedir.h"
#include "hash.h"
#include "subtests.h"
#include "test.h"
#include "volume.h"

struct fsize_cap {
    struct rlimit saved;
    void (*xfsz)(int);
};

struct vi_lent {
    char *legacy;
    unsigned char *icon;
    size_t icon_len;
};

extern char **cnamewrap(const char *name);

extern int getfiledirparms(AFPObj *obj, uint16_t vid, cnid_t did,
                           const char *name);
extern int getfiledirparms_bitmap(AFPObj *obj, uint16_t vid, cnid_t did,
                                  const char *name, uint16_t fbitmap,
                                  uint16_t dbitmap);
extern int catsearch_ext(AFPObj *obj, uint16_t vid, uint16_t dbitmap,
                         uint32_t rbitmap, const void *spec1,
                         const void *spec2, uint16_t speclen, uint32_t *nrecs,
                         char lname[MACFILELEN + 1]);
extern int createdir(AFPObj *obj, uint16_t vid, cnid_t did, const char *name);
extern int createfile(AFPObj *obj, uint16_t vid, cnid_t did, const char *name);
extern int create_disk_file(const char *path);
extern int write_disk_file(const char *path, const void *data, size_t len);
extern int delete (AFPObj *obj, uint16_t vid, cnid_t did, const char *name);
extern int renameobj(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
                     const char *newname);
extern int moveobj(AFPObj *obj, uint16_t vid, cnid_t sdid, const char *name,
                   cnid_t ddid, const char *dname, const char *newname);
extern int enumerate(AFPObj *obj, uint16_t vid, cnid_t did);
extern int enumerate_names(AFPObj *obj, uint16_t vid, cnid_t did,
                           char lnames[][MACFILELEN + 1],
                           char unames[][UTF8FILELEN_EARLY + 1],
                           uint16_t max);
extern int enumerate_names_from(AFPObj *obj, uint16_t vid, cnid_t did,
                                uint32_t sindex, char lnames[][MACFILELEN + 1],
                                char unames[][UTF8FILELEN_EARLY + 1],
                                uint16_t max);
extern int enumerate_lnames(AFPObj *obj, uint16_t vid, cnid_t did,
                            char lnames[][MACFILELEN + 1], uint16_t max);
extern int getfiledirparms_lname(AFPObj *obj, uint16_t vid, cnid_t did,
                                 const char *name, char lname[MACFILELEN + 1]);
extern int enumerate_range(AFPObj *obj, uint16_t vid, cnid_t did,
                           uint16_t fbitmap, uint16_t dbitmap,
                           uint32_t sindex, uint16_t reqcnt);
extern uint16_t openvol(AFPObj *obj, const char *name);
extern void dsi_test_header(uint8_t *block, uint8_t command,
                            uint16_t request_id, uint32_t code_or_doff,
                            uint32_t len);
extern size_t dsi_test_frame(uint8_t *p, uint8_t command, uint16_t request_id,
                             uint32_t doff, uint32_t len, uint8_t afp_command);
extern int dsi_test_open(DSI *dsi, uint32_t quantum, int *peer);
extern void dsi_test_cleanup(DSI *dsi);
extern int dsi_test_connect_tcp(DSI *dsi, int *peer);
extern int dsi_test_no_loopback(int errnum);
extern void dsi_test_close_socket(DSI *dsi);
extern pid_t dsi_test_serve_as(const AFPObj *obj, uid_t uid, int ipc_fd,
                               uint32_t flags, int *peer);
extern pid_t dsi_test_serve(const AFPObj *obj, int *peer);
extern int dsi_test_reap(pid_t pid, int peer);
extern int fsize_cap(struct fsize_cap *cap, rlim_t limit);
extern int fsize_uncap(const struct fsize_cap *cap);
extern int vi_lend(struct vol *vol, struct vi_lent *lent);
extern void vi_return(struct vol *vol, const struct vi_lent *lent);
extern int openfork(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
                    uint8_t fork, uint16_t access, uint16_t *refnum);
extern int openfork_bitmap(AFPObj *obj, uint16_t vid, cnid_t did,
                           const char *name, uint8_t fork, uint16_t access,
                           uint16_t bitmap, uint16_t *refnum);
extern int closefork(AFPObj *obj, uint16_t refnum);
extern int checkaccess(AFPObj *obj, uint16_t vid, cnid_t did,
                       const char *name, uint32_t rights);
extern int resolveid(AFPObj *obj, uint16_t vid, cnid_t cnid);
extern int createid(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
                    cnid_t *id);
extern int next_second(const char *path);
extern int settle(const char *path);
extern int hints_begin(AFPObj *obj, int fds[2]);
extern int hint_came(int fd, uint8_t event, cnid_t cnid);
extern void hints_end(AFPObj *obj, const int fds[2]);
extern int setdirparms(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
                       uint16_t bitmap, const void *params, size_t len);
extern int getextattr(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
                      const char *attr, uint32_t maxreply, char *reply,
                      size_t *replylen);
extern int remextattr(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
                      const char *attr);
extern int setextattr(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
                      const char *attr, const char *value, uint32_t len);
extern int exchangefiles(AFPObj *obj, uint16_t vid, cnid_t sid, cnid_t did,
                         const char *name1, const char *name2);
extern int setacl(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
                  uint16_t bitmap, const void *body, size_t len);
extern int flushvol(AFPObj *obj, uint16_t vid);
extern int setforkparams(AFPObj *obj, uint16_t refnum, uint32_t len);
extern int getforkparams(AFPObj *obj, uint16_t refnum, uint16_t bitmap);
extern int getvolparams(AFPObj *obj, uint16_t vid, uint16_t bitmap);
extern int listextattr(AFPObj *obj, uint16_t vid, cnid_t did,
                       const char *name, uint32_t maxreply, char *reply,
                       size_t *replylen);
extern void pack_appledouble(uint8_t *buf, const uint32_t *entry, uint16_t n);
extern void drop_entry(const struct vol *vol, cnid_t did, char *name);

#endif  /* AFPFUNC_HELPERS */
