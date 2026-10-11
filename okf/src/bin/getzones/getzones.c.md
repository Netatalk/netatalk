---
type: C Source File
title: "bin/getzones/getzones.c"
description: "8 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/getzones/getzones.c"
tags: ["bin/getzones"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 41 to 47.

Print the usage of getzones to stderr and exit with 1.

Parameters:
* `s`: program name

Called by: [main](getzones.c.md#main)

### main

```c
int main(int argc, char *argv[])
```

Defined at lines 67 to 220.

Ask an AppleTalk router for zone information through ZIP and print it.

Without an option the tool prints every zone of the AppleTalk network. The -l option prints the zones of the local network, and -m prints the zone of the asking node. The -g option prints the GetNetInfo reply of the current network, and -z asks GetNetInfo whether one zone is valid. The -q option prints the zones of one network. These five options exclude each other.

The -c option sets the Mac charset of the zone names, MAC_ROMAN by default. The optional address names the router to ask, and without it the tool asks the local atalkd.

Parameters:
* `argc`: argument count
* `argv`: argument vector

Returns: 0, while a failure exits with 1 and a GetNetInfo reply that marks the requested zone invalid exits with 2

Calls: [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [do_atp_lookup](getzones.c.md#do_atp_lookup), [do_getnetinfo](getzones.c.md#do_getnetinfo), [do_query](getzones.c.md#do_query), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name), [usage](getzones.c.md#usage)

### do_atp_lookup

```c
void do_atp_lookup(struct sockaddr_at *saddr, uint8_t lookup_type, charset_t charset)
```

Defined at lines 235 to 318.

Run a ZIP request over ATP, print the zones of each reply, and exit.

GetZoneList and GetLocalZones ask page by page, from index 1 on, until a reply carries the last flag. GetMyZone asks once with index 0. Each request has a timeout of 2 seconds and 5 tries. The call exits with 0 after the last page, or with 1 for an ATP error.

Parameters:
* `saddr`: router to ask
* `lookup_type`: ZIPOP_GETZONELIST, ZIPOP_GETLOCALZONES, or ZIPOP_GETMYZONE
* `charset`: Mac charset of the zone names

Calls: [atp_close](../../libatalk/atp/atp_close.c.md#atp_close), [atp_open](../../libatalk/atp/atp_open.c.md#atp_open), [atp_rresp](../../libatalk/atp/atp_rresp.c.md#atp_rresp), [atp_sreq](../../libatalk/atp/atp_sreq.c.md#atp_sreq), [print_zones](getzones.c.md#print_zones)

Called by: [main](getzones.c.md#main)

### print_zones

```c
static void print_zones(short n, const char *buf, charset_t charset)
```

Defined at lines 327 to 352.

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

Defined at lines 366 to 431.

Send a ZIP GetNetInfo request, wait for its reply, and print it.

The zone name goes into the request as given, without a charset conversion. The call waits without a timeout and skips datagrams that are not GetNetInfo replies. A socket or send error exits with 1.

Parameters:
* `dest`: router to ask
* `zone_to_confirm`: zone to ask about, cut at 32 bytes, or NULL for none
* `charset`: Mac charset of the zone names in the reply

Calls: [netddp_open](../../libatalk/netddp/netddp_open.c.md#netddp_open), [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto), [print_gnireply](getzones.c.md#print_gnireply), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [main](getzones.c.md#main)

### print_gnireply

```c
static void print_gnireply(ssize_t len, uint8_t *buf, charset_t charset)
```

Defined at lines 445 to 558.

Print the fields of a ZIP GetNetInfo reply.

The output lists the network range, the flags, the requested zone, the multicast address, and the default zone when the reply has one. A short or malformed reply exits with 1, and a reply that marks the requested zone invalid exits with 2.

Parameters:
* `len`: reply length
* `buf`: reply, from the DDP type byte on
* `charset`: Mac charset of the zone names

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate)

Called by: [do_getnetinfo](getzones.c.md#do_getnetinfo)

### do_query

```c
void do_query(struct sockaddr_at *dest, uint16_t network, charset_t charset)
```

Defined at lines 572 to 651.

Send a ZIP query for one network and print the zones of the replies.

The call waits without a timeout and skips datagrams that are not ZIP replies. A ZIPOP_REPLY carries every zone. Extended replies can span datagrams, so the call reads on until the printed zones reach the count that the first one names. A socket or send error exits with 1.

Parameters:
* `dest`: router to ask
* `network`: network number
* `charset`: Mac charset of the zone names

Calls: [netddp_open](../../libatalk/netddp/netddp_open.c.md#netddp_open), [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto), [print_and_count_zones_in_reply](getzones.c.md#print_and_count_zones_in_reply)

Called by: [main](getzones.c.md#main)

### print_and_count_zones_in_reply

```c
static int print_and_count_zones_in_reply(uint8_t *buf, size_t len, charset_t charset)
```

Defined at lines 665 to 715.

Print the network number and the zone name of each tuple in a ZIP reply.

The call stops at the first tuple that does not fit the buffer. A zone name that does not convert exits with 1.

Parameters:
* `buf`: tuples after the reply header
* `len`: length of the tuples
* `charset`: Mac charset of the zone names

Returns: the number of tuples printed

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate)

Called by: [do_query](getzones.c.md#do_query)

# Macros

* Undocumented: `MACCHARSET`, `ZIPOP_DEFAULT`
