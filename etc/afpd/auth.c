/*
 * Copyright (c) 2026 Andy Lemin (andylemin)
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief AFP login, logout, password, and session token commands, and UAM
 * loading and dispatch
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <atalk/afp.h>
#ifndef NO_DDP
#include <atalk/asp.h>
#endif
#include <atalk/compat.h>
#include <atalk/dsi.h>
#include <atalk/fce_api.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/server_ipc.h>

#ifdef WITH_SPOTLIGHT
#include <atalk/spotlight.h>
#endif

#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/uuid.h>

#ifdef HAVE_ACLS
#include "acls.h"
#endif

#include "auth.h"
#include "extattrs.h"
#include "fork.h"
#include "status.h"
#include "switch.h"
#include "uam_auth.h"

/* Deferred SIGTERM flag — defined in afp_dsi.c */
extern volatile sig_atomic_t die_pending;

static int afp_version_index;
static struct uam_mod uam_modules = {NULL, NULL, &uam_modules, &uam_modules};
static struct uam_obj uam_login = {"", "", 0, {{NULL, NULL, NULL, NULL }}, &uam_login,
    &uam_login
};
static struct uam_obj uam_changepw = {"", "", 0, {{NULL, NULL, NULL, NULL}}, &uam_changepw,
    &uam_changepw
};

static struct uam_obj *afp_uam = NULL;

/*!
 * @brief Return the size of the socket address structure for the family of an
 * address
 *
 * @param[in] sa  socket address
 * @returns the size of sockaddr_in or sockaddr_in6, or of sockaddr_storage for
 *          another family
 */
static socklen_t sockaddr_len(const struct sockaddr *sa)
{
    switch (sa->sa_family) {
    case AF_INET:
        return sizeof(struct sockaddr_in);

    case AF_INET6:
        return sizeof(struct sockaddr_in6);

    default:
        return sizeof(struct sockaddr_storage);
    }
}

/*!
 * @brief Name the client of a session for the login report to the master afpd
 *
 * A DSI client gets its DNS name, or its numeric address when the reverse
 * lookup fails. An ASP client gets its AppleTalk network and node numbers.
 *
 * @param[in] obj  session object
 * @returns the name in a static buffer, or "-" for an unknown transport
 */
static const char *client_address(AFPObj *obj)
{
    static char host[NI_MAXHOST];

    if (obj->proto == AFPPROTO_DSI && obj->dsi) {
        const struct sockaddr *sa = (const struct sockaddr *)&obj->dsi->client;

        if (getnameinfo(sa, sockaddr_len(sa), host, sizeof(host), NULL, 0,
                        NI_NAMEREQD) == 0) {
            return host;
        }

        return getip_string(sa);
    }

#ifndef NO_DDP

    if (obj->proto == AFPPROTO_ASP && obj->handle) {
        const struct ASP *asp = obj->handle;
        snprintf(host, sizeof(host), "%u.%u",
                 ntohs(asp->asp_sat.sat_addr.s_net),
                 asp->asp_sat.sat_addr.s_node);
        return host;
    }

#endif
    return "-";
}

/*!
 * @brief Write the AFP version list of the FPGetSrvrInfo reply into a status
 * block
 *
 * The list starts at the offset stored at AFPSTATUS_VERSOFF. Versions up to
 * 2.1 need an ASP listener, and versions after 2.2 need a DSI listener, so
 * both transports list 2.2. The call stores the end of the list as the UAM
 * list offset.
 *
 * @param[in,out] data  status block
 * @param[in]     asp   ASP listener, or NULL for the DSI status
 * @param[in]     dsi   DSI listener, or NULL for the ASP status
 */
void status_versions(char *data,
#ifndef NO_DDP
                     const ASP asp,
#endif
                     const DSI *dsi)
{
    char                *start = data;
    uint16_t           status;
    int         len, num, i, count = 0;
    memcpy(&status, start + AFPSTATUS_VERSOFF, sizeof(status));
    num = sizeof(afp_versions) / sizeof(afp_versions[0]);

    for (i = 0; i < num; i++) {
#ifndef NO_DDP

        if (!asp && (afp_versions[i].av_number <= 21)) {
            continue;
        }

#endif /* ! NO_DDP */

        if (!dsi && (afp_versions[i].av_number > 22)) {
            continue;
        }

        count++;
    }

    data += ntohs(status);
    *data++ = count;

    for (i = 0; i < num; i++) {
#ifndef NO_DDP

        if (!asp && (afp_versions[i].av_number <= 21)) {
            continue;
        }

#endif /* ! NO_DDP */

        if (!dsi && (afp_versions[i].av_number > 22)) {
            continue;
        }

        len = strlen(afp_versions[i].av_name);
        *data++ = len;
        memcpy(data, afp_versions[i].av_name, len);
        data += len;
    }

    status = htons(data - start);
    memcpy(start + AFPSTATUS_UAMSOFF, &status, sizeof(status));
}

/*!
 * @brief Write the UAM name list of the FPGetSrvrInfo reply into a status block
 *
 * The list holds every loaded UAM whose module path appears in authlist, and
 * the call logs each as available. The list starts at the offset stored at
 * AFPSTATUS_UAMSOFF, and the call stores its end as the icon offset.
 *
 * @param[in,out] data      status block
 * @param[in]     authlist  uam list option of afp.conf
 */
void status_uams(char *data, const char *authlist)
{
    char                *start = data;
    uint16_t           status;
    struct uam_obj      *uams;
    int         len, num = 0;
    memcpy(&status, start + AFPSTATUS_UAMSOFF, sizeof(status));
    uams = &uam_login;

    while ((uams = uams->uam_prev) != &uam_login) {
        if (strstr(authlist, uams->uam_path)) {
            num++;
        }
    }

    data += ntohs(status);
    *data++ = num;

    while ((uams = uams->uam_prev) != &uam_login) {
        if (strstr(authlist, uams->uam_path)) {
            LOG(log_info, logtype_afpd, "uam: \"%s\" available", uams->uam_name);
            len = strlen(uams->uam_name);
            *data++ = len;
            memcpy(data, uams->uam_name, len);
            data += len;
        }
    }

    /* icon offset */
    status = htons(data - start);
    memcpy(start + AFPSTATUS_ICONOFF, &status, sizeof(status));
}

/*!
 * @brief Pass a login result on, or answer a failed login and end the session
 *
 * AFP_OK and AFPERR_AUTHCONT go back to the caller unchanged. Any other code
 * goes to the client as the reply, and the session then exits through
 * obj->exit.
 *
 * @param[in] obj  session object
 * @param[in] err  result of the login step
 * @returns err for AFP_OK and AFPERR_AUTHCONT, or AFP_OK when obj->exit returns
 */
