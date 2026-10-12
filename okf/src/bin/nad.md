---
type: Subsystem
title: "bin/nad"
description: "24 files, 181 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/bin/nad"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

# Files

* [bin/nad/crc16.c](nad/crc16.c.md): CRC-16/XMODEM for BinHex and MacBinary archives.
* [bin/nad/crc16.h](nad/crc16.h.md): No functions or types.
* [bin/nad/ftw.c](nad/ftw.c.md): File tree walker from the GNU C Library for the nad commands.
* [bin/nad/ftw.h](nad/ftw.h.md): 1 type.
* [bin/nad/hqx.c](nad/hqx.c.md): BinHex 4.0 encoding and decoding for the nad archive commands.
* [bin/nad/hqx.h](nad/hqx.h.md): No functions or types.
* [bin/nad/macbin.c](nad/macbin.c.md): MacBinary encoding and decoding for the nad archive commands.
* [bin/nad/macbin.h](nad/macbin.h.md): No functions or types.
* [bin/nad/megatron.c](nad/megatron.c.md): Conversion driver of the nad archive commands bin, hex, unbin, and unhex.
* [bin/nad/megatron.h](nad/megatron.h.md): 4 types, includes 2 project headers.
* [bin/nad/nad.c](nad/nad.c.md): The nad command: entry point and subcommand dispatch.
* [bin/nad/nad.h](nad/nad.h.md): 2 types, includes 4 project headers.
* [bin/nad/nad_adouble.c](nad/nad_adouble.c.md): AppleDouble metadata and CAP name encoding for the nad archive and StuffIt commands.
* [bin/nad/nad_adouble.h](nad/nad_adouble.h.md): No functions or types.
* [bin/nad/nad_cp.c](nad/nad_cp.c.md): AFP-aware copying of files and directory trees for nad.
* [bin/nad/nad_find.c](nad/nad_find.c.md): The nad find command: search a volume for entries by name.
* [bin/nad/nad_ls.c](nad/nad_ls.c.md): The nad ls command: list entries with their Mac metadata.
* [bin/nad/nad_mkdir.c](nad/nad_mkdir.c.md): The nad mkdir command: create directories with CNIDs and metadata.
* [bin/nad/nad_mv.c](nad/nad_mv.c.md): The nad mv command: move entries with their CNIDs and metadata.
* [bin/nad/nad_rm.c](nad/nad_rm.c.md): The nad rm command: delete entries with their CNIDs and metadata.
* [bin/nad/nad_rmdir.c](nad/nad_rmdir.c.md): The nad rmdir command: delete empty directories with their CNIDs and metadata.
* [bin/nad/nad_set.c](nad/nad_set.c.md): The nad set command: change the Mac metadata of an entry.
* [bin/nad/nad_stuffit.c](nad/nad_stuffit.c.md): The nad sit and unsit commands: create and extract StuffIt archives.
* [bin/nad/nad_util.c](nad/nad_util.c.md): Volume, signal, and CNID helpers shared by the nad commands.

# Includes headers from

* [include/atalk](../include/atalk.md): 76 includes
* [sys/netatalk](../sys/netatalk.md): 4 includes

# Calls into

* [libatalk/adouble](../libatalk/adouble.md): 71 calls
* [libatalk/compat](../libatalk/compat.md): 30 calls
* [libatalk/cnid](../libatalk/cnid.md): 24 calls
* [libatalk/util](../libatalk/util.md): 18 calls
* [include/atalk](../include/atalk.md): 3 calls
* [libatalk/unicode](../libatalk/unicode.md): 2 calls
* [etc/afpd](../etc/afpd.md): 1 calls

# Most called functions

* [closevol](nad/nad_util.c.md#closevol): 10 callers
* [openvol_optional](nad/nad_util.c.md#openvol_optional): 10 callers
* [nad_report_cnid_reset](nad/nad_util.c.md#nad_report_cnid_reset): 8 callers
* [set_signal](nad/nad_util.c.md#set_signal): 7 callers
* [crc16_xmodem_update](nad/crc16.c.md#crc16_xmodem_update): 6 callers
* [nad_close](nad/nad_adouble.c.md#nad_close): 4 callers
* [nad_open](nad/nad_adouble.c.md#nad_open): 4 callers
* [bin_close](nad/macbin.c.md#bin_close): 3 callers
* [hqx_close](nad/hqx.c.md#hqx_close): 3 callers
* [hqx_emit_data](nad/hqx.c.md#hqx_emit_data): 3 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[mtoupathcap](nad/nad_adouble.c.md#mtoupathcap), [openvol](nad/nad_util.c.md#openvol), [parse_archive_mode](nad/megatron.c.md#parse_archive_mode), [upfunc](nad/nad_cp.c.md#upfunc), [upfunc](nad/nad_rm.c.md#upfunc), [utompathcap](nad/nad_adouble.c.md#utompathcap), [xgetcwd](nad/ftw.c.md#xgetcwd)
