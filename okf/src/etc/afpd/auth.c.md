---
type: C Source File
title: "etc/afpd/auth.c"
description: "AFP login, logout, password, and session token commands, and UAM loading and dispatch."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/auth.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [acls.h](acls.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/spotlight.h](../../include/atalk/spotlight.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/uuid.h](../../include/atalk/uuid.h.md)
* [auth.h](auth.h.md)
* [extattrs.h](extattrs.h.md)
* [fork.h](fork.h.md)
* [status.h](status.h.md)
* [switch.h](switch.h.md)
* [uam_auth.h](uam_auth.h.md)
* System headers: `arpa/inet.h`, `ctype.h`, `errno.h`, `grp.h`, `limits.h`, `netdb.h`, `pwd.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/stat.h`, `sys/types.h`, `time.h`, `unistd.h`

# Functions

### sockaddr_len

```c
static socklen_t sockaddr_len(const struct sockaddr *sa)
```

Defined at lines 87 to 99.

Return the size of the socket address structure for the family of an address.

Parameters:
* `sa`: socket address

Returns: the size of sockaddr_in or sockaddr_in6, or of sockaddr_storage for another family

Called by: [client_address](auth.c.md#client_address)

### client_address

```c
static const char * client_address(AFPObj *obj)
```

Defined at lines 110 to 137.

Name the client of a session for the login report to the master afpd.

A [DSI](../../include/atalk/dsi.h.md#struct-dsi) client gets its DNS name, or its numeric address when the reverse lookup fails. An [ASP](../../include/atalk/asp.h.md#struct-asp) client gets its AppleTalk network and node numbers.

Parameters:
* `obj`: session object

Returns: the name in a static buffer, or "-" for an unknown transport

Calls: [getip_string](../../libatalk/util/socket.c.md#getip_string), [sockaddr_len](auth.c.md#sockaddr_len)

Called by: [login](auth.c.md#login)

### status_versions

```c
void status_versions(char *data, const ASP asp, const DSI *dsi)
```

Defined at lines 152 to 204.

Write the AFP version list of the FPGetSrvrInfo reply into a status block.

The list starts at the offset stored at AFPSTATUS_VERSOFF. Versions up to 2.1 need an [ASP](../../include/atalk/asp.h.md#struct-asp) listener, and versions after 2.2 need a [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener, so both transports list 2.2. The call stores the end of the list as the UAM list offset.

Parameters:
* `data`: status block
* `asp`: [ASP](../../include/atalk/asp.h.md#struct-asp) listener, or NULL for the [DSI](../../include/atalk/dsi.h.md#struct-dsi) status
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener, or NULL for the [ASP](../../include/atalk/asp.h.md#struct-asp) status

Called by: [status_init](status.c.md#status_init)

### status_uams

```c
void status_uams(char *data, const char *authlist)
```

Defined at lines 216 to 247.

Write the UAM name list of the FPGetSrvrInfo reply into a status block.

The list holds every loaded UAM whose module path appears in authlist, and the call logs each as available. The list starts at the offset stored at AFPSTATUS_UAMSOFF, and the call stores its end as the icon offset.

Parameters:
* `data`: status block
* `authlist`: uam list option of afp.conf

Called by: [status_init](status.c.md#status_init)

Uses file-scope variables: `uam_login`

### send_reply

```c
static int send_reply(const AFPObj *obj, const int err)
```

Defined at lines 260 to 274.

Pass a login result on, or answer a failed login and end the session.

AFP_OK and AFPERR_AUTHCONT go back to the caller unchanged. Any other code goes to the client as the reply, and the session then exits through obj->exit.

Parameters:
* `obj`: session object
* `err`: result of the login step

Returns: err for AFP_OK and AFPERR_AUTHCONT, or AFP_OK when obj->exit returns

Called by: [afp_login](auth.c.md#afp_login), [afp_login_ext](auth.c.md#afp_login_ext), [afp_logincont](auth.c.md#afp_logincont)

### afp_errpwdexpired

```c
static int afp_errpwdexpired(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 286 to 292.

Answer a command with AFPERR_PWDEXPR while a password change is due.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block, unused
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: AFPERR_PWDEXPR

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

### afp_null_nolog

```c
static int afp_null_nolog(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 304 to 309.

Answer a command with AFPERR_NOOP and no log line.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block, unused
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: AFPERR_NOOP

### set_auth_switch

```c
static int set_auth_switch(const AFPObj *obj, int expired)
```

Defined at lines 332 to 410.

Install the command table of a logged-in session.

A valid login switches to the postauth table and adds the commands of the negotiated AFP version. Each version also gets the commands of the versions below it:

* 2.2 adds FPZzzzz.
* 3.0 adds FPEnumerateExt, FPByteRangeLockExt, FPCatSearchExt, FPReadExt, FPWriteExt, FPGetSessionToken, and FPDisconnectOldSession.
* 3.1 adds FPSyncDir, FPSyncFork, FPEnumerateExt2, and the Spotlight command.
* 3.2 adds the extended attribute commands and, with ACL support, the ACL commands.

An expired password keeps the preauth table. Every command then answers AFPERR_PWDEXPR, except FPLogout and FPChangePassword.

Parameters:
* `obj`: session object
* `expired`: nonzero when the UAM reported an expired password

Returns: AFP_OK

Calls: [afp_access](acls.c.md#afp_access), [afp_bytelock_ext](fork.c.md#afp_bytelock_ext), [afp_catsearch_ext](catsearch.c.md#afp_catsearch_ext), [afp_changepw](auth.c.md#afp_changepw), [afp_disconnect](auth.c.md#afp_disconnect), [afp_enumerate_ext](enumerate.c.md#afp_enumerate_ext), [afp_enumerate_ext2](enumerate.c.md#afp_enumerate_ext2), [afp_errpwdexpired](auth.c.md#afp_errpwdexpired), [afp_getacl](acls.c.md#afp_getacl), [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_getsession](auth.c.md#afp_getsession), [afp_listextattr](extattrs.c.md#afp_listextattr), [afp_logout](auth.c.md#afp_logout), [afp_read_ext](fork.c.md#afp_read_ext), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_setacl](acls.c.md#afp_setacl), [afp_setextattr](extattrs.c.md#afp_setextattr), [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc), [afp_syncdir](directory.c.md#afp_syncdir), [afp_syncfork](fork.c.md#afp_syncfork), [afp_write_ext](fork.c.md#afp_write_ext), [afp_zzzzz](auth.c.md#afp_zzzzz), [uam_afpserver_action](switch.c.md#uam_afpserver_action)

Called by: [afp_changepw](auth.c.md#afp_changepw), [login](auth.c.md#login)

Uses file-scope variables: `afp_switch` in [etc/afpd/switch.c](switch.c.md), `postauth_switch` in [etc/afpd/switch.c](switch.c.md)

### singleuser_admits_uid

```c
bool singleuser_admits_uid(const AFPObj *obj, uid_t uid)
```

Defined at lines 423 to 426.

Tell whether the server may serve a user.

A single-user server cannot switch to another account, so only the uid that started it may log in. A server without OPTION_SINGLEUSER serves every user that login admits.

Parameters:
* `obj`: session object
* `uid`: uid of the user who logs in

Returns: true when the server may serve uid

Called by: [login](auth.c.md#login)

### login

```c
static int login(AFPObj *obj, struct passwd *pwd, void(*logout)(void), int expired)
```

Defined at lines 446 to 550.

Finish a login after the UAM accepted the user.

The call refuses root, a user that a single-user server may not serve, and a login past the connection limit. It sets the groups and switches the euid and egid to the user. A member of the admin group keeps the root euid instead. The force user and force group options then override the switch.

Next the call installs the command table and reports the login with the client address to the master afpd. It resets the umask, because PAM may change it. A [DSI](../../include/atalk/dsi.h.md#struct-dsi) session also installs its signal handlers again.

Parameters:
* `obj`: session object
* `pwd`: password entry of the user
* `logout`: logout hook of the UAM, or NULL
* `expired`: nonzero when the password expired

Returns: AFP_OK, AFPERR_NOTAUTH, AFPERR_MAXSESS, or AFPERR_BADUAM

Calls: [ad_setfuid](../../libatalk/adouble/ad_open.c.md#ad_setfuid), [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers), [client_address](auth.c.md#client_address), [fce_register](fce_api.c.md#fce_register), [ipc_child_state](../../libatalk/util/server_ipc.c.md#ipc_child_state), [ipc_child_write](../../libatalk/util/server_ipc.c.md#ipc_child_write), [print_groups](../../libatalk/util/unix.c.md#print_groups), [set_auth_switch](auth.c.md#set_auth_switch), [set_groups](../../libatalk/util/unix.c.md#set_groups), [singleuser_admits_uid](auth.c.md#singleuser_admits_uid), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_login](auth.c.md#afp_login), [afp_login_ext](auth.c.md#afp_login_ext), [afp_logincont](auth.c.md#afp_logincont)

Uses file-scope variables: `afp_version_index`

Mentioned in the documentation of: [run_afpd](../netatalk/netatalk.c.md#run_afpd)

### afp_zzzzz

```c
int afp_zzzzz(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 569 to 629.

FPZzzzz: put the [DSI](../../include/atalk/dsi.h.md#struct-dsi) session to sleep or wake it from extended sleep.

A sleep request sets DSI_SLEEPING and closes the recvfile pipe, and the extended sleep flag adds DSI_EXTSLEEP. The extended wakeup flag ends an extended sleep. Each change of state goes to the master afpd.

The reply holds the sleep option divided by 120, at least 1. That gives the hours a sleeping session lasts at the default tickleval of 30 seconds.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFP_OK, or AFPERR_MISC for a request under 6 bytes

Calls: [dsi_close_pipe](../../libatalk/dsi/dsi_write.c.md#dsi_close_pipe), [ipc_child_state](../../libatalk/util/server_ipc.c.md#ipc_child_state)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### create_session_token

```c
static int create_session_token(AFPObj *obj)
```

Defined at lines 637 to 652.

Allocate the session token and fill it with random bytes.

Parameters:
* `obj`: session object that receives the token

Returns: 0, or AFPERR_MISC when the allocation or the random source fails

Calls: [uam_random_string](uam.c.md#uam_random_string)

Called by: [afp_getsession](auth.c.md#afp_getsession)

### create_session_key

```c
static int create_session_key(AFPObj *obj)
```

Defined at lines 664 to 682.

Allocate sinfo.sessionkey once and fill it with random bytes.

The GSS UAM encrypts it with gss_wrap for its login reply. A session that has one keeps it.

Parameters:
* `obj`: session object that receives sessionkey

Returns: AFP_OK, or AFPERR_MISC when the allocation or the random source fails

Calls: [uam_random_string](uam.c.md#uam_random_string)

Called by: [afp_login](auth.c.md#afp_login), [afp_login_ext](auth.c.md#afp_login_ext)

### afp_getsession

```c
int afp_getsession(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 706 to 828.

FPGetSessionToken: return a token that a reconnecting client can present.

The session creates its random token on the first call and hands it to the master afpd. Types 0 to 2 return the token. Type 8 returns the encrypted Kerberos session key instead.

Types 3 and 4 also carry a client ID and a boot time, which go to the master as well. The master then ends an older session of the same client and user whose boot time differs, because that client rebooted. The reply holds a 4-byte length and the token.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFP_OK, AFPERR_NOOP for an unknown type, AFPERR_PARAM for a short request, or AFPERR_MISC

Calls: [create_session_token](auth.c.md#create_session_token), [ipc_child_write](../../libatalk/util/server_ipc.c.md#ipc_child_write)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

### afp_disconnect

```c
int afp_disconnect(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 851 to 957.

FPDisconnectOldSession: hand the connection to the disconnected session that owns a token.

A client sends it on a new connection to resume a disconnected session. The call stops the tickle timer and sends the token, the [DSI](../../include/atalk/dsi.h.md#struct-dsi) request ID, and the client socket to the master afpd. The master passes them to the old session, which answers the request, and sends SIGTERM to this one. A SIGTERM within the 5-second wait marks success, and the process exits.

A failed handoff restarts the timer and leaves DSI_RECONINPROG set, so the next connection error ends this session.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: AFPERR_MISC when the token length is wrong or the handoff fails, or AFPERR_PARAM for a short request

Calls: [ipc_child_write](../../libatalk/util/server_ipc.c.md#ipc_child_write), [send_fd](../../libatalk/util/socket.c.md#send_fd), [writet](../../libatalk/util/socket.c.md#writet)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Uses file-scope variables: `die_pending` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### get_version

```c
static int get_version(AFPObj *obj, char *ibuf, size_t ibuflen, size_t len)
```

Defined at lines 973 to 1003.

Select the AFP version that a login request names.

The call compares the first len bytes of the request with each known version string, so a prefix of one also matches. A version from 3.0 on needs a 64-bit off_t.

Parameters:
* `obj`: session object that receives afp_version
* `ibuf`: version string of the request
* `ibuflen`: bytes left in the request
* `len`: length of the version string

Returns: 0, or AFPERR_BADVERS for an empty, oversized, unknown, or unsupported version

Called by: [afp_login](auth.c.md#afp_login), [afp_login_ext](auth.c.md#afp_login_ext)

Uses file-scope variables: `afp_version_index`

### afp_login

```c
int afp_login(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1023 to 1092.

FPLogin: select the AFP version and the UAM, and run the first login step.

The request holds the version string and the UAM name as Pascal strings, then the data of the UAM. A UAM that needs another round trip answers AFPERR_AUTHCONT, and FPLoginCont continues. An accepted user goes through login. Every other failure answers the client and ends the session. After the master announced a shutdown, the failure is AFPERR_SHUTDOWN.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFP_OK or AFPERR_AUTHCONT, and after a failure AFP_OK when the session exit returns

Calls: [auth_uamfind](auth.c.md#auth_uamfind), [create_session_key](auth.c.md#create_session_key), [get_version](auth.c.md#get_version), [login](auth.c.md#login), [send_reply](auth.c.md#send_reply)

Uses file-scope variables: `afp_uam`, `nologin` in [etc/afpd/main.c](main.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch), [preauth_switch](switch.c.md#preauth_switch)

### afp_login_ext

```c
int afp_login_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1110 to 1269.

FPLoginExt: select the AFP version and the UAM, and run the first login step with a UTF-8 user name.

The request adds flags, a user name of type 3, and a directory service name to the fields of FPLogin. The UAM must offer login_ext, and it gets the user name with its type byte. The results match those of afp_login.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFP_OK or AFPERR_AUTHCONT, and after a failure AFP_OK when the session exit returns

Calls: [auth_uamfind](auth.c.md#auth_uamfind), [create_session_key](auth.c.md#create_session_key), [get_version](auth.c.md#get_version), [login](auth.c.md#login), [send_reply](auth.c.md#send_reply)

Uses file-scope variables: `afp_uam`, `nologin` in [etc/afpd/main.c](main.c.md)

Dispatched via: [preauth_switch](switch.c.md#preauth_switch)

### afp_logincont

```c
int afp_logincont(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1285 to 1307.

FPLoginCont: pass the next login step to the UAM that FPLogin selected.

Without a selected UAM or a logincont hook, the call answers AFPERR_NOTAUTH and ends the session. An accepted user goes through login.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFP_OK or AFPERR_AUTHCONT, and after a failure AFP_OK when the session exit returns

Calls: [login](auth.c.md#login), [send_reply](auth.c.md#send_reply)

Uses file-scope variables: `afp_uam`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch), [preauth_switch](switch.c.md#preauth_switch)

### afp_logout

```c
int afp_logout(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1323 to 1341.

FPLogout: close every fork and volume of the session.

A [DSI](../../include/atalk/dsi.h.md#struct-dsi) session sets DSI_AFP_LOGGED_OUT, so the session ends at the next failed receive, such as the client closing the connection. With FCE the call also sends a logout event.

Parameters:
* `obj`: session object
* `ibuf`: request block, unused
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: AFP_OK

Calls: [close_all_vol](volume.c.md#close_all_vol), [fce_register](fce_api.c.md#fce_register), [of_close_all_forks](ofork.c.md#of_close_all_forks)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch), [preauth_switch](switch.c.md#preauth_switch)

### afp_changepw

```c
int afp_changepw(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1361 to 1441.

FPChangePassword: pass a password change to the UAM that the request names.

The set password option must be on. Before AFP 3.0 the request carries the user name. Later versions send an empty name, and the call uses the login name. A successful change installs the full command table, which ends the expired-password mode.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the UAM result, AFPERR_NOOP when the option is off, AFPERR_BADUAM, or AFPERR_PARAM

Calls: [auth_uamfind](auth.c.md#auth_uamfind), [set_auth_switch](auth.c.md#set_auth_switch), [uam_getname](uam.c.md#uam_getname)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`uam_obj::uam_changepw`](uam_auth.h.md#struct-uam_obj): no table assigns this field

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_getuserinfo

```c
int afp_getuserinfo(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1461 to 1535.

FPGetUserInfo: return the user ID, the group ID, or the UUID of the session user.

Only the session user can be asked about, so a request without the this-user flag gets AFPERR_PARAM. The user ID is the euid at login, and the group ID is the current egid. Without OPTION_UUID the reply drops the UUID bit from its bitmap.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFP_OK, AFPERR_PARAM, AFPERR_BITMAP for unknown bits, or AFPERR_NOITEM when the UUID lookup fails

Calls: [getuuidfromname](../../libatalk/acl/uuid.c.md#getuuidfromname), [uuid_bin2string](../../libatalk/acl/uuid.c.md#uuid_bin2string)

Uses file-scope variables: `uuid` in [etc/afpd/auth.h](auth.h.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### auth_uamfind

```c
struct uam_obj * auth_uamfind(const int type, const char *name, const int len)
```

Defined at lines 1555 to 1572.

Find a registered UAM by name.

In afpd the login types share one list, and the password change type has a second. The comparison runs strndiacasecmp over len bytes, which ignores case, so a prefix of a UAM name also matches.

Parameters:
* `type`: UAM type: in afpd UAM_SERVER_LOGIN, UAM_SERVER_LOGIN_EXT, or UAM_SERVER_CHANGEPW, and in papd UAM_SERVER_LOGIN, UAM_SERVER_CHANGEPW, or UAM_SERVER_PRINTAUTH
* `name`: UAM name, not NUL-terminated
* `len`: length of name

Returns: the UAM, or NULL for an unknown type or name

Calls: [strndiacasecmp](../../libatalk/util/strdicasecmp.c.md#strndiacasecmp)

Called by: [afp_changepw](auth.c.md#afp_changepw), [afp_login](auth.c.md#afp_login), [afp_login_ext](auth.c.md#afp_login_ext), [uam_gss_enabled](status.c.md#uam_gss_enabled), [uam_register](uam.c.md#uam_register), [uam_unregister](uam.c.md#uam_unregister)

### auth_register

```c
int auth_register(const int type, struct uam_obj *uam)
```

Defined at lines 1584 to 1598.

Add a UAM to the list of its type.

Parameters:
* `type`: UAM type: in afpd UAM_SERVER_LOGIN, UAM_SERVER_LOGIN_EXT, or UAM_SERVER_CHANGEPW, and in papd UAM_SERVER_LOGIN, UAM_SERVER_CHANGEPW, or UAM_SERVER_PRINTAUTH
* `uam`: UAM with a nonempty name

Returns: 0, -1 for a UAM without a name, or 1 for an unknown type, after which the caller frees uam

Called by: [uam_register](uam.c.md#uam_register)

### auth_load

```c
int auth_load(AFPObj *obj, const char *path, const char *list)
```

Defined at lines 1611 to 1655.

Load the UAM modules that the uam list option names.

Each name in list, separated by commas or spaces, loads from the UAM directory. A module that is missing or fails to load only gets a log line.

Parameters:
* `obj`: server object
* `path`: UAM directory
* `list`: uam list option of afp.conf

Returns: 0, or -1 for an empty path or list or a path that is too long

Calls: [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [uam_load](uam.c.md#uam_load)

Called by: [configinit](afp_config.c.md#configinit)

Uses file-scope variables: `uam_modules`

### auth_unload

```c
void auth_unload(void)
```

Defined at lines 1658 to 1668.

get rid of all of the uams

Calls: [uam_unload](uam.c.md#uam_unload)

Called by: [afp_goaway](main.c.md#afp_goaway), [configfree](afp_config.c.md#configfree)

Uses file-scope variables: `uam_modules`

# Macros

* Undocumented: `UAM_LIST`

# File-scope variables

`afp_uam`, `afp_version_index`, `uam_changepw`, `uam_login`, `uam_modules`