static int send_reply(const AFPObj *obj, const int err)
{
    if ((err == AFP_OK) || (err == AFPERR_AUTHCONT)) {
        return err;
    }

    if (obj->proto == AFPPROTO_DSI) {
        obj->reply(obj->dsi, err);
    } else {
        obj->reply(obj->handle, err);
    }

    obj->exit(0);
    return AFP_OK;
}

/*!
 * @brief Answer a command with AFPERR_PWDEXPR while a password change is due
 *
 * A DSIWrite's payload is read off the stream before the reply, as every
 * handler a DSIWrite frame runs must read it.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block, unused
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns AFPERR_PWDEXPR
 */
int afp_errpwdexpired(AFPObj *obj, char *ibuf _U_, size_t ibuflen _U_,
                      char *rbuf _U_, size_t *rbuflen)
{
    if (obj->proto == AFPPROTO_DSI) {
        dsi_writedrain(obj->dsi);
    }

    *rbuflen = 0;
    return AFPERR_PWDEXPR;
}

/*!
 * @brief Answer a command with AFPERR_NOOP and no log line
 *
 * @param[in]  obj      session object, unused
 * @param[in]  ibuf     request block, unused
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns AFPERR_NOOP
 */
static int afp_null_nolog(AFPObj *obj _U_, char *ibuf _U_, size_t ibuflen _U_,
                          char *rbuf _U_, size_t *rbuflen)
{
    *rbuflen = 0;
    return AFPERR_NOOP;
}

/*!
 * @brief Install the command table of a logged-in session
 *
 * A valid login switches to the postauth table and adds the commands of the
 * negotiated AFP version. Each version also gets the commands of the versions
 * below it:
 *
 * - 2.2 adds FPZzzzz.
 * - 3.0 adds FPEnumerateExt, FPByteRangeLockExt, FPCatSearchExt, FPReadExt,
 *   FPWriteExt, FPGetSessionToken, and FPDisconnectOldSession.
 * - 3.1 adds FPSyncDir, FPSyncFork, FPEnumerateExt2, and the Spotlight command.
 * - 3.2 adds the extended attribute commands and, with ACL support, the ACL
 *   commands.
 *
 * An expired password keeps the preauth table. Every command then answers
 * AFPERR_PWDEXPR, except FPLogout and FPChangePassword.
 *
 * @param[in] obj      session object
 * @param[in] expired  nonzero when the UAM reported an expired password
 * @returns AFP_OK
 */
static int set_auth_switch(const AFPObj *obj, int expired)
{
    int i;

    if (!expired) {
        afp_switch = postauth_switch;

        switch (obj->afp_version) {
        case 34:
        case 33:
        case 32:
#ifdef HAVE_ACLS
            uam_afpserver_action(AFP_GETACL, UAM_AFPSERVER_POSTAUTH, afp_getacl, NULL);
            uam_afpserver_action(AFP_SETACL, UAM_AFPSERVER_POSTAUTH, afp_setacl, NULL);
            uam_afpserver_action(AFP_ACCESS, UAM_AFPSERVER_POSTAUTH, afp_access, NULL);
#endif /* HAVE_ACLS */
            uam_afpserver_action(AFP_GETEXTATTR, UAM_AFPSERVER_POSTAUTH, afp_getextattr,
                                 NULL);
            uam_afpserver_action(AFP_SETEXTATTR, UAM_AFPSERVER_POSTAUTH, afp_setextattr,
                                 NULL);
            uam_afpserver_action(AFP_REMOVEATTR, UAM_AFPSERVER_POSTAUTH, afp_remextattr,
                                 NULL);
            uam_afpserver_action(AFP_LISTEXTATTR, UAM_AFPSERVER_POSTAUTH, afp_listextattr,
                                 NULL);

        case 31:
            uam_afpserver_action(AFP_SYNCDIR, UAM_AFPSERVER_POSTAUTH, afp_syncdir, NULL);
            uam_afpserver_action(AFP_SYNCFORK, UAM_AFPSERVER_POSTAUTH, afp_syncfork, NULL);
#ifdef WITH_SPOTLIGHT
            uam_afpserver_action(AFP_SPOTLIGHT_PRIVATE, UAM_AFPSERVER_POSTAUTH,
                                 afp_spotlight_rpc, NULL);
#endif
            uam_afpserver_action(AFP_ENUMERATE_EXT2, UAM_AFPSERVER_POSTAUTH,
                                 afp_enumerate_ext2, NULL);

        case 30:
            uam_afpserver_action(AFP_ENUMERATE_EXT, UAM_AFPSERVER_POSTAUTH,
                                 afp_enumerate_ext, NULL);
            uam_afpserver_action(AFP_BYTELOCK_EXT,  UAM_AFPSERVER_POSTAUTH,
                                 afp_bytelock_ext, NULL);
            /* catsearch_ext uses the same packet as catsearch FIXME double check this, it wasn't true for enue
               enumerate_ext */
            uam_afpserver_action(AFP_CATSEARCH_EXT, UAM_AFPSERVER_POSTAUTH,
                                 afp_catsearch_ext, NULL);
            uam_afpserver_action(AFP_GETSESSTOKEN,  UAM_AFPSERVER_POSTAUTH, afp_getsession,
                                 NULL);
            uam_afpserver_action(AFP_READ_EXT,      UAM_AFPSERVER_POSTAUTH, afp_read_ext,
                                 NULL);
            uam_afpserver_action(AFP_WRITE_EXT,     UAM_AFPSERVER_POSTAUTH, afp_write_ext,
                                 NULL);
            uam_afpserver_action(AFP_DISCTOLDSESS,  UAM_AFPSERVER_POSTAUTH, afp_disconnect,
                                 NULL);

        case 22:
            /*
             * If first connection to a server is done in classic AFP2.2 version is used
             * but OSX uses AFP3.x FPZzzzz command !
             */
            uam_afpserver_action(AFP_ZZZZZ,  UAM_AFPSERVER_POSTAUTH, afp_zzzzz, NULL);
            break;
        }
    } else {
        /*
         * BF: expired password handling
         * to allow the user to change his/her password we have to allow login
         * but every following call except for FPChangePassword will be thrown
         * away with an AFPERR_PWDEXPR error. (thanks to Leland Wallace from Apple
         * for clarifying this)
         */
        for (i = 0; i <= 0xff; i++) {
            uam_afpserver_action(i, UAM_AFPSERVER_PREAUTH, afp_errpwdexpired, NULL);
        }

        uam_afpserver_action(AFP_LOGOUT, UAM_AFPSERVER_PREAUTH, afp_logout, NULL);
        uam_afpserver_action(AFP_CHANGEPW, UAM_AFPSERVER_PREAUTH, afp_changepw, NULL);
    }

    return AFP_OK;
}

