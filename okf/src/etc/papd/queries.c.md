---
type: C Source File
title: "etc/papd/queries.c"
description: "18 functions, 1 type, includes 9 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/queries.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Initialized at line 259. Dispatches to: [gq_pagecost](queries.c.md#gq_pagecost), [gq_product](queries.c.md#gq_product), [gq_rbispoolerid](queries.c.md#gq_rbispoolerid), [gq_rbiuamlist](queries.c.md#gq_rbiuamlist), [gq_true](queries.c.md#gq_true)

### queries

All queries start with %%?Begin and end with %%?End.

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
int cq_default(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 65 to 120.

Answer an unknown query with the default from its end comment.

The call drops the lines of the query up to its end comment, such as `%%?EndFeatureQuery: Unknown`, and replies with the text after the colon. A query without an end keyword only pops.

Parameters:
* `in`: data from the client
* `out`: reply data
* `sat`: client address, unused

Returns: CH_DONE, 0 at the end of the input, or CH_MORE while a line is incomplete

Calls: [append](file.c.md#append), [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [markline](file.c.md#markline)

Called by: [cq_feature](queries.c.md#cq_feature), [cq_printer](queries.c.md#cq_printer), [cq_query](queries.c.md#cq_query)

Dispatched via: [queries](queries.c.md#queries)

### gq_true

```c
int gq_true(struct papfile *out)
```

Defined at lines 129 to 137.

Answer true for a spooling printer.

Parameters:
* `out`: reply data

Returns: 0, or -1 for a printer that does not spool, which falls back to the default

Calls: [append](file.c.md#append)

Dispatched via: [genqueries](queries.c.md#genqueries)

### gq_pagecost

```c
int gq_pagecost(struct papfile *out)
```

Defined at lines 147 to 168.

Answer UMICHCostPerPage with the page cost message or, with accounting, the page cost.

Parameters:
* `out`: reply data

Returns: 0, or -1 without a message or accounting, which falls back to the default

Calls: [append](file.c.md#append), [lp_pagecost](lp.h.md#lp_pagecost)

Dispatched via: [genqueries](queries.c.md#genqueries)

### gq_rbispoolerid

```c
int gq_rbispoolerid(struct papfile *out)
```

Defined at lines 193 to 197.

Answer RBISpoolerID with the name and version of the papd spooler.

Parameters:
* `out`: reply data

Returns: 0

Calls: [append](file.c.md#append)

Uses file-scope variables: `spoolerid`

Dispatched via: [genqueries](queries.c.md#genqueries)

### gq_rbiuamlist

```c
int gq_rbiuamlist(struct papfile *out)
```

Defined at lines 210 to 226.

Answer RBIUAMListQuery with the print UAMs, one per line, ending with `*`

A printer without print UAM authentication answers `*` alone.

Parameters:
* `out`: reply data

Returns: 0

Calls: [append](file.c.md#append), [getuamnames](auth.c.md#getuamnames)

Uses file-scope variables: `nouams`

Dispatched via: [genqueries](queries.c.md#genqueries)

### gq_product

```c
int gq_product(struct papfile *out)
```

Defined at lines 234 to 241.

Answer the Product query with the `*Product` value of the PPD file.

Parameters:
* `out`: reply data

Returns: 0

Calls: [append](file.c.md#append)

Dispatched via: [genqueries](queries.c.md#genqueries)

### cq_query

```c
int cq_query(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 276 to 345.

Answer a `%%?BeginQuery` general query.

The keyword after the colon picks a handler from the genqueries table: UMICHCostPerPage, RBISpoolerID, RBIUAMListQuery, Product, ADOIsBinaryOK?, UMICHListQueue, or UMICHDeleteJob. An unknown keyword, or a handler that returns -1, passes the query to cq_default. The call then drops the lines up to the end comment.

Parameters:
* `in`: data from the client
* `out`: reply data
* `sat`: client address, unused

Returns: CH_DONE, 0 at the end of the input, or CH_MORE while a line is incomplete

Calls: [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [comswitch](comment.c.md#comswitch), [cq_default](queries.c.md#cq_default), [markline](file.c.md#markline)

Calls through [`genquery::gq_handler`](queries.c.md#struct-genquery): no table assigns this field

Uses file-scope variables: `genqueries`, `queries`

Dispatched via: [queries](queries.c.md#queries)

### cq_font_answer

```c
void cq_font_answer(char *start, char *stop, struct papfile *out)
```

Defined at lines 357 to 398.

Answer each font name of a font query with `/Name:Yes` or `/Name:No`

The answer says whether the PPD file lists the font. Names of 256 bytes or more get no answer.

Parameters:
* `start`: first byte of the font names
* `stop`: end of the font names
* `out`: reply data

Calls: [append](file.c.md#append)

Called by: [cq_font](queries.c.md#cq_font)

### cq_fontlist

```c
int cq_fontlist(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 412 to 481.

Answer a `%%?BeginFontListQuery` with the 35 standard PostScript fonts.

The list ends with `*`, and PF_FONT_QUERY makes session send it a line per packet. The call then drops the lines up to the end comment.

Parameters:
* `in`: data from the client
* `out`: reply data, receives PF_FONT_QUERY
* `sat`: client address, unused

Returns: CH_DONE, 0 at the end of the input, or CH_MORE while a line is incomplete

Calls: [append](file.c.md#append), [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [markline](file.c.md#markline)

Dispatched via: [queries](queries.c.md#queries)

### cq_font

```c
int cq_font(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 497 to 561.

Answer a `%%?BeginFontQuery` for the fonts on the query line and on its `%%+` continuations.

Each font gets its answer from cq_font_answer, and the end comment adds a final `*`.

Parameters:
* `in`: data from the client
* `out`: reply data
* `sat`: client address, unused

Returns: CH_DONE, 0 at the end of the input, or CH_MORE while a line is incomplete

Calls: [append](file.c.md#append), [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [cq_font_answer](queries.c.md#cq_font_answer), [markline](file.c.md#markline)

Uses file-scope variables: `comcont` in [etc/papd/comment.c](comment.c.md)

Dispatched via: [queries](queries.c.md#queries)

### cq_feature

```c
int cq_feature(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 576 to 643.

Answer a `%%?BeginFeatureQuery` with the PPD value of the feature.

The keyword after the colon names the feature, and the LaserWriter 8 form `*?Resolution` counts as `*Resolution`. A feature without a value passes the query to cq_default.

Parameters:
* `in`: data from the client
* `out`: reply data
* `sat`: client address, unused

Returns: CH_DONE, 0 at the end of the input, or CH_MORE while a line is incomplete

Calls: [append](file.c.md#append), [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [comswitch](comment.c.md#comswitch), [cq_default](queries.c.md#cq_default), [markline](file.c.md#markline)

Uses file-scope variables: `queries`

Dispatched via: [queries](queries.c.md#queries)

### cq_printer

```c
int cq_printer(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 662 to 739.

Answer a `%%?BeginPrinterQuery` with the PostScript revision, version, and product.

The values come from `*PSVersion`, such as `(3010.000) 0`, and `*Product` of the PPD file. A missing value, or a `*PSVersion` without a space, passes the query to cq_default.

Parameters:
* `in`: data from the client
* `out`: reply data
* `sat`: client address, unused

Returns: CH_DONE, 0 at the end of the input, or CH_MORE while a line is incomplete

Calls: [append](file.c.md#append), [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [comswitch](comment.c.md#comswitch), [cq_default](queries.c.md#cq_default), [markline](file.c.md#markline)

Uses file-scope variables: `prod`, `psver`, `queries`

Dispatched via: [queries](queries.c.md#queries)

### cq_rbilogin

```c
int cq_rbilogin(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 842 to 906.

Log a user in for printing with the `%%?BeginQuery: RBILogin` query.

The word after RBILogin names the print UAM, and the rest of the line goes to the uam_printer call of that UAM. A successful login makes the user the authenticated user of the job through lp_person and answers `0`. A failure answers `-1` with a PostScript security error that asks the client to drop the rest of the job.

Parameters:
* `in`: data from the client
* `out`: reply data
* `sat`: client address, unused

Returns: CH_DONE, 0 at the end of the input, or CH_MORE while a line is incomplete

Calls: [append](file.c.md#append), [auth_uamfind](auth.c.md#auth_uamfind), [comcmp](comment.c.md#comcmp), [compop](comment.c.md#compop), [lp_person](lp.c.md#lp_person), [markline](file.c.md#markline)

Calls through [`uam_obj::uam_printer`](../afpd/uam_auth.h.md#struct-uam_obj): no table assigns this field

Uses file-scope variables: `papd_uam`, `rbiloginbad`, `rbiloginerrstr`, `rbiloginok`

Dispatched via: [queries](queries.c.md#queries)

### cq_end

```c
int cq_end(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 920 to 941.

End a query job at `%EOF`

The call sets PF_QUERY on the input, so cm_psquery ends the job. It pops the comment state and consumes the line.

Parameters:
* `in`: data from the client, receives PF_QUERY
* `out`: reply data, unused
* `sat`: client address, unused

Returns: CH_DONE, 0 at the end of the input, or CH_MORE while the line is incomplete

Calls: [compop](comment.c.md#compop), [markline](file.c.md#markline)

Dispatched via: [queries](queries.c.md#queries)

# Types

### struct genquery

Defined at line 244.
* `char * gq_name`
* `int(* gq_handler`: Called through by [cq_query](queries.c.md#cq_query).

# File-scope variables

`nouams`, `papd_uam`, `prod`, `psver`, `rbiloginbad`, `rbiloginerrstr`, `rbiloginok`, `spoolerid`
