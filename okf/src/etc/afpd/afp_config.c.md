---
type: C Source File
title: "etc/afpd/afp_config.c"
description: "2 functions, includes 21 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/afp_config.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afp_config.h](afp_config.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/iniparser_util.h](../../include/atalk/iniparser_util.h.md)
* [atalk/ldapconfig.h](../../include/atalk/ldapconfig.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/server_child.h](../../include/atalk/server_child.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/zip.h](../../include/atalk/zip.h.md)
* [dircache.h](dircache.h.md)
* [status.h](status.h.md)
* [uam_auth.h](uam_auth.h.md)
* [volume.h](volume.h.md)
* System headers: `arpa/inet.h`, `ctype.h`, `errno.h`, `limits.h`, `netinet/in.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `unistd.h`

# Functions

### configfree

```c
void configfree(AFPObj *obj, DSI *dsi)
```

Defined at lines 65 to 116.

Release the configuration and the [DSI](../../include/atalk/dsi.h.md#struct-dsi) listeners.

An [ASP](../../include/atalk/asp.h.md#struct-asp) session only drops its [ASP](../../include/atalk/asp.h.md#struct-asp) resources. With dsi NULL the master afpd reloads its configuration, so the UAMs and every [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener go. A session child passes its session [DSI](../../include/atalk/dsi.h.md#struct-dsi), and that [DSI](../../include/atalk/dsi.h.md#struct-dsi) stays as the only one of obj. Both cases unload the volumes.

Parameters:
* `obj`: server or session object
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) of a session child, or NULL in the master

Calls: [atp_close](../../libatalk/atp/atp_close.c.md#atp_close), [auth_unload](auth.c.md#auth_unload), [dsi_free](../../libatalk/dsi/dsi_tcp.c.md#dsi_free), [unload_volumes](../../libatalk/util/netatalk_conf.c.md#unload_volumes)

Called by: [dsi_start](main.c.md#dsi_start), [main](main.c.md#main)

### configinit

```c
int configinit(AFPObj *dsi_obj, AFPObj *asp_obj)
```

Defined at lines 132 to 433.

Set up the UAMs, the listeners, and the server-wide settings at startup or reload.

When the options carry OPTION_DDP, the [ASP](../../include/atalk/asp.h.md#struct-asp) server registers its NBP name first. [DSI](../../include/atalk/dsi.h.md#struct-dsi) listeners come from the afp listen addresses, from the afp interfaces, or from the host name when neither yields one. The FCE settings and the other global options of afp.conf follow.

Parameters:
* `dsi_obj`: server object for AFP over TCP
* `asp_obj`: server object for AFP over AppleTalk

Returns: 0, or -1 when no listener could start or a setting failed

Calls: [acl_ldap_freeconfig](../../libatalk/acl/ldap_config.c.md#acl_ldap_freeconfig), [acl_ldap_readconfig](../../libatalk/acl/ldap_config.c.md#acl_ldap_readconfig), [asp_close](../../libatalk/asp/asp_close.c.md#asp_close), [asp_init](../../libatalk/asp/asp_init.c.md#asp_init), [atp_close](../../libatalk/atp/atp_close.c.md#atp_close), [atp_open](../../libatalk/atp/atp_open.c.md#atp_open), [auth_load](auth.c.md#auth_load), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [dsi_init](../../libatalk/dsi/dsi_init.c.md#dsi_init), [fce_add_udp_socket](fce_api.c.md#fce_add_udp_socket), [fce_set_coalesce](fce_util.c.md#fce_set_coalesce), [fce_set_events](fce_api.c.md#fce_set_events), [getip_port](../../libatalk/util/socket.c.md#getip_port), [getip_string](../../libatalk/util/socket.c.md#getip_string), [nbp_rgstr](../../libatalk/nbp/nbp_rgstr.c.md#nbp_rgstr), [nbp_unrgstr](../../libatalk/nbp/nbp_unrgstr.c.md#nbp_unrgstr), [safe_atoi](../../libatalk/util/netatalk_conf.c.md#safe_atoi), [set_signature](status.c.md#set_signature), [status_init](status.c.md#status_init), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [main](main.c.md#main)