/*!
 * @brief Tell whether the server may serve a user
 *
 * A single-user server cannot switch to another account, so only the uid that
 * started it may log in. A server without OPTION_SINGLEUSER serves every user
 * that login admits.
 *
 * @param[in] obj  session object
 * @param[in] uid  uid of the user who logs in
 * @returns true when the server may serve uid
 */
bool singleuser_admits_uid(const AFPObj *obj, uid_t uid)
{
    return !(obj->options.flags & OPTION_SINGLEUSER) || uid == getuid();
}

/*!
 * @brief Finish a login after the UAM accepted the user
 *
 * The call refuses root, a user that a single-user server may not serve, and
 * a login past the connection limit. It sets the groups and switches the euid
 * and egid to the user. A member of the admin group keeps the root euid
 * instead. The force user and force group options then override the switch.
 *
 * Next the call installs the command table and reports the login with the
 * client address to the master afpd. It resets the umask, because PAM may
 * change it. A DSI session also installs its signal handlers again.
 *
 * @param[in,out] obj      session object
 * @param[in]     pwd      password entry of the user
 * @param[in]     logout   logout hook of the UAM, or NULL
 * @param[in]     expired  nonzero when the password expired
 * @returns AFP_OK, AFPERR_NOTAUTH, AFPERR_MAXSESS, or AFPERR_BADUAM
 */
static int login(AFPObj *obj, struct passwd *pwd, void (*logout)(void),
                 int expired)
{
    bool admin = false;
    int i;

    /* don't allow root login */
    if (pwd->pw_uid == 0) {
        LOG(log_error, logtype_afpd, "login: root login denied!");
        return AFPERR_NOTAUTH;
    }

    /* judged before any group is touched */
    if (!singleuser_admits_uid(obj, pwd->pw_uid)) {
        LOG(log_error, logtype_afpd,
            "login: single-user server rejects user %s (uid %u, server uid %u)",
            pwd->pw_name, (unsigned int)pwd->pw_uid, (unsigned int)getuid());
        return AFPERR_NOTAUTH;
    }

    if (obj->cnx_cnt >= obj->cnx_max) {
        LOG(log_error, logtype_dsi, "login: too many connections, limit: %d",
            obj->cnx_max);
        return AFPERR_MAXSESS;
    }

    LOG(log_note, logtype_afpd, "Login by %s (%s)",
        pwd->pw_name, afp_versions[afp_version_index].av_name);

    if (set_groups(obj, pwd) != 0) {
        return AFPERR_BADUAM;
    }

    LOG(log_debug, logtype_afpd, "obj->options.admingid == %d",
        obj->options.admingid);

    if (obj->options.admingid != 0) {
        for (i = 0; i < obj->ngroups; i++) {
            if (obj->groups[i] == obj->options.admingid) {
                admin = true;
            }
        }
    }

    if (admin) {
        ad_setfuid(0);
        LOG(log_info, logtype_afpd, "admin login -- %s", pwd->pw_name);
    } else {
        if (setegid(pwd->pw_gid) < 0 || seteuid(pwd->pw_uid) < 0) {
            LOG(log_error, logtype_afpd, "login: %s %s", pwd->pw_name, strerror(errno));
            return AFPERR_BADUAM;
        }
    }

    if (obj->options.force_user) {
        if (seteuid(obj->options.force_uid) < 0) {
            LOG(log_error, logtype_afpd, "login: %s %s", pwd->pw_name, strerror(errno));
            return AFPERR_BADUAM;
        }

        LOG(log_info, logtype_afpd, "login: force uid: %ju",
            (uintmax_t)obj->options.force_uid);
    }

    if (obj->options.force_group) {
        if (setegid(obj->options.force_gid) < 0) {
            LOG(log_error, logtype_afpd, "login: %s %s", pwd->pw_name, strerror(errno));
            return AFPERR_BADUAM;
        }

        LOG(log_info, logtype_afpd, "login: force gid: %ju",
            (uintmax_t)obj->options.force_gid);
    }

    LOG(log_debug, logtype_afpd, "login: supplementary groups: %s",
        print_groups(obj->ngroups, obj->groups));
    set_auth_switch(obj, expired);
    obj->logout = logout;
    obj->uid = pwd->pw_uid;
    obj->euid = geteuid();
    obj->pid = getpid();
    /* report to parent */
    const char *address = client_address(obj);
    ipc_child_write(obj, IPC_LOGINDONE,
                    strnlen(address, IPC_MAXMSGSIZE - IPC_HEADERLEN),
                    (void *)address);

    if (obj->proto == AFPPROTO_ASP) {
        ipc_child_state(obj, ASP_RUNNING);
    }

    /* pam_umask or similar might have changed our umask */
    (void)umask(obj->options.umask);

    /* Some PAM module might have reset our signal handlers and timer, so we need to reestablish them */
    if (obj->proto == AFPPROTO_DSI) {
        afp_over_dsi_sighandlers(obj);
    }

#ifdef WITH_FCE
    /* Send FCE login event */
    fce_register(obj, FCE_LOGIN, "", NULL);
#endif /* WITH_FCE */
    return AFP_OK;
}

/*!
 * @brief FPZzzzz: put the DSI session to sleep or wake it from extended sleep
 *
 * A sleep request sets DSI_SLEEPING and closes the recvfile pipe, and the
 * extended sleep flag adds DSI_EXTSLEEP. The extended wakeup flag ends an
 * extended sleep. Each change of state goes to the master afpd.
 *
 * The reply holds the sleep option divided by 120, at least 1. That gives the
 * hours a sleeping session lasts at the default tickleval of 30 seconds.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFP_OK, or AFPERR_MISC for a request under 6 bytes
 */
