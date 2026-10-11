---
type: C Source File
title: "libatalk/atp/atp_rsel.c"
description: "3 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_rsel.c"
tags: ["libatalk/atp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/atp](../atp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/netddp.h](../../include/atalk/netddp.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atp_internals.h](atp_internals.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `signal.h`, `string.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### resend_request

```c
static int resend_request(ATP ah)
```

Defined at lines 47 to 81.

Send the pending ATP request again, with a bitmap of the responses still missing.

The call stamps the send time and uses up one try unless the tries are infinite.

Parameters:
* `ah`: ATP endpoint with a pending request

Returns: 0, or -1 for a failed send

Calls: [bprint](../util/bprint.c.md#bprint), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Called by: [atp_rsel](atp_rsel.c.md#atp_rsel)

### atp_resp_incomplete

```c
static int atp_resp_incomplete(ATP ah)
```

Defined at lines 94 to 102.

Report a response that leaves the transaction incomplete.

The callers loop while this returns 0, so the call returns 0 only while retries remain. After the last retry atp_rsel waits without a timeout, and a further wait could block with nothing to end it. The call fails with ETIMEDOUT instead.

Parameters:
* `ah`: ATP endpoint

Returns: 0 while retries remain, or -1 with ETIMEDOUT

Called by: [atp_rsel](atp_rsel.c.md#atp_rsel)

### atp_rsel

```c
int atp_rsel(ATP, struct sockaddr_at *, int)
```

Defined at lines 121 to 431. Declared in [include/atalk/atp.h](../../include/atalk/atp.h.md).

Read ATP packets until a new request arrives or the pending response is complete.

While a request is pending, the call resends it after each timeout until the tries run out. A new request joins the queue for atp_rreq, and a duplicate of an answered exactly once request gets the saved response again. A response packet of the pending transaction fills its slot. ATP_EOM ends the response early, and ATP_STS sends the request again with the remaining bitmap. A complete exactly once response gets a TREL.

Parameters:
* `ah`: ATP endpoint
* `faddr`: address to accept, receives the sender
* `func`: ATP_TREQ, ATP_TRESP, or 0 for both

Returns: ATP_TREQ, ATP_TRESP, 0 for a packet that completes nothing, or -1 for an error, ETIMEDOUT after the last try

Calls: [at_addr_eq](atp_packet.c.md#at_addr_eq), [atp_alloc_buf](atp_bufs.c.md#atp_alloc_buf), [atp_free_buf](atp_bufs.c.md#atp_free_buf), [atp_print_bufuse](atp_internals.h.md#atp_print_bufuse), [atp_queue_push](atp_packet.c.md#atp_queue_push), [atp_recv_atp](atp_packet.c.md#atp_recv_atp), [atp_resp_incomplete](atp_rsel.c.md#atp_resp_incomplete), [bprint](../util/bprint.c.md#bprint), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto), [resend_request](atp_rsel.c.md#resend_request)

Called by: [atp_rreq_try](atp_rreq.c.md#atp_rreq_try), [atp_rresp](atp_rresp.c.md#atp_rresp), [main](../../etc/papd/main.c.md#main), [send_file](../../bin/pap/pap.c.md#send_file), [session](../../etc/papd/session.c.md#session)
