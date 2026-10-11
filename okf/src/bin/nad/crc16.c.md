---
type: C Source File
title: "bin/nad/crc16.c"
description: "1 function, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/crc16.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [crc16.h](crc16.h.md)

# Functions

### crc16_xmodem_update

```c
uint16_t crc16_xmodem_update(uint16_t crc, const void *buf, size_t len)
```

Defined at lines 30 to 47.

Add bytes to a running CRC-16/XMODEM value.

The CRC uses the polynomial 0x1021 with the high bit first and no final XOR. BinHex and MacBinary start it at 0, and a run over more than one buffer passes each result to the next call.

Parameters:
* `crc`: CRC so far, 0 at the start
* `buf`: bytes to add
* `len`: number of bytes

Returns: the CRC after the bytes

Called by: [bin_header_write](macbin.c.md#bin_header_write), [hqx_header_read](hqx.c.md#hqx_header_read), [hqx_header_write](hqx.c.md#hqx_header_write), [hqx_read](hqx.c.md#hqx_read), [hqx_write](hqx.c.md#hqx_write), [test_header](macbin.c.md#test_header)

# Macros

* Undocumented: `CRC16_XMODEM_POLY`