int afp_zzzzz(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
              size_t *rbuflen)
{
    uint32_t data;
    DSI *dsi = (DSI *)AFPobj->dsi;
    *rbuflen = 0;
    ibuf += 2;
    ibuflen -= 2;

    if (ibuflen < 4) {
        return AFPERR_MISC;
    }

    memcpy(&data, ibuf, 4); /* flag */
    data = ntohl(data);

    /*
     * Possible sleeping states:
     * 1) normal sleep: DSI_SLEEPING (up to 10.3)
     * 2) extended sleep: DSI_SLEEPING | DSI_EXTSLEEP (starting with 10.4)
     */

    if (data & AFPZZZ_EXT_WAKEUP) {
        /* wakeup request from exetended sleep */
        if (dsi->flags & DSI_EXTSLEEP) {
            LOG(log_note, logtype_afpd, "afp_zzzzz: waking up from extended sleep");
            dsi->flags &= ~(DSI_SLEEPING | DSI_EXTSLEEP);
            ipc_child_state(obj, DSI_RUNNING);
        }
    } else {
        /* sleep request */
        dsi->flags |= DSI_SLEEPING;
        dsi_close_pipe(dsi);

        if (data & AFPZZZ_EXT_SLEEP) {
            LOG(log_note, logtype_afpd, "afp_zzzzz: entering extended sleep");
            dsi->flags |= DSI_EXTSLEEP;
            ipc_child_state(obj, DSI_EXTSLEEP);
        } else {
            LOG(log_note, logtype_afpd, "afp_zzzzz: entering normal sleep");
            ipc_child_state(obj, DSI_SLEEPING);
        }
    }

    /*
     * According to AFP 3.3 spec we should not return anything,
     * but e.g. 10.5.8 server still returns the numbers of hours
     * the server is keeping the sessino (ie max sleeptime).
     */
    data = obj->options.sleep / 120; /* hours */

    if (!data) {
        data = 1;
    }

    *rbuflen = sizeof(data);
    data = htonl(data);
    memcpy(rbuf, &data, sizeof(data));
    rbuf += sizeof(data);
    return AFP_OK;
}

/*!
 * @brief Allocate the session token and fill it with random bytes
 *
 * @param[in,out] obj  session object that receives the token
 * @returns 0, or AFPERR_MISC when the allocation or the random source fails
 */
static int create_session_token(AFPObj *obj)
{
    if (NULL == (obj->sinfo.sessiontoken = malloc(SESSIONTOKEN_LEN))) {
        return AFPERR_MISC;
    }

    obj->sinfo.sessiontoken_len = SESSIONTOKEN_LEN;

    if (uam_random_string(obj, obj->sinfo.sessiontoken, SESSIONTOKEN_LEN) < 0) {
        free(obj->sinfo.sessiontoken);
        obj->sinfo.sessiontoken = NULL;
        return AFPERR_MISC;
    }

    return 0;
}

/*!
 * @brief Allocate sinfo.sessionkey once and fill it with random bytes
 *
 * The GSS UAM encrypts it with gss_wrap for its login reply. A session that
 * has one keeps it.
 *
 * @param[in,out] obj  session object that receives sessionkey
 * @returns AFP_OK, or AFPERR_MISC when the allocation or the random source
 *          fails
 */
static int create_session_key(AFPObj *obj)
{
    /* create session key */
    if (obj->sinfo.sessionkey == NULL) {
        if (NULL == (obj->sinfo.sessionkey = malloc(SESSIONKEY_LEN))) {
            return AFPERR_MISC;
        }

        if (uam_random_string(obj, obj->sinfo.sessionkey, SESSIONKEY_LEN) != 0) {
            free(obj->sinfo.sessionkey);
            obj->sinfo.sessionkey = NULL;
            return AFPERR_MISC;
        }

        obj->sinfo.sessionkey_len = SESSIONKEY_LEN;
    }

    return AFP_OK;
}


/*!
 * @brief FPGetSessionToken: return a token that a reconnecting client can
 * present
 *
 * The session creates its random token on the first call and hands it to the
 * master afpd. Types 0 to 2 return the token. Type 8 returns the encrypted
 * Kerberos session key instead.
 *
 * Types 3 and 4 also carry a client ID and a boot time, which go to the
 * master as well. The master then ends an older session of the same client
 * and user whose boot time differs, because that client rebooted. The reply
 * holds a 4-byte length and the token.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFP_OK, AFPERR_NOOP for an unknown type, AFPERR_PARAM for a short
 *          request, or AFPERR_MISC
 */
int afp_getsession(
    AFPObj *obj,
    char   *ibuf, size_t ibuflen,
    char   *rbuf, size_t *rbuflen)
{
    uint16_t           type;
    uint32_t           idlen = 0;
    uint32_t       boottime;
    uint32_t           tklen, tp;
    char                *token;
    char                *p;
    *rbuflen = 0;
    tklen = 0;

    if (ibuflen < 2 + sizeof(type)) {
        return AFPERR_PARAM;
    }

    ibuf += 2;
    ibuflen -= 2;
    memcpy(&type, ibuf, sizeof(type));
    type = ntohs(type);
    ibuf += sizeof(type);
    ibuflen -= sizeof(type);

    if (obj->sinfo.sessiontoken == NULL) {
        if (create_session_token(obj)) {
            return AFPERR_MISC;
        }
    }

    if (ipc_child_write(obj, IPC_SESSIONTOKEN, obj->sinfo.sessiontoken_len,
                        obj->sinfo.sessiontoken) != 0) {
        return AFPERR_MISC;
    }

    /*
     *
     */
    switch (type) {
    /* old version ? */
    case 0:
        tklen = obj->sinfo.sessiontoken_len;
        token = obj->sinfo.sessiontoken;
        break;

    /* disconnect */
    case 1:

    /* reconnect update id */
    case 2:
        if (ibuflen >= sizeof(idlen)) {
            memcpy(&idlen, ibuf, sizeof(idlen));
            idlen = ntohl(idlen);
            ibuf += sizeof(idlen);
            ibuflen -= sizeof(idlen);

            if (ibuflen < idlen) {
                return AFPERR_PARAM;
            }

#if 0
            memcpy(id, ibuf, idlen)
#endif
            tklen = obj->sinfo.sessiontoken_len;
            token = obj->sinfo.sessiontoken;
        }

        break;

    case 3:
    case 4:
        if (ibuflen >= 8) {
            p = ibuf;
            memcpy(&idlen, ibuf, sizeof(idlen));
            idlen = ntohl(idlen);
            ibuf += sizeof(idlen);
            ibuflen -= sizeof(idlen);
            ibuf += sizeof(boottime);
            ibuflen -= sizeof(boottime);

            if (ibuflen < idlen || idlen > (90 - 10)) {
                return AFPERR_PARAM;
            }

            if (!obj->sinfo.clientid) {
                obj->sinfo.clientid = malloc(idlen + 8);
                memcpy(obj->sinfo.clientid, p, idlen + 8);
                obj->sinfo.clientid_len = idlen + 8;
            }

            if (ipc_child_write(obj, IPC_GETSESSION, idlen + 8, p) != 0) {
                return AFPERR_MISC;
            }

            tklen = obj->sinfo.sessiontoken_len;
            token = obj->sinfo.sessiontoken;
        }

        break;

    /* Panther Kerberos Token */
    case 8:
        tklen = obj->sinfo.cryptedkey_len;
        token = obj->sinfo.cryptedkey;
        break;

    default:
        return AFPERR_NOOP;
    }

    if (tklen == 0) {
        return AFPERR_MISC;
    }

    tp = htonl(tklen);
    memcpy(rbuf, &tp, sizeof(tklen));
    rbuf += sizeof(tklen);
    *rbuflen += sizeof(tklen);
    memcpy(rbuf, token, tklen);
    *rbuflen += tklen;
    return AFP_OK;
}

