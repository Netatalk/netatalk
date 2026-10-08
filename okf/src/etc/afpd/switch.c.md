---
type: C Source File
title: "etc/afpd/switch.c"
description: "2 functions, includes 14 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/switch.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [acls.h](acls.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/uam.h](../../include/atalk/uam.h.md)
* [auth.h](auth.h.md)
* [desktop.h](desktop.h.md)
* [directory.h](directory.h.md)
* [file.h](file.h.md)
* [filedir.h](filedir.h.md)
* [fork.h](fork.h.md)
* [misc.h](misc.h.md)
* [status.h](status.h.md)
* [switch.h](switch.h.md)
* System headers: `stdio.h`, `sys/stat.h`

# Function tables

### postauth_switch

Initialized at line 133. Dispatches to: [afp_addappl](appl.c.md#afp_addappl), [afp_addcomment](desktop.c.md#afp_addcomment), [afp_addicon](desktop.c.md#afp_addicon), [afp_bytelock](fork.c.md#afp_bytelock), [afp_catsearch](catsearch.c.md#afp_catsearch), [afp_changepw](auth.c.md#afp_changepw), [afp_closedir](directory.c.md#afp_closedir), [afp_closedt](desktop.c.md#afp_closedt), [afp_closefork](fork.c.md#afp_closefork), [afp_closevol](volume.c.md#afp_closevol), [afp_copyfile](file.c.md#afp_copyfile), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_createid](file.c.md#afp_createid), [afp_delete](filedir.c.md#afp_delete), [afp_deleteid](file.c.md#afp_deleteid), [afp_enumerate](enumerate.c.md#afp_enumerate), [afp_exchangefiles](file.c.md#afp_exchangefiles), [afp_flush](fork.c.md#afp_flush), [afp_flushfork](fork.c.md#afp_flushfork), [afp_getappl](appl.c.md#afp_getappl), [afp_getcomment](desktop.c.md#afp_getcomment), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_getforkparams](fork.c.md#afp_getforkparams), [afp_geticon](desktop.c.md#afp_geticon), [afp_geticoninfo](desktop.c.md#afp_geticoninfo), [afp_getsrvrinfo](status.c.md#afp_getsrvrinfo), [afp_getsrvrmesg](messages.c.md#afp_getsrvrmesg), [afp_getsrvrparms](volume.c.md#afp_getsrvrparms), [afp_getuserinfo](auth.c.md#afp_getuserinfo), [afp_getvolparams](volume.c.md#afp_getvolparams), [afp_login](auth.c.md#afp_login), [afp_logincont](auth.c.md#afp_logincont), [afp_logout](auth.c.md#afp_logout), [afp_mapid](directory.c.md#afp_mapid), [afp_mapname](directory.c.md#afp_mapname), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_null](switch.c.md#afp_null), [afp_opendir](directory.c.md#afp_opendir), [afp_opendt](desktop.c.md#afp_opendt), [afp_openfork](fork.c.md#afp_openfork), [afp_openvol](volume.c.md#afp_openvol), [afp_read](fork.c.md#afp_read), [afp_rename](filedir.c.md#afp_rename), [afp_resolveid](file.c.md#afp_resolveid), [afp_rmvappl](appl.c.md#afp_rmvappl), [afp_rmvcomment](desktop.c.md#afp_rmvcomment), [afp_setdirparams](directory.c.md#afp_setdirparams), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [afp_setfilparams](file.c.md#afp_setfilparams), [afp_setforkparams](fork.c.md#afp_setforkparams), [afp_setvolparams](volume.c.md#afp_setvolparams), [afp_syncdir](directory.c.md#afp_syncdir), [afp_syncfork](fork.c.md#afp_syncfork), [afp_write](fork.c.md#afp_write)

### preauth_switch

Initialized at line 64. Dispatches to: [afp_login](auth.c.md#afp_login), [afp_login_ext](auth.c.md#afp_login_ext), [afp_logincont](auth.c.md#afp_logincont), [afp_logout](auth.c.md#afp_logout)

# Functions

### afp_null

```c
static int afp_null(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 51 to 57.

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### uam_afpserver_action

```c
int uam_afpserver_action(const int id, const int which, AFPCmd new_table, AFPCmd *old)
```

Defined at lines 204 to 237.

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Uses file-scope variables: `postauth_switch`, `preauth_switch`

# File-scope variables

`afp_switch`
