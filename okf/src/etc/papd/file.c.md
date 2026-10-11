---
type: C Source File
title: "etc/papd/file.c"
description: "5 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/file.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Defined at lines 32 to 78.

Find the next line in the input buffer of a print job.

The line runs up to the first CR or LF. Every CR and LF that follows counts as its line end, so a blank line never comes back as a separate line. At the end of the job the call appends a newline to a last line that has none.

Parameters:
* `pf`: input buffer of the job
* `start`: receives the first byte of the line
* `linelength`: receives the line length without the line end
* `crlflength`: receives the length of the line end

Returns: 1 for a line, 0 at the end of the job, or -1 while the line is incomplete

Calls: [append](file.c.md#append)

Called by: [ch_creator](headers.c.md#ch_creator), [ch_endcomm](headers.c.md#ch_endcomm), [ch_endtranslate](headers.c.md#ch_endtranslate), [ch_for](headers.c.md#ch_for), [ch_starttranslate](headers.c.md#ch_starttranslate), [ch_title](headers.c.md#ch_title), [ch_translateone](headers.c.md#ch_translateone), [cm_psadobe](magics.c.md#cm_psadobe), [cm_psquery](magics.c.md#cm_psquery), [cm_psswitch](magics.c.md#cm_psswitch), [cq_default](queries.c.md#cq_default), [cq_end](queries.c.md#cq_end), [cq_feature](queries.c.md#cq_feature), [cq_font](queries.c.md#cq_font), [cq_fontlist](queries.c.md#cq_fontlist), [cq_printer](queries.c.md#cq_printer), [cq_query](queries.c.md#cq_query), [cq_rbilogin](queries.c.md#cq_rbilogin), [ps](magics.c.md#ps), [session](session.c.md#session)

### morespace

```c
void morespace(struct papfile *pf, const char *data, int len)
```

Defined at lines 91 to 122.

Append data to a papfile buffer, after moving the data to the buffer start and growing the buffer as needed.

The buffer grows in steps of PF_MORESPACE bytes. A failed allocation exits papd.

Parameters:
* `pf`: papfile buffer
* `data`: bytes to append
* `len`: number of bytes

Called by: [append](file.c.md#append)

### append

```c
void append(struct papfile *pf, const char *data, int len)
```

Defined at lines 130 to 139.

Append data to a papfile buffer.

Stub of the papd append call for the UAM modules that afpd loads, which ignores its arguments.

Data that does not fit after the current data goes through morespace.

Calls: [morespace](file.c.md#morespace)

Called by: [cq_default](queries.c.md#cq_default), [cq_feature](queries.c.md#cq_feature), [cq_font](queries.c.md#cq_font), [cq_font_answer](queries.c.md#cq_font_answer), [cq_fontlist](queries.c.md#cq_fontlist), [cq_printer](queries.c.md#cq_printer), [cq_rbilogin](queries.c.md#cq_rbilogin), [gq_pagecost](queries.c.md#gq_pagecost), [gq_product](queries.c.md#gq_product), [gq_rbispoolerid](queries.c.md#gq_rbispoolerid), [gq_rbiuamlist](queries.c.md#gq_rbiuamlist), [gq_true](queries.c.md#gq_true), [markline](file.c.md#markline), [session](session.c.md#session), [spoolerror](file.c.md#spoolerror), [spoolreply](file.c.md#spoolreply)

### spoolreply

```c
void spoolreply(struct papfile *out, char *str)
```

Defined at lines 149 to 161.

Append a PostScript status message for the client, `%%[ status: str ]%%`.

Parameters:
* `out`: output buffer to the client
* `str`: message, or NULL for the default `Spooler error.` text

Calls: [append](file.c.md#append)

Called by: [cm_psquery](magics.c.md#cm_psquery), [ps](magics.c.md#ps)

### spoolerror

```c
void spoolerror(struct papfile *out, char *str)
```

Defined at lines 169 to 181.

Append a PostScript error message for the client, `%%[ Error: str ]%%`.

Parameters:
* `out`: output buffer to the client
* `str`: message, or NULL for the default `Spooler error.` text

Calls: [append](file.c.md#append)

Called by: [lp_init](lp.c.md#lp_init), [lp_open](lp.c.md#lp_open), [parser_error](magics.c.md#parser_error), [ps](magics.c.md#ps)