/*!
 * @brief FPDisconnectOldSession: hand the connection to the disconnected
 * session that owns a token
 *
 * A client sends it on a new connection to resume a disconnected session.
 * The call stops the tickle timer and sends the token, the DSI request ID,
 * and the client socket to the master afpd. The master passes them to the old
 * session, which answers the request, and sends SIGTERM to this one. A
 * SIGTERM within the 5-second wait marks success, and the process exits.
 *
 * A failed handoff restarts the timer and leaves DSI_RECONINPROG set, so the
 * next connection error ends this session.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns AFPERR_MISC when the token length is wrong or the handoff fails, or
 *          AFPERR_PARAM for a short request
 */
int afp_disconnect(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf _U_,
                   size_t *rbuflen)
{
    DSI                 *dsi = (DSI *)obj->dsi;
    uint16_t           type;
    uint32_t           tklen;
    char               *token;
    *rbuflen = 0;

    if (ibuflen < 2 + sizeof(type) + sizeof(tklen)) {
        return AFPERR_PARAM;
    }

    ibuf += 2;
    ibuflen -= 2;
#if 0

    /* check for guest user */
    if (0 == (strcasecmp(obj->username, obj->options.guest))) {
        return AFPERR_MISC;
    }

#endif
    memcpy(&type, ibuf, sizeof(type));
    type = ntohs(type);
    ibuf += sizeof(type);
    ibuflen -= sizeof(type);
    memcpy(&tklen, ibuf, sizeof(tklen));
    tklen = ntohl(tklen);
    ibuf += sizeof(tklen);
    ibuflen -= sizeof(tklen);
    (void)type;

    if (tklen != SESSIONTOKEN_LEN) {
        return AFPERR_MISC;
    }

    if (ibuflen < tklen) {
        return AFPERR_PARAM;
    }

    token = ibuf;
    LOG(log_note, logtype_afpd, "afp_disconnect: trying primary reconnect");
    dsi->flags |= DSI_RECONINPROG;
    /* Deactivate tickle timer */
    const struct itimerval none = {{0, 0}, {0, 0}};
    setitimer(ITIMER_REAL, &none, NULL);

    /* check for old session, possibly transferring session from here to there */
    if (ipc_child_write(obj, IPC_DISCOLDSESSION, tklen, token) != 0) {
        goto exit;
    }

    /* write uint16_t DSI request ID */
    if (writet(obj->ipc_fd, &dsi->header.dsi_requestID, 2, 0, 2) != 2) {
        LOG(log_error, logtype_afpd, "afp_disconnect: couldn't send DSI request ID");
        goto exit;
    }

    /* now send our connected AFP client socket */
    if (send_fd(obj->ipc_fd, dsi->socket) != 0) {
        goto exit;
    }

    /* Wait out the window in which master may SIGTERM us if the session
     * transferred to Child A. */
    sleep(5);

    /* SIGTERM during sleep(5): drop the flag so the success branch fires. */
    if (die_pending) {
        die_pending = 0;
        dsi->flags &= ~DSI_RECONINPROG;
    }

    if (!(dsi->flags & DSI_RECONINPROG)) {
        sleep(2); /* let the reconnecting session settle */
        LOG(log_note, logtype_afpd, "afp_disconnect: primary reconnect succeeded");
        exit(0);
    }

exit:

    /* The handoff failures jump here, above the sleep(5) sweep; die_pending here
     * means a termination signal raced the handoff after the transfer to Child
     * A completed.  Not afp_dsi_die(): its DSI_RECONINPROG guard would swallow
     * the exit.  Preserve afp_dsi_die()'s exit mapping so a non-SIGTERM signal
     * (e.g. SIGQUIT) keeps its diagnostic exit status. */
    if (die_pending) {
        int sig = die_pending;
        die_pending = 0;
        dsi->flags &= ~DSI_RECONINPROG;
        LOG(log_note, logtype_afpd,
            "afp_disconnect: session transferred, child exiting");

        if (sig == SIGTERM || sig == SIGALRM || sig < 0) {
            exit(0);
        }

        exit(sig);
    }

    /* Leave DSI_RECONINPROG set: the afp_over_dsi() backstop uses it to reap
     * this child on the next connection error. */
    setitimer(ITIMER_REAL, &dsi->timer, NULL);
    LOG(log_error, logtype_afpd, "afp_disconnect: primary reconnect failed");
    return AFPERR_MISC;
}

/*!
 * @brief Select the AFP version that a login request names
 *
 * The call compares the first len bytes of the request with each known
 * version string, so a prefix of one also matches. A version from 3.0 on
 * needs a 64-bit off_t.
 *
 * @param[in,out] obj      session object that receives afp_version
 * @param[in]     ibuf     version string of the request
 * @param[in]     ibuflen  bytes left in the request
 * @param[in]     len      length of the version string
 * @returns 0, or AFPERR_BADVERS for an empty, oversized, unknown, or
 *          unsupported version
 */
static int get_version(AFPObj *obj, char *ibuf, size_t ibuflen, size_t len)
{
    int num, i;

    if (!len || len > ibuflen) {
        return AFPERR_BADVERS;
    }

    num = sizeof(afp_versions) / sizeof(afp_versions[0]);

    for (i = 0; i < num; i++) {
        if (strncmp(ibuf, afp_versions[i].av_name, len) == 0) {
            obj->afp_version = afp_versions[i].av_number;
            afp_version_index = i;
            break;
        }
    }

    /* An inappropo version */
    if (i == num) {
        return AFPERR_BADVERS;
    }

    /* FIXME Hack */
    if (obj->afp_version >= 30 && sizeof(off_t) != 8) {
        LOG(log_error, logtype_afpd, "get_version: no LARGE_FILE support recompile!");
        return AFPERR_BADVERS;
    }

    return 0;
}

