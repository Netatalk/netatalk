---
type: C Source File
title: "bin/getzones/getzones.c"
description: "8 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/getzones/getzones.c"
tags: ["bin/getzones"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/getzones](../getzones.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/netddp.h](../../include/atalk/netddp.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/zip.h](../../include/atalk/zip.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `atalk/ddp.h`, `netdb.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`, `unistd.h`

# Functions

### usage

```c
static void usage(char *s)
```

Defined at lines 36 to 42.

Called by: [main](getzones.c.md#main)

### main

```c
int main(int argc, char *argv[])
```

Defined at lines 44 to 197.

Calls: [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [do_atp_lookup](getzones.c.md#do_atp_lookup), [do_getnetinfo](getzones.c.md#do_getnetinfo), [do_query](getzones.c.md#do_query), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name), [usage](getzones.c.md#usage)

### do_atp_lookup

```c
void do_atp_lookup(struct sockaddr_at *saddr, uint8_t lookup_type, charset_t charset)
```

Defined at lines 199 to 282.

Calls: [atp_close](../../libatalk/atp/atp_close.c.md#atp_close), [atp_open](../../libatalk/atp/atp_open.c.md#atp_open), [atp_rresp](../../libatalk/atp/atp_rresp.c.md#atp_rresp), [atp_sreq](../../libatalk/atp/atp_sreq.c.md#atp_sreq), [print_zones](getzones.c.md#print_zones)

Called by: [main](getzones.c.md#main)

### print_zones

```c
static void print_zones(short n, const char *buf, charset_t charset)
```

Defined at lines 291 to 316.

Print zones from a getzone reply.

Parameters:
* `n`: number of zones in this packet
* `buf`: zone length/name pairs
* `charset`: charset to convert zone names from

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate)

Called by: [do_atp_lookup](getzones.c.md#do_atp_lookup)

### do_getnetinfo

```c
void do_getnetinfo(struct sockaddr_at *dest, const char *zone_to_confirm, charset_t charset)
```

Defined at lines 318 to 383.

Calls: [netddp_open](../../libatalk/netddp/netddp_open.c.md#netddp_open), [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto), [print_gnireply](getzones.c.md#print_gnireply), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [main](getzones.c.md#main)

### print_gnireply

```c
static void print_gnireply(ssize_t len, uint8_t *buf, charset_t charset)
```

Defined at lines 385 to 498.

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate)

Called by: [do_getnetinfo](getzones.c.md#do_getnetinfo)

### do_query

```c
void do_query(struct sockaddr_at *dest, uint16_t network, charset_t charset)
```

Defined at lines 500 to 579.

Calls: [netddp_open](../../libatalk/netddp/netddp_open.c.md#netddp_open), [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto), [print_and_count_zones_in_reply](getzones.c.md#print_and_count_zones_in_reply)

Called by: [main](getzones.c.md#main)

### print_and_count_zones_in_reply

```c
static int print_and_count_zones_in_reply(uint8_t *buf, size_t len, charset_t charset)
```

Defined at lines 581 to 631.

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate)

Called by: [do_query](getzones.c.md#do_query)

# Macros

* Undocumented: `MACCHARSET`, `ZIPOP_DEFAULT`
