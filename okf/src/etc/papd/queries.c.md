---
type: C Source File
title: "etc/papd/queries.c"
description: "18 functions, 1 type, includes 9 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/queries.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [comment.h](comment.h.md)
* [file.h](file.h.md)
* [lp.h](lp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [ppd.h](ppd.h.md)
* [printer.h](printer.h.md)
* [uam_auth.h](uam_auth.h.md)
* System headers: `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/time.h`, `sys/types.h`

# Function tables

### genqueries

Initialized at line 216. Dispatches to: [gq_pagecost](queries.c.md#gq_pagecost), [gq_product](queries.c.md#gq_product), [gq_rbispoolerid](queries.c.md#gq_rbispoolerid), [gq_rbiuamlist](queries.c.md#gq_rbiuamlist), [gq_true](queries.c.md#gq_true)

### queries

Initialized at line 35. Dispatches to: [cq_default](queries.c.md#cq_default), [cq_end](queries.c.md#cq_end), [cq_feature](queries.c.md#cq_feature), [cq_font](queries.c.md#cq_font), [cq_fontlist](queries.c.md#cq_fontlist), [cq_printer](queries.c.md#cq_printer), [cq_query](queries.c.md#cq_query), [cq_rbilogin](queries.c.md#cq_rbilogin)

# Functions

### cq_k4login

```c
int cq_k4login(struct papfile *, struct papfile *, struct sockaddr_at *)
```

Declared at etc/papd/queries.c line 28; no definition in the scanned sources.

### cq_uameth

```c
int cq_uameth(struct papfile *, struct papfile *, struct sockaddr_at *)
```

Declared at etc/papd/queries.c line 29; no definition in the scanned sources.

### gq_balance

```c
int gq_balance(struct papfile *)
```

Declared at etc/papd/queries.c line 31; no definition in the scanned sources.

### cq_rmjob

```c
int cq_rmjob(struct papfile *, struct papfile *, struct sockaddr_at *)
```

Declared at etc/papd/queries.c line 44; no definition in the scanned sources.

### cq_default

```c
int cq_default(struct papfile *, struct papfile *, struct sockaddr_at *)
```

Defined at lines 52 to 107.

Calls: [append](file.c.md#append), [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [markline](file.c.md#markline)

Called by: [cq_feature](queries.c.md#cq_feature), [cq_printer](queries.c.md#cq_printer), [cq_query](queries.c.md#cq_query)

Dispatched via: [queries](queries.c.md#queries)

### gq_true

```c
int gq_true(struct papfile *)
```

Defined at lines 109 to 117.

Calls: [append](file.c.md#append)

Dispatched via: [genqueries](queries.c.md#genqueries)

### gq_pagecost

```c
int gq_pagecost(struct papfile *)
```

Defined at lines 119 to 140.

Calls: [append](file.c.md#append), [lp_pagecost](lp.h.md#lp_pagecost)

Dispatched via: [genqueries](queries.c.md#genqueries)

### gq_rbispoolerid

```c
int gq_rbispoolerid(struct papfile *)
```

Defined at lines 162 to 166.

Handler for RBISpoolerID

Calls: [append](file.c.md#append)

Uses file-scope variables: `spoolerid`

Dispatched via: [genqueries](queries.c.md#genqueries)

### gq_rbiuamlist

```c
int gq_rbiuamlist(struct papfile *)
```

Defined at lines 173 to 189.

Handler for RBIUAMListQuery

Calls: [append](file.c.md#append), [getuamnames](auth.c.md#getuamnames)

Uses file-scope variables: `nouams`

Dispatched via: [genqueries](queries.c.md#genqueries)

### gq_product

```c
int gq_product(struct papfile *)
```

Defined at lines 191 to 198.

Calls: [append](file.c.md#append)

Dispatched via: [genqueries](queries.c.md#genqueries)

### cq_query

```c
int cq_query(struct papfile *, struct papfile *, struct sockaddr_at *)
```

Defined at lines 218 to 287.

Calls: [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [comswitch](comment.c.md#comswitch), [cq_default](queries.c.md#cq_default), [markline](file.c.md#markline)

Calls through [`genquery::gq_handler`](queries.c.md#struct-genquery): no table assigns this field

Uses file-scope variables: `genqueries`, `queries`

Dispatched via: [queries](queries.c.md#queries)

### cq_font_answer

```c
void cq_font_answer(char *, char *, struct papfile *)
```

Defined at lines 289 to 330.

Calls: [append](file.c.md#append)

Called by: [cq_font](queries.c.md#cq_font)

### cq_fontlist

```c
int cq_fontlist(struct papfile *, struct papfile *, struct sockaddr_at *)
```

Defined at lines 332 to 401.

Calls: [append](file.c.md#append), [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [markline](file.c.md#markline)

Dispatched via: [queries](queries.c.md#queries)

### cq_font

```c
int cq_font(struct papfile *, struct papfile *, struct sockaddr_at *)
```

Defined at lines 404 to 468.

Calls: [append](file.c.md#append), [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [cq_font_answer](queries.c.md#cq_font_answer), [markline](file.c.md#markline)

Uses file-scope variables: `comcont` in [etc/papd/comment.c](comment.c.md)

Dispatched via: [queries](queries.c.md#queries)

### cq_feature

```c
int cq_feature(struct papfile *, struct papfile *, struct sockaddr_at *)
```

Defined at lines 470 to 537.

Calls: [append](file.c.md#append), [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [comswitch](comment.c.md#comswitch), [cq_default](queries.c.md#cq_default), [markline](file.c.md#markline)

Uses file-scope variables: `queries`

Dispatched via: [queries](queries.c.md#queries)

### cq_printer

```c
int cq_printer(struct papfile *, struct papfile *, struct sockaddr_at *)
```

Defined at lines 542 to 619.

Calls: [append](file.c.md#append), [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [comswitch](comment.c.md#comswitch), [cq_default](queries.c.md#cq_default), [markline](file.c.md#markline)

Uses file-scope variables: `prod`, `psver`, `queries`

Dispatched via: [queries](queries.c.md#queries)

### cq_rbilogin

```c
int cq_rbilogin(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 710 to 774.

Handler for RBILogin

Calls: [append](file.c.md#append), [auth_uamfind](auth.c.md#auth_uamfind), [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [lp_person](lp.c.md#lp_person), [markline](file.c.md#markline)

Calls through [`uam_obj::uam_printer`](../afpd/uam_auth.h.md#struct-uam_obj): no table assigns this field

Uses file-scope variables: `papd_uam`, `rbiloginbad`, `rbiloginerrstr`, `rbiloginok`

Dispatched via: [queries](queries.c.md#queries)

### cq_end

```c
int cq_end(struct papfile *, struct papfile *, struct sockaddr_at *)
```

Defined at lines 776 to 797.

Calls: [compop](comment.c.md#compop), [markline](file.c.md#markline)

Dispatched via: [queries](queries.c.md#queries)

# Types

### struct genquery

Defined at line 201.
* `char * gq_name`
* `int(* gq_handler`: Called through by [cq_query](queries.c.md#cq_query).

# File-scope variables

`nouams`, `papd_uam`, `prod`, `psver`, `rbiloginbad`, `rbiloginerrstr`, `rbiloginok`, `spoolerid`