/*!
 * @brief FPLogin: select the AFP version and the UAM, and run the first login
 * step
 *
 * The request holds the version string and the UAM name as Pascal strings,
 * then the data of the UAM. A UAM that needs another round trip answers
 * AFPERR_AUTHCONT, and FPLoginCont continues. An accepted user goes through
 * login. Every other failure answers the client and ends the session. After
 * the master announced a shutdown, the failure is AFPERR_SHUTDOWN.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFP_OK or AFPERR_AUTHCONT, and after a failure AFP_OK when the
 *          session exit returns
 */
int afp_login(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
              size_t *rbuflen)
{
    struct passwd *pwd = NULL;
    size_t len;
    int     i;
    *rbuflen = 0;

    if (nologin & 1) {
        return send_reply(obj, AFPERR_SHUTDOWN);
    }

    if (ibuflen < 2) {
        return send_reply(obj, AFPERR_BADVERS);
    }

    if (ibuf == NULL) {
        return send_reply(obj, AFPERR_PARAM);
    }

    ibuf++;
    len = (unsigned char) * ibuf++;
    ibuflen -= 2;
    i = get_version(obj, ibuf, ibuflen, len);

    if (i) {
        return send_reply(obj, i);
    }

    if (ibuflen <= len) {
        return send_reply(obj, AFPERR_BADUAM);
    }

    ibuf += len;
    ibuflen -= len;
    len = (unsigned char) * ibuf++;
    ibuflen--;

    if (!len || len > ibuflen) {
        return send_reply(obj, AFPERR_BADUAM);
    }

    if (NULL == (afp_uam = auth_uamfind(UAM_SERVER_LOGIN, ibuf, len))) {
        return send_reply(obj, AFPERR_BADUAM);
    }

    ibuf += len;
    ibuflen -= len;

    if (AFP_OK != (i = create_session_key(obj))) {
        return send_reply(obj, i);
    }

    i = afp_uam->u.uam_login.login(obj, &pwd, ibuf, ibuflen, rbuf, rbuflen);
    LOG(log_debug, logtype_afpd, "afp_login: UAM login returned %d, pwd=%p", i,
        pwd);

    if (i == AFPERR_AUTHCONT) {
        return send_reply(obj, i);
    }

    if (!pwd || (i != AFP_OK && i != AFPERR_PWDEXPR)) {
        LOG(log_error, logtype_afpd, "afp_login: UAM login failed, sending reply");
        return send_reply(obj, i);
    }

    LOG(log_debug, logtype_afpd, "afp_login: UAM login OK, calling login()");
    return send_reply(obj, login(obj, pwd, afp_uam->u.uam_login.logout,
                                 ((i == AFPERR_PWDEXPR) ? 1 : 0)));
}

/*!
 * @brief FPLoginExt: select the AFP version and the UAM, and run the first
 * login step with a UTF-8 user name
 *
 * The request adds flags, a user name of type 3, and a directory service name
 * to the fields of FPLogin. The UAM must offer login_ext, and it gets the user
 * name with its type byte. The results match those of afp_login.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFP_OK or AFPERR_AUTHCONT, and after a failure AFP_OK when the
 *          session exit returns
 */
int afp_login_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                  size_t *rbuflen)
{
    struct passwd *pwd = NULL;
    size_t  len;
    int     i;
    char        type;
    uint16_t   len16;
    char        *username;
    *rbuflen = 0;

    if (nologin & 1) {
        return send_reply(obj, AFPERR_SHUTDOWN);
    }

    if (ibuflen < 5) {
        return send_reply(obj, AFPERR_BADVERS);
    }

    if (ibuf == NULL) {
        return send_reply(obj, AFPERR_PARAM);
    }

    ibuf++;
    ibuf++;     /* pad  */
    ibuf += 2;  /* flag */
    len = (unsigned char) * ibuf;
    ibuf++;
    ibuflen -= 5;
    i = get_version(obj, ibuf, ibuflen, len);

    if (i) {
        return send_reply(obj, i);
    }

    if (ibuflen <= len) {
        return send_reply(obj, AFPERR_BADUAM);
    }

    ibuf    += len;
    ibuflen -= len;
    len = (unsigned char) * ibuf;
    ibuf++;
    ibuflen--;

    if (!len || len > ibuflen) {
        return send_reply(obj, AFPERR_BADUAM);
    }

    if ((afp_uam = auth_uamfind(UAM_SERVER_LOGIN, ibuf, len)) == NULL) {
        return send_reply(obj, AFPERR_BADUAM);
    }

    ibuf    += len;
    ibuflen -= len;

    if (!afp_uam->u.uam_login.login_ext) {
        LOG(log_error, logtype_afpd, "login_ext: uam %s not AFP 3 ready!",
            afp_uam->uam_name);
        return send_reply(obj, AFPERR_BADUAM);
    }

    /* user name */
    if (ibuflen <= 1 + sizeof(len16)) {
        return send_reply(obj, AFPERR_PARAM);
    }

    type = *ibuf;
    username = ibuf;
    ibuf++;
    ibuflen--;

    if (type != 3) {
        return send_reply(obj, AFPERR_PARAM);
    }

    memcpy(&len16, ibuf, sizeof(len16));
    ibuf += sizeof(len16);
    ibuflen -= sizeof(len16);
    len = ntohs(len16);

    if (len > ibuflen) {
        return send_reply(obj, AFPERR_PARAM);
    }

    ibuf += len;
    ibuflen -= len;

    /* directory service name */
    if (!ibuflen) {
        return send_reply(obj, AFPERR_PARAM);
    }

    type = *ibuf;
    ibuf++;
    ibuflen--;

    switch (type) {
    case 1:
    case 2:
        if (!ibuflen) {
            return send_reply(obj, AFPERR_PARAM);
        }

        len = (unsigned char) * ibuf;
        ibuf++;
        ibuflen--;
        break;

    case 3:

        /* With "No User Authen" it is equal */
        if (ibuflen < sizeof(len16)) {
            return send_reply(obj, AFPERR_PARAM);
        }

        memcpy(&len16, ibuf, sizeof(len16));
        ibuf += sizeof(len16);
        ibuflen -= sizeof(len16);
        len = ntohs(len16);
        break;

    default:
        return send_reply(obj, AFPERR_PARAM);
    }

    if (ibuflen < len) {
        LOG(log_error, logtype_afpd,
            "login_ext: Login failed. Invalid directory service name!");
        return send_reply(obj, AFPERR_PARAM);
    }

    ibuf += len;
    ibuflen -= len;

    /* Pad */
    if (ibuflen && ((unsigned long) ibuf & 1)) { /* pad character */
        ibuf++;
        ibuflen--;
    }

    if (AFP_OK != (i = create_session_key(obj))) {
        return send_reply(obj, i);
    }

    /* FIXME user name are in UTF8 */
    i = afp_uam->u.uam_login.login_ext(obj, username, &pwd, ibuf, ibuflen, rbuf,
                                       rbuflen);

    if (i == AFPERR_AUTHCONT) {
        return send_reply(obj, i);
    }

    if (!pwd || (i != AFP_OK && i != AFPERR_PWDEXPR)) {
        return send_reply(obj, i);
    }

    return send_reply(obj, login(obj, pwd, afp_uam->u.uam_login.logout,
                                 ((i == AFPERR_PWDEXPR) ? 1 : 0)));
}

