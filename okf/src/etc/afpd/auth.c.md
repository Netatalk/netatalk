---
type: C Source File
title: "etc/afpd/auth.c"
description: "26 functions, includes 19 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/auth.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-09T22:45:51+11:00 }
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

Defined at lines 73 to 85.

Called by: [client_address](auth.c.md#client_address)

### client_address

```c
static const char * client_address(AFPObj *obj)
```

Defined at lines 87 to 114.

Calls: [getip_string](../../libatalk/util/socket.c.md#getip_string), [sockaddr_len](auth.c.md#sockaddr_len)

Called by: [login](auth.c.md#login)

### status_versions

```c
void status_versions(char *data, const ASP asp, const DSI *dsi)
```

Defined at lines 117 to 169.

Called by: [status_init](status.c.md#status_init)

### status_uams

```c
void status_uams(char *data, const char *authlist)
```

Defined at lines 171 to 202.

Called by: [status_init](status.c.md#status_init)

Uses file-scope variables: `uam_login`

### send_reply

```c
static int send_reply(const AFPObj *obj, const int err)
```

Defined at lines 206 to 220.

Called by: [afp_login](auth.c.md#afp_login), [afp_login_ext](auth.c.md#afp_login_ext), [afp_logincont](auth.c.md#afp_logincont)

Calls through [`AFPObj::exit`](../../include/atalk/globals.h.md#struct-afpobj): no table assigns this field

Calls through [`AFPObj::reply`](../../include/atalk/globals.h.md#struct-afpobj): no table assigns this field

### afp_errpwdexpired

```c
static int afp_errpwdexpired(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 222 to 228.

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

### afp_null_nolog

```c
static int afp_null_nolog(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 230 to 235.

### set_auth_switch

```c
static int set_auth_switch(const AFPObj *obj, int expired)
```

Defined at lines 237 to 315.

Calls: [afp_access](acls.c.md#afp_access), [afp_bytelock_ext](fork.c.md#afp_bytelock_ext), [afp_catsearch_ext](catsearch.c.md#afp_catsearch_ext), [afp_changepw](auth.c.md#afp_changepw), [afp_disconnect](auth.c.md#afp_disconnect), [afp_enumerate_ext](enumerate.c.md#afp_enumerate_ext), [afp_enumerate_ext2](enumerate.c.md#afp_enumerate_ext2), [afp_errpwdexpired](auth.c.md#afp_errpwdexpired), [afp_getacl](acls.c.md#afp_getacl), [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_getsession](auth.c.md#afp_getsession), [afp_listextattr](extattrs.c.md#afp_listextattr), [afp_logout](auth.c.md#afp_logout), [afp_read_ext](fork.c.md#afp_read_ext), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_setacl](acls.c.md#afp_setacl), [afp_setextattr](extattrs.c.md#afp_setextattr), [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc), [afp_syncdir](directory.c.md#afp_syncdir), [afp_syncfork](fork.c.md#afp_syncfork), [afp_write_ext](fork.c.md#afp_write_ext), [afp_zzzzz](auth.c.md#afp_zzzzz), [uam_afpserver_action](switch.c.md#uam_afpserver_action)

Called by: [afp_changepw](auth.c.md#afp_changepw), [login](auth.c.md#login)

Uses file-scope variables: `afp_switch` in [etc/afpd/switch.c](switch.c.md), `postauth_switch` in [etc/afpd/switch.c](switch.c.md)

### singleuser_admits_uid

```c
bool singleuser_admits_uid(const AFPObj *obj, uid_t uid)
```

Defined at lines 323 to 326.

Whether this server may serve uid.

A single-user server has no authority to adopt another account, so only the uid that started it logs in; a root service serves anyone [login()](auth.c.md#login) admits.

Called by: [login](auth.c.md#login)

### login

```c
static int login(AFPObj *obj, struct passwd *pwd, void(*logout)(void), int expired)
```

Defined at lines 328 to 432.

Calls: [ad_setfuid](../../libatalk/adouble/ad_open.c.md#ad_setfuid), [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers), [client_address](auth.c.md#client_address), [fce_register](fce_api.c.md#fce_register), [ipc_child_state](../../libatalk/util/server_ipc.c.md#ipc_child_state), [ipc_child_write](../../libatalk/util/server_ipc.c.md#ipc_child_write), [print_groups](../../libatalk/util/unix.c.md#print_groups), [set_auth_switch](auth.c.md#set_auth_switch), [set_groups](../../libatalk/util/unix.c.md#set_groups), [singleuser_admits_uid](auth.c.md#singleuser_admits_uid), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_login](auth.c.md#afp_login), [afp_login_ext](auth.c.md#afp_login_ext), [afp_logincont](auth.c.md#afp_logincont)

Uses file-scope variables: `afp_version_index`

Mentioned in the documentation of: [dsi_disconnect](../../libatalk/dsi/dsi_stream.c.md#dsi_disconnect), [run_afpd](../netatalk/netatalk.c.md#run_afpd), [singleuser_admits_uid](auth.c.md#singleuser_admits_uid)

### afp_zzzzz

```c
int afp_zzzzz(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 435 to 494.

Calls: [ipc_child_state](../../libatalk/util/server_ipc.c.md#ipc_child_state)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### create_session_token

```c
static int create_session_token(AFPObj *obj)
```

Defined at lines 497 to 512.

Calls: [uam_random_string](uam.c.md#uam_random_string)

Called by: [afp_getsession](auth.c.md#afp_getsession)

### create_session_key

```c
static int create_session_key(AFPObj *obj)
```

Defined at lines 514 to 532.

Calls: [uam_random_string](uam.c.md#uam_random_string)

Called by: [afp_login](auth.c.md#afp_login), [afp_login_ext](auth.c.md#afp_login_ext)

### afp_getsession

```c
int afp_getsession(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 536 to 658.

Calls: [create_session_token](auth.c.md#create_session_token), [ipc_child_write](../../libatalk/util/server_ipc.c.md#ipc_child_write)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

### afp_disconnect

```c
int afp_disconnect(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 661 to 767.

Calls: [ipc_child_write](../../libatalk/util/server_ipc.c.md#ipc_child_write), [send_fd](../../libatalk/util/socket.c.md#send_fd), [writet](../../libatalk/util/socket.c.md#writet)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Uses file-scope variables: `die_pending` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### get_version

```c
static int get_version(AFPObj *obj, char *ibuf, size_t ibuflen, size_t len)
```

Defined at lines 770 to 800.

Called by: [afp_login](auth.c.md#afp_login), [afp_login_ext](auth.c.md#afp_login_ext)

Uses file-scope variables: `afp_version_index`

### afp_login

```c
int afp_login(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 803 to 872.

Calls: [auth_uamfind](auth.c.md#auth_uamfind), [create_session_key](auth.c.md#create_session_key), [get_version](auth.c.md#get_version), [login](auth.c.md#login), [send_reply](auth.c.md#send_reply)

Calls through [`uam_obj::login`](uam_auth.h.md#struct-uam_obj): no table assigns this field

Uses file-scope variables: `afp_uam`, `nologin` in [etc/afpd/main.c](main.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch), [preauth_switch](switch.c.md#preauth_switch)

### afp_login_ext

```c
int afp_login_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 875 to 1034.

Calls: [auth_uamfind](auth.c.md#auth_uamfind), [create_session_key](auth.c.md#create_session_key), [get_version](auth.c.md#get_version), [login](auth.c.md#login), [send_reply](auth.c.md#send_reply)

Calls through [`uam_obj::login_ext`](uam_auth.h.md#struct-uam_obj): no table assigns this field

Uses file-scope variables: `afp_uam`, `nologin` in [etc/afpd/main.c](main.c.md)

Dispatched via: [preauth_switch](switch.c.md#preauth_switch)

### afp_logincont

```c
int afp_logincont(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1037 to 1059.

Calls: [login](auth.c.md#login), [send_reply](auth.c.md#send_reply)

Calls through [`uam_obj::logincont`](uam_auth.h.md#struct-uam_obj): no table assigns this field

Uses file-scope variables: `afp_uam`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch), [preauth_switch](switch.c.md#preauth_switch)

### afp_logout

```c
int afp_logout(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1062 to 1080.

Calls: [close_all_vol](volume.c.md#close_all_vol), [fce_register](fce_api.c.md#fce_register), [of_close_all_forks](ofork.c.md#of_close_all_forks)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch), [preauth_switch](switch.c.md#preauth_switch)

### afp_changepw

```c
int afp_changepw(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1088 to 1168.

Calls: [auth_uamfind](auth.c.md#auth_uamfind), [set_auth_switch](auth.c.md#set_auth_switch), [uam_getname](uam.c.md#uam_getname)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`uam_obj::uam_changepw`](uam_auth.h.md#struct-uam_obj): no table assigns this field

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_getuserinfo

```c
int afp_getuserinfo(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1172 to 1246.

Calls: [getuuidfromname](../../libatalk/acl/uuid.c.md#getuuidfromname), [uuid_bin2string](../../libatalk/acl/uuid.c.md#uuid_bin2string)

Uses file-scope variables: `uuid` in [etc/afpd/auth.h](auth.h.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### auth_uamfind

```c
struct uam_obj * auth_uamfind(const int type, const char *name, const int len)
```

Defined at lines 1254 to 1271.

Calls: [strndiacasecmp](../../libatalk/util/strdicasecmp.c.md#strndiacasecmp)

Called by: [afp_changepw](auth.c.md#afp_changepw), [afp_login](auth.c.md#afp_login), [afp_login_ext](auth.c.md#afp_login_ext), [uam_gss_enabled](status.c.md#uam_gss_enabled), [uam_register](uam.c.md#uam_register), [uam_unregister](uam.c.md#uam_unregister)

### auth_register

```c
int auth_register(const int type, struct uam_obj *uam)
```

Defined at lines 1273 to 1287.

Called by: [uam_register](uam.c.md#uam_register)

### auth_load

```c
int auth_load(AFPObj *obj, const char *path, const char *list)
```

Defined at lines 1290 to 1334.

load all of the modules

Calls: [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [uam_load](uam.c.md#uam_load)

Called by: [configinit](afp_config.c.md#configinit)

Uses file-scope variables: `uam_modules`

Mentioned in the documentation of: [srp_is_the_only_uam](../netatalk/netatalk.c.md#srp_is_the_only_uam)

### auth_unload

```c
void auth_unload(void)
```

Defined at lines 1337 to 1347.

get rid of all of the uams

Calls: [uam_unload](uam.c.md#uam_unload)

Called by: [afp_goaway](main.c.md#afp_goaway), [configfree](afp_config.c.md#configfree)

Uses file-scope variables: `uam_modules`

# Macros

* Undocumented: `UAM_LIST`

# File-scope variables

`afp_uam`, `afp_version_index`, `uam_changepw`, `uam_login`, `uam_modules`
