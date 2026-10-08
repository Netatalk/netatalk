---
type: C Source File
title: "etc/papd/file.c"
description: "5 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/file.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/logger.h](../../include/atalk/logger.h.md)
* [file.h](file.h.md)
* System headers: `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Functions

### markline

```c
int markline(struct papfile *pf, char **start, int *linelength, int *crlflength)
```

Defined at lines 18 to 64.

Calls: [append](file.c.md#append)

Called by: [ch_creator](headers.c.md#ch_creator), [ch_endcomm](headers.c.md#ch_endcomm), [ch_endtranslate](headers.c.md#ch_endtranslate), [ch_for](headers.c.md#ch_for), [ch_starttranslate](headers.c.md#ch_starttranslate), [ch_title](headers.c.md#ch_title), [ch_translateone](headers.c.md#ch_translateone), [cm_psadobe](magics.c.md#cm_psadobe), [cm_psquery](magics.c.md#cm_psquery), [cm_psswitch](magics.c.md#cm_psswitch), [cq_default](queries.c.md#cq_default), [cq_end](queries.c.md#cq_end), [cq_feature](queries.c.md#cq_feature), [cq_font](queries.c.md#cq_font), [cq_fontlist](queries.c.md#cq_fontlist), [cq_printer](queries.c.md#cq_printer), [cq_query](queries.c.md#cq_query), [cq_rbilogin](queries.c.md#cq_rbilogin), [ps](magics.c.md#ps), [session](session.c.md#session)

### morespace

```c
void morespace(struct papfile *pf, const char *data, int len)
```

Defined at lines 66 to 97.

Called by: [append](file.c.md#append)

### append

```c
void append(struct papfile *pf, const char *data, int len)
```

Defined at lines 100 to 109.

Calls: [morespace](file.c.md#morespace)

Called by: [cq_default](queries.c.md#cq_default), [cq_feature](queries.c.md#cq_feature), [cq_font](queries.c.md#cq_font), [cq_font_answer](queries.c.md#cq_font_answer), [cq_fontlist](queries.c.md#cq_fontlist), [cq_printer](queries.c.md#cq_printer), [cq_rbilogin](queries.c.md#cq_rbilogin), [gq_pagecost](queries.c.md#gq_pagecost), [gq_product](queries.c.md#gq_product), [gq_rbispoolerid](queries.c.md#gq_rbispoolerid), [gq_rbiuamlist](queries.c.md#gq_rbiuamlist), [gq_true](queries.c.md#gq_true), [markline](file.c.md#markline), [session](session.c.md#session), [spoolerror](file.c.md#spoolerror), [spoolreply](file.c.md#spoolreply)

### spoolreply

```c
void spoolreply(struct papfile *out, char *str)
```

Defined at lines 112 to 124.

Calls: [append](file.c.md#append)

Called by: [cm_psquery](magics.c.md#cm_psquery), [ps](magics.c.md#ps)

### spoolerror

```c
void spoolerror(struct papfile *out, char *str)
```

Defined at lines 126 to 138.

Calls: [append](file.c.md#append)

Called by: [lp_init](lp.c.md#lp_init), [lp_open](lp.c.md#lp_open), [parser_error](magics.c.md#parser_error), [ps](magics.c.md#ps)
