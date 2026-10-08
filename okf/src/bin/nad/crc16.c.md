---
type: C Source File
title: "bin/nad/crc16.c"
description: "1 function, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/crc16.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [crc16.h](crc16.h.md)

# Functions

### crc16_xmodem_update

```c
uint16_t crc16_xmodem_update(uint16_t crc, const void *buf, size_t len)
```

Defined at lines 18 to 35.

Called by: [bin_header_write](macbin.c.md#bin_header_write), [hqx_header_read](hqx.c.md#hqx_header_read), [hqx_header_write](hqx.c.md#hqx_header_write), [hqx_read](hqx.c.md#hqx_read), [hqx_write](hqx.c.md#hqx_write), [test_header](macbin.c.md#test_header)

# Macros

* Undocumented: `CRC16_XMODEM_POLY`