/*!
 * @brief FPLoginCont: pass the next login step to the UAM that FPLogin selected
 *
 * Without a selected UAM or a logincont hook, the call answers AFPERR_NOTAUTH
 * and ends the session. An accepted user goes through login.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFP_OK or AFPERR_AUTHCONT, and after a failure AFP_OK when the
 *          session exit returns
 */
int afp_logincont(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                  size_t *rbuflen)
{
    struct passwd *pwd = NULL;
    int err;

    if (afp_uam == NULL || afp_uam->u.uam_login.logincont == NULL || ibuflen < 2) {
        *rbuflen = 0;
        return send_reply(obj, AFPERR_NOTAUTH);
    }

    ibuf += 2;
    ibuflen -= 2;
    err = afp_uam->u.uam_login.logincont(obj, &pwd, ibuf, ibuflen,
                                         rbuf, rbuflen);

    if (!pwd || (err != AFP_OK && err != AFPERR_PWDEXPR)) {
        return send_reply(obj, err);
    }

    return send_reply(obj, login(obj, pwd, afp_uam->u.uam_login.logout,
                                 ((err == AFPERR_PWDEXPR) ? 1 : 0)));
}

/*!
 * @brief FPLogout: close every fork and volume of the session
 *
 * A DSI session sets DSI_AFP_LOGGED_OUT, so the session ends at the next
 * failed receive, such as the client closing the connection. With FCE the
 * call also sends a logout event.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block, unused
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns AFP_OK
 */
int afp_logout(AFPObj *obj, char *ibuf _U_, size_t ibuflen  _U_,
               char *rbuf  _U_, size_t *rbuflen)
{
    LOG(log_note, logtype_afpd, "AFP logout by %s", obj->username);
    of_close_all_forks(obj);
    close_all_vol(obj);

    if (obj->proto == AFPPROTO_DSI) {
        DSI *dsi = obj->dsi;
        dsi->flags = DSI_AFP_LOGGED_OUT;
    }

    *rbuflen = 0;
#ifdef WITH_FCE
    /* Send FCE logout event */
    fce_register(obj, FCE_LOGOUT, "", NULL);
#endif /* WITH_FCE */
    return AFP_OK;
}


/*!
 * @brief FPChangePassword: pass a password change to the UAM that the request
 * names
 *
 * The set password option must be on. Before AFP 3.0 the request carries the
 * user name. Later versions send an empty name, and the call uses the login
 * name. A successful change installs the full command table, which ends the
 * expired-password mode.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns the UAM result, AFPERR_NOOP when the option is off, AFPERR_BADUAM,
 *          or AFPERR_PARAM
 */
int afp_changepw(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                 size_t *rbuflen)
{
    char username[MAXUSERLEN], *start = ibuf;
    struct uam_obj *uam;
    struct passwd *pwd;
    size_t len;
    int    ret;
    *rbuflen = 0;
    ibuf += 2;

    /* check if password change is allowed, OS-X ignores the flag.
     * we shouldn't trust the client on this anyway.
     * not sure about the "right" error code, NOOP for now */
    if (!(obj->options.passwdbits & PASSWD_SET)) {
        return AFPERR_NOOP;
    }

    /* make sure we can deal w/ this uam */
    len = (unsigned char) * ibuf++;

    if ((uam = auth_uamfind(UAM_SERVER_CHANGEPW, ibuf, len)) == NULL) {
        return AFPERR_BADUAM;
    }

    ibuf += len;

    if ((len + 1) & 1) { /* pad byte */
        ibuf++;
    }

    if (obj->afp_version < 30) {
        len = (unsigned char) * ibuf++;

        if (len > sizeof(username) - 1) {
            return AFPERR_PARAM;
        }

        memcpy(username, ibuf, len);
        username[len] = '\0';
        ibuf += len;

        if ((len + 1) & 1) { /* pad byte */
            ibuf++;
        }
    } else {
        /* AFP > 3.0 doesn't pass the username, APF 3.1 specs page 124 */
        if (ibuf[0] != '\0' || ibuf[1] != '\0') {
            return AFPERR_PARAM;
        }

        ibuf += 2;
        len = MIN(sizeof(username) - 1, strlen(obj->username));
        memcpy(username, obj->username, len);
        username[len] = '\0';
    }

    LOG(log_info, logtype_afpd, "changing password for <%s>", username);

    if ((pwd = uam_getname(obj, username, sizeof(username))) == NULL) {
        return AFPERR_PARAM;
    }

    /* send it off to the uam. we really don't use ibuflen right now. */
    if (ibuflen < (size_t)(ibuf - start)) {
        return AFPERR_PARAM;
    }

    ibuflen -= (ibuf - start);
    ret = uam->u.uam_changepw(obj, username, pwd, ibuf, ibuflen,
                              rbuf, rbuflen);
    LOG(log_info, logtype_afpd, "password change %s.",
        (ret == AFPERR_AUTHCONT) ? "continued" :
        (ret ? "failed" : "succeeded"));

    if (ret == AFP_OK) {
        set_auth_switch(obj, 0);
    }

    return ret;
}


/*!
 * @brief FPGetUserInfo: return the user ID, the group ID, or the UUID of the
 * session user
 *
 * Only the session user can be asked about, so a request without the
 * this-user flag gets AFPERR_PARAM. The user ID is the euid at login, and the
 * group ID is the current egid. Without OPTION_UUID the reply drops the UUID
 * bit from its bitmap.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFP_OK, AFPERR_PARAM, AFPERR_BITMAP for unknown bits, or
 *          AFPERR_NOITEM when the UUID lookup fails
 */
