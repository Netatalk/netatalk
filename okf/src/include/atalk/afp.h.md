---
type: C Header File
title: "include/atalk/afp.h"
description: "No functions or types."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/afp.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `stdint.h`, `sys/types.h`

# Included by

* [etc/afpd/acls.c](../../etc/afpd/acls.c.md)
* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/afpd/appl.c](../../etc/afpd/appl.c.md)
* [etc/afpd/auth.c](../../etc/afpd/auth.c.md)
* [etc/afpd/catsearch.c](../../etc/afpd/catsearch.c.md)
* [etc/afpd/desktop.c](../../etc/afpd/desktop.c.md)
* [etc/afpd/directory.c](../../etc/afpd/directory.c.md)
* [etc/afpd/enumerate.c](../../etc/afpd/enumerate.c.md)
* [etc/afpd/extattrs.c](../../etc/afpd/extattrs.c.md)
* [etc/afpd/fce_api.c](../../etc/afpd/fce_api.c.md)
* [etc/afpd/fce_util.c](../../etc/afpd/fce_util.c.md)
* [etc/afpd/file.c](../../etc/afpd/file.c.md)
* [etc/afpd/filedir.c](../../etc/afpd/filedir.c.md)
* [etc/afpd/fork.c](../../etc/afpd/fork.c.md)
* [etc/afpd/main.c](../../etc/afpd/main.c.md)
* [etc/afpd/messages.c](../../etc/afpd/messages.c.md)
* [etc/afpd/nfsquota.c](../../etc/afpd/nfsquota.c.md)
* [etc/afpd/quota.c](../../etc/afpd/quota.c.md)
* [etc/afpd/switch.c](../../etc/afpd/switch.c.md)
* [etc/afpd/uam.c](../../etc/afpd/uam.c.md)
* [etc/afpd/unix.c](../../etc/afpd/unix.c.md)
* [etc/afpd/virtual_icon.c](../../etc/afpd/virtual_icon.c.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [etc/netatalk/netatalk.c](../../etc/netatalk/netatalk.c.md)
* [etc/papd/auth.c](../../etc/papd/auth.c.md)
* [etc/papd/uam.c](../../etc/papd/uam.c.md)
* [etc/uams/uams_dhx2_pam.c](../../etc/uams/uams_dhx2_pam.c.md)
* [etc/uams/uams_dhx2_passwd.c](../../etc/uams/uams_dhx2_passwd.c.md)
* [etc/uams/uams_dhx_pam.c](../../etc/uams/uams_dhx_pam.c.md)
* [etc/uams/uams_dhx_passwd.c](../../etc/uams/uams_dhx_passwd.c.md)
* [etc/uams/uams_gss.c](../../etc/uams/uams_gss.c.md)
* [etc/uams/uams_guest.c](../../etc/uams/uams_guest.c.md)
* [etc/uams/uams_pam.c](../../etc/uams/uams_pam.c.md)
* [etc/uams/uams_passwd.c](../../etc/uams/uams_passwd.c.md)
* [etc/uams/uams_randnum.c](../../etc/uams/uams_randnum.c.md)
* [etc/uams/uams_srp.c](../../etc/uams/uams_srp.c.md)
* [include/atalk/asp.h](asp.h.md)
* [include/atalk/dsi.h](dsi.h.md)
* [include/atalk/globals.h](globals.h.md)
* [libatalk/acl/cache.c](../../libatalk/acl/cache.c.md)
* [libatalk/acl/ldap.c](../../libatalk/acl/ldap.c.md)
* [libatalk/acl/unix.c](../../libatalk/acl/unix.c.md)
* [libatalk/acl/uuid.c](../../libatalk/acl/uuid.c.md)
* [libatalk/asp/asp_attn.c](../../libatalk/asp/asp_attn.c.md)
* [libatalk/dsi/dsi_attn.c](../../libatalk/dsi/dsi_attn.c.md)
* [libatalk/util/afp_util.c](../../libatalk/util/afp_util.c.md)
* [libatalk/util/netatalk_conf.c](../../libatalk/util/netatalk_conf.c.md)
* [libatalk/util/unix.c](../../libatalk/util/unix.c.md)
* [libatalk/vfs/acl.c](../../libatalk/vfs/acl.c.md)
* [libatalk/vfs/ea_ad.c](../../libatalk/vfs/ea_ad.c.md)
* [libatalk/vfs/ea_sys.c](../../libatalk/vfs/ea_sys.c.md)
* [libatalk/vfs/unix.c](../../libatalk/vfs/unix.c.md)
* [libatalk/vfs/vfs.c](../../libatalk/vfs/vfs.c.md)

# Typedefs and enums

* `typedef uint16_t AFPUserBytes`
* `enum afpmessage_t`: `AFPMESG_LOGIN`, `AFPMESG_SERVER`

# Macros

* Undocumented: `AFPATTN_CRASH`, `AFPATTN_MESG`, `AFPATTN_NORECONNECT`, `AFPATTN_NOTIFY`, `AFPATTN_SHUTDOWN`, `AFPATTN_TIME`, `AFPATTN_VOLCHANGED`, `AFPERR_ACCESS`, `AFPERR_AUTHCONT`, `AFPERR_BADID`, `AFPERR_BADTYPE`, `AFPERR_BADUAM`, `AFPERR_BADVERS`, `AFPERR_BITMAP`, `AFPERR_BUSY`, `AFPERR_CANTMOVE`, `AFPERR_CATCHNG`, `AFPERR_CTNSHRD`, `AFPERR_DENYCONF`, `AFPERR_DFULL`, `AFPERR_DID1`, `AFPERR_DIFFVOL`, `AFPERR_DIRNEMPT`, `AFPERR_EOF`, `AFPERR_EXIST`, `AFPERR_EXISTID`, `AFPERR_FLATVOL`, `AFPERR_INSHRD`, `AFPERR_INTRASH`, `AFPERR_ITYPE`, `AFPERR_LOCK`, `AFPERR_MAXSESS`, `AFPERR_MISC`, `AFPERR_NFILE`, `AFPERR_NLOCK`, `AFPERR_NODIR`, `AFPERR_NOID`, `AFPERR_NOITEM`, `AFPERR_NOOBJ`, `AFPERR_NOOP`, `AFPERR_NORANGE`, `AFPERR_NORENAME`, `AFPERR_NOSRVR`, `AFPERR_NOTAUTH`, `AFPERR_OLOCK`, `AFPERR_PARAM`, `AFPERR_PWDCHNG`, `AFPERR_PWDEXPR`, `AFPERR_PWDPOLCY`, `AFPERR_PWDSAME`, `AFPERR_PWDSHORT`, `AFPERR_RANGEOVR`, `AFPERR_SAMEOBJ`, `AFPERR_SESSCLOS`, `AFPERR_SHUTDOWN`, `AFPERR_USRLOGIN`, `AFPERR_VLOCK`, `AFPPROTO_ASP`, `AFPPROTO_DSI`, `AFPSRVRINFO_COPY`, `AFPSRVRINFO_EXTSLEEP`, `AFPSRVRINFO_FASTBOZO`, `AFPSRVRINFO_NOSAVEPASSWD`, `AFPSRVRINFO_PASSWD`, `AFPSRVRINFO_SRVMSGS`, `AFPSRVRINFO_SRVNOTIFY`, `AFPSRVRINFO_SRVRDIR`, `AFPSRVRINFO_SRVRECONNECT`, `AFPSRVRINFO_SRVSIGNATURE`, `AFPSRVRINFO_SRVUTF8`, `AFPSRVRINFO_TCPIP`, `AFPSRVRINFO_UUID`, `AFPZZZ_EXT_SLEEP`, `AFPZZZ_EXT_WAKEUP`, `AFP_ACCESS`, `AFP_ADDAPPL`, `AFP_ADDCMT`, `AFP_ADDICON`, `AFP_BYTELOCK`, `AFP_BYTELOCK_EXT`, `AFP_CATSEARCH`, `AFP_CATSEARCH_EXT`, `AFP_CHANGEPW`, `AFP_CLOSEDIR`, `AFP_CLOSEDT`, `AFP_CLOSEFORK`, `AFP_CLOSEVOL`, `AFP_COPYFILE`, `AFP_CREATEDIR`, `AFP_CREATEFILE`, `AFP_CREATEID`, `AFP_DELETE`, `AFP_DELETEID`, `AFP_DISCTOLDSESS`, `AFP_ENUMERATE`, `AFP_ENUMERATE_EXT`, `AFP_ENUMERATE_EXT2`, `AFP_EXCHANGEFILE`, `AFP_FLUSH`, `AFP_FLUSHFORK`, `AFP_GETACL`, `AFP_GETAPPL`, `AFP_GETCMT`, `AFP_GETEXTATTR`, `AFP_GETFLDRPARAM`, `AFP_GETFORKPARAM`, `AFP_GETICON`, `AFP_GETSESSTOKEN`, `AFP_GETSRVINFO`, `AFP_GETSRVPARAM`, `AFP_GETSRVRMSG`, `AFP_GETUSERINFO`, `AFP_GETVOLPARAM`, `AFP_GTICNINFO`, `AFP_LISTEXTATTR`, `AFP_LOGIN`, `AFP_LOGINCONT`, `AFP_LOGIN_EXT`, `AFP_LOGOUT`, `AFP_MAPID`, `AFP_MAPNAME`, `AFP_MOVE`, `AFP_OK`, `AFP_OPENDIR`, `AFP_OPENDT`, `AFP_OPENFORK`, `AFP_OPENVOL`, `AFP_READ`, `AFP_READ_EXT`, `AFP_REMOVEATTR`, `AFP_RENAME`, `AFP_RESOLVEID`, `AFP_RMVAPPL`, `AFP_RMVCMT`, `AFP_SETACL`, `AFP_SETDIRPARAM`, `AFP_SETEXTATTR`, `AFP_SETFILEPARAM`, `AFP_SETFLDRPARAM`, `AFP_SETFORKPARAM`, `AFP_SETVOLPARAM`, `AFP_SPOTLIGHT_PRIVATE`, `AFP_SYNCDIR`, `AFP_SYNCFORK`, `AFP_WRITE`, `AFP_WRITE_EXT`, `AFP_ZZZZZ`, `REPLAYCACHE_SIZE`
