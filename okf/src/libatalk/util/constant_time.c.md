---
type: C Source File
title: "libatalk/util/constant_time.c"
description: "1 function, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/constant_time.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/constant_time.h](../../include/atalk/constant_time.h.md)

# Functions

### atalk_ct_memcmp

```c
int atalk_ct_memcmp(const void *a, const void *b, size_t n)
```

Defined at lines 27 to 38.

Constant-time memory equality check.

Compares exactly `n` bytes and returns 0 if the buffers are equal, non-zero if they differ.

Note: Unlike memcmp(), this function does not provide lexicographic ordering and must not be used for sorting.

Returns: 0 if the buffers are equal, non-zero if they differ.

Called by: [changepw_3](../../etc/uams/uams_dhx2_pam.c.md#changepw_3), [pam_changepw](../../etc/uams/uams_pam.c.md#pam_changepw), [rand2num_logincont](../../etc/uams/uams_randnum.c.md#rand2num_logincont), [randnum_changepw](../../etc/uams/uams_randnum.c.md#randnum_changepw), [randnum_logincont](../../etc/uams/uams_randnum.c.md#randnum_logincont), [server_child_transfer_session](server_child.c.md#server_child_transfer_session), [srp_logincont](../../etc/uams/uams_srp.c.md#srp_logincont), [update_srp_passwd](../../bin/afppasswd/afppasswd.c.md#update_srp_passwd)