int afp_getuserinfo(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf,
                    size_t *rbuflen)
{
    uint8_t  thisuser;
    uint32_t id;
    uint16_t bitmap;
    char *bitmapp;
    LOG(log_debug, logtype_afpd, "begin afp_getuserinfo:");
    *rbuflen = 0;
    ibuf++;
    thisuser = *ibuf++;
    ibuf += sizeof(id); /* userid is not used in AFP 2.0 */
    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);

    /* deal with error cases. we don't have to worry about
     * AFPERR_ACCESS or AFPERR_NOITEM as geteuid and getegid always
     * succeed. */
    if (!thisuser) {
        return AFPERR_PARAM;
    }

    if ((bitmap & USERIBIT_ALL) != bitmap) {
        return AFPERR_BITMAP;
    }

    /* remember place where we store the possibly modified bitmap later */
    memcpy(rbuf, ibuf, sizeof(bitmap));
    bitmapp = rbuf;
    rbuf += sizeof(bitmap);
    *rbuflen = sizeof(bitmap);

    /* copy the user/group info */
    if (bitmap & USERIBIT_USER) {
        id = htonl(obj->euid);
        memcpy(rbuf, &id, sizeof(id));
        rbuf += sizeof(id);
        *rbuflen += sizeof(id);
    }

    if (bitmap & USERIBIT_GROUP) {
        id = htonl(getegid());
        memcpy(rbuf, &id, sizeof(id));
        rbuf += sizeof(id);
        *rbuflen += sizeof(id);
    }

    if (bitmap & USERIBIT_UUID) {
        if (!(obj->options.flags & OPTION_UUID)) {
            bitmap &= ~USERIBIT_UUID;
            bitmap = htons(bitmap);
            memcpy(bitmapp, &bitmap, sizeof(bitmap));
        } else {
            LOG(log_debug, logtype_afpd, "afp_getuserinfo: get UUID for \'%s\'",
                obj->username);
            int ret;
            atalk_uuid_t uuid;
            ret = getuuidfromname(obj->username, UUID_USER, uuid);

            if (ret != 0) {
                LOG(log_info, logtype_afpd, "afp_getuserinfo: error getting UUID !");
                return AFPERR_NOITEM;
            }

            LOG(log_debug, logtype_afpd, "afp_getuserinfo: got UUID: %s",
                uuid_bin2string(uuid));
            memcpy(rbuf, uuid, UUID_BINSIZE);
            rbuf += UUID_BINSIZE;
            *rbuflen += UUID_BINSIZE;
        }
    }

    LOG(log_debug, logtype_afpd, "END afp_getuserinfo:");
    return AFP_OK;
}

#define UAM_LIST(type) (((type) == UAM_SERVER_LOGIN || (type) == UAM_SERVER_LOGIN_EXT) ? &uam_login : \
                        (((type) == UAM_SERVER_CHANGEPW) ?              \
                         &uam_changepw : NULL))

/*!
 * @brief Find a registered UAM by name
 *
 * In afpd the login types share one list, and the password change type has a
 * second. The comparison runs strndiacasecmp over len bytes, which ignores
 * case, so a prefix of a UAM name also matches.
 *
 * @param[in] type  UAM type: in afpd UAM_SERVER_LOGIN, UAM_SERVER_LOGIN_EXT, or
 *                  UAM_SERVER_CHANGEPW, and in papd UAM_SERVER_LOGIN,
 *                  UAM_SERVER_CHANGEPW, or UAM_SERVER_PRINTAUTH
 * @param[in] name  UAM name, not NUL-terminated
 * @param[in] len   length of name
 * @returns the UAM, or NULL for an unknown type or name
 */
struct uam_obj *auth_uamfind(const int type, const char *name,
                             const int len)
{
    struct uam_obj *prev, *start;

    if (!name || !(start = UAM_LIST(type))) {
        return NULL;
    }

    prev = start;

    while ((prev = prev->uam_prev) != start)
        if (strndiacasecmp(prev->uam_name, name, len) == 0) {
            return prev;
        }

    return NULL;
}

/*!
 * @brief Add a UAM to the list of its type
 *
 * @param[in] type  UAM type: in afpd UAM_SERVER_LOGIN, UAM_SERVER_LOGIN_EXT, or
 *                  UAM_SERVER_CHANGEPW, and in papd UAM_SERVER_LOGIN,
 *                  UAM_SERVER_CHANGEPW, or UAM_SERVER_PRINTAUTH
 * @param[in] uam   UAM with a nonempty name
 * @returns 0, -1 for a UAM without a name, or 1 for an unknown type, after
 *          which the caller frees uam
 */
int auth_register(const int type, struct uam_obj *uam)
{
    struct uam_obj *start;

    if (!uam || !uam->uam_name || (*uam->uam_name == '\0')) {
        return -1;
    }

    if (!(start = UAM_LIST(type))) {
        return 1;    /* we don't know what to do with it, caller must free it */
    }

    uam_attach(start, uam);
    return 0;
}

/*!
 * @brief Load the UAM modules that the uam list option names
 *
 * Each name in list, separated by commas or spaces, loads from the UAM
 * directory. A module that is missing or fails to load only gets a log line.
 *
 * @param[in] obj   server object
 * @param[in] path  UAM directory
 * @param[in] list  uam list option of afp.conf
 * @returns 0, or -1 for an empty path or list or a path that is too long
 */
int auth_load(AFPObj *obj, const char *path, const char *list)
{
    char name[MAXPATHLEN + 1], buf[MAXPATHLEN + 1], *p, *last;
    struct uam_mod *mod;
    struct stat st;
    size_t len;

    if (!path || !*path || !list || (len = strlen(path)) > sizeof(name) - 2) {
        return -1;
    }

    LOG(log_debug, logtype_afpd, "auth_load: %s, %s", path, list);
    strlcpy(buf, list, sizeof(buf));

    if ((p = strtok_r(buf, ", ", &last)) == NULL) {
        return -1;
    }

    strlcpy(name, path, sizeof(name));

    if (name[len - 1] != '/') {
        strlcat(name, "/", sizeof(name));
        len++;
    }

    while (p) {
        strlcpy(name + len, p, sizeof(name) - len);
        LOG(log_debug, logtype_afpd, "uam: loading (%s)", name);

        if (stat(name, &st) == 0) {
            if ((mod = uam_load(obj, name, p))) {
                uam_attach(&uam_modules, mod);
                LOG(log_debug, logtype_afpd, "uam: %s loaded", p);
            } else {
                LOG(log_error, logtype_afpd, "uam: %s load failure", p);
            }
        } else {
            LOG(log_info, logtype_afpd, "uam: uam not found (%s)", strerror(errno));
        }

        p = strtok_r(NULL, ", ", &last);
    }

    return 0;
}

/*! get rid of all of the uams */
void auth_unload(void)
{
    struct uam_mod *mod, *prev, *start = &uam_modules;
    prev = start->uam_prev;

    while ((mod = prev) != start) {
        prev = prev->uam_prev;
        uam_detach(mod);
        uam_unload(mod);
    }
}
