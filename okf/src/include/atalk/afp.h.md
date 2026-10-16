---
type: C Header File
title: "include/atalk/afp.h"
description: "Apple Filing Protocol command, capability and error constants."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/afp.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
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

* `AFPATTN_CRASH`: server crashed
* `AFPATTN_MESG`: server has message
* `AFPATTN_NORECONNECT`: don't reconnect
* `AFPATTN_SHUTDOWN`: shutdown/disconnect
* `AFPATTN_TIME`: time in minutes
* `AFPATTN_VOLCHANGED`: volume has changed
* `AFPERR_ACCESS`: permission denied
* `AFPERR_AUTHCONT`: logincont
* `AFPERR_BADID`: non-existent file id
* `AFPERR_BADTYPE`: object is the wrong type
* `AFPERR_BADUAM`: uam doesn't exist
* `AFPERR_BADVERS`: bad afp version number
* `AFPERR_BITMAP`: invalid bitmap
* `AFPERR_BUSY`: FileBusy.
* `AFPERR_CANTMOVE`: can't move file
* `AFPERR_CATCHNG`: catalog has changed
* `AFPERR_CTNSHRD`: share point contains a share point
* `AFPERR_DENYCONF`: file synchronization locks conflict
* `AFPERR_DFULL`: disk full
* `AFPERR_DID1`: not an afp error DID is 1
* `AFPERR_DIFFVOL`: different volume
* `AFPERR_DIRNEMPT`: directory not empty
* `AFPERR_EOF`: end of file catsearch and afp_read
* `AFPERR_EXIST`: object already exists
* `AFPERR_EXISTID`: file already has an id
* `AFPERR_FLATVOL`: volume doesn't support directories
* `AFPERR_INSHRD`: folder being shared is inside a shared folder.
* `AFPERR_INTRASH`: shared folder in trash.
* `AFPERR_ITYPE`: wrong icon type
* `AFPERR_LOCK`: LockErr.
* `AFPERR_MAXSESS`: maximum number of allowed sessions reached
* `AFPERR_MISC`: misc.
* `AFPERR_NFILE`: too many files open
* `AFPERR_NLOCK`: no more locks
* `AFPERR_NODIR`: couldn't find directory
* `AFPERR_NOID`: file thread not found
* `AFPERR_NOITEM`: ItemNotFound.
* `AFPERR_NOOBJ`: object not found
* `AFPERR_NOOP`: command not supported
* `AFPERR_NORANGE`: no range lock
* `AFPERR_NORENAME`: can't rename
* `AFPERR_NOSRVR`: no response by server at that address
* `AFPERR_NOTAUTH`: user not authenticated
* `AFPERR_OLOCK`: object locked
* `AFPERR_PARAM`: parameter error
* `AFPERR_PWDCHNG`: password needs to be changed
* `AFPERR_PWDEXPR`: password expired
* `AFPERR_PWDPOLCY`: password fails policy check
* `AFPERR_PWDSAME`: same password/can't change password
* `AFPERR_PWDSHORT`: password too short
* `AFPERR_RANGEOVR`: range overlap
* `AFPERR_SAMEOBJ`: source file == destination file
* `AFPERR_SESSCLOS`: session closed
* `AFPERR_SHUTDOWN`: server is going down
* `AFPERR_USRLOGIN`: user already logged on
* `AFPERR_VLOCK`: volume locked
* `AFPSRVRINFO_COPY`: supports copyfile
* `AFPSRVRINFO_EXTSLEEP`: supports extended sleep
* `AFPSRVRINFO_FASTBOZO`: fast copying
* `AFPSRVRINFO_NOSAVEPASSWD`: don't allow save password
* `AFPSRVRINFO_PASSWD`: supports change password
* `AFPSRVRINFO_SRVMSGS`: supports server messages
* `AFPSRVRINFO_SRVNOTIFY`: supports server notifications
* `AFPSRVRINFO_SRVRDIR`: supports directories service
* `AFPSRVRINFO_SRVRECONNECT`: supports server reconnect
* `AFPSRVRINFO_SRVSIGNATURE`: supports server signature
* `AFPSRVRINFO_SRVUTF8`: supports UTF8 names AFP 3.1
* `AFPSRVRINFO_TCPIP`: supports tcpip
* `AFPSRVRINFO_UUID`: supports UUIDs
* Undocumented: `AFPATTN_NOTIFY`, `AFPPROTO_ASP`, `AFPPROTO_DSI`, `AFPZZZ_EXT_SLEEP`, `AFPZZZ_EXT_WAKEUP`, `AFP_ACCESS`, `AFP_ADDAPPL`, `AFP_ADDCMT`, `AFP_ADDICON`, `AFP_BYTELOCK`, `AFP_BYTELOCK_EXT`, `AFP_CATSEARCH`, `AFP_CATSEARCH_EXT`, `AFP_CHANGEPW`, `AFP_CLOSEDIR`, `AFP_CLOSEDT`, `AFP_CLOSEFORK`, `AFP_CLOSEVOL`, `AFP_COPYFILE`, `AFP_CREATEDIR`, `AFP_CREATEFILE`, `AFP_CREATEID`, `AFP_DELETE`, `AFP_DELETEID`, `AFP_DISCTOLDSESS`, `AFP_ENUMERATE`, `AFP_ENUMERATE_EXT`, `AFP_ENUMERATE_EXT2`, `AFP_EXCHANGEFILE`, `AFP_FLUSH`, `AFP_FLUSHFORK`, `AFP_GETACL`, `AFP_GETAPPL`, `AFP_GETCMT`, `AFP_GETEXTATTR`, `AFP_GETFLDRPARAM`, `AFP_GETFORKPARAM`, `AFP_GETICON`, `AFP_GETSESSTOKEN`, `AFP_GETSRVINFO`, `AFP_GETSRVPARAM`, `AFP_GETSRVRMSG`, `AFP_GETUSERINFO`, `AFP_GETVOLPARAM`, `AFP_GTICNINFO`, `AFP_LISTEXTATTR`, `AFP_LOGIN`, `AFP_LOGINCONT`, `AFP_LOGIN_EXT`, `AFP_LOGOUT`, `AFP_MAPID`, `AFP_MAPNAME`, `AFP_MOVE`, `AFP_OK`, `AFP_OPENDIR`, `AFP_OPENDT`, `AFP_OPENFORK`, `AFP_OPENVOL`, `AFP_READ`, `AFP_READ_EXT`, `AFP_REMOVEATTR`, `AFP_RENAME`, `AFP_RESOLVEID`, `AFP_RMVAPPL`, `AFP_RMVCMT`, `AFP_SETACL`, `AFP_SETDIRPARAM`, `AFP_SETEXTATTR`, `AFP_SETFILEPARAM`, `AFP_SETFLDRPARAM`, `AFP_SETFORKPARAM`, `AFP_SETVOLPARAM`, `AFP_SPOTLIGHT_PRIVATE`, `AFP_SYNCDIR`, `AFP_SYNCFORK`, `AFP_WRITE`, `AFP_WRITE_EXT`, `AFP_ZZZZZ`, `REPLAYCACHE_SIZE`
