---
type: C Header File
title: "etc/spotlight/xapian/sl_xapian.h"
description: "Xapian index maintenance and Spotlight query interface."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/xapian/sl_xapian.h"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [etc/spotlight](../../spotlight.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* System headers: `stddef.h`

# Included by

* [etc/spotlight/xapian/sl_xapian.c](sl_xapian.c.md)

# Functions

### sl_xapian_reconcile

```c
int sl_xapian_reconcile(const char *db_path, const char *volume_path, const char *volume_uuid, char *errbuf, size_t errlen)
```

Declared at etc/spotlight/xapian/sl_xapian.h line 22; no definition in the scanned sources.

Called by: [sl_xapian_ensure_seeded](sl_xapian.c.md#sl_xapian_ensure_seeded)

### sl_xapian_index_ready

```c
int sl_xapian_index_ready(const char *db_path, const char *volume_path, const char *volume_uuid, char *errbuf, size_t errlen)
```

Declared at etc/spotlight/xapian/sl_xapian.h line 27; no definition in the scanned sources.

Called by: [sl_xapian_ensure_seeded](sl_xapian.c.md#sl_xapian_ensure_seeded)

### sl_xapian_query

```c
int sl_xapian_query(const char *db_path, const char *scope, const char *qstring, size_t offset, size_t limit, char ***paths, size_t *count, int *more, char *errbuf, size_t errlen)
```

Declared at etc/spotlight/xapian/sl_xapian.h line 32; no definition in the scanned sources.

Called by: [sl_xapian_open_query](sl_xapian.c.md#sl_xapian_open_query)

### sl_xapian_upsert_path

```c
int sl_xapian_upsert_path(const char *db_path, const char *path, char *errbuf, size_t errlen)
```

Declared at etc/spotlight/xapian/sl_xapian.h line 42; no definition in the scanned sources.

Called by: [sl_xapian_index_event](sl_xapian.c.md#sl_xapian_index_event)

### sl_xapian_delete_path

```c
int sl_xapian_delete_path(const char *db_path, const char *path, char *errbuf, size_t errlen)
```

Declared at etc/spotlight/xapian/sl_xapian.h line 46; no definition in the scanned sources.

Called by: [sl_xapian_index_event](sl_xapian.c.md#sl_xapian_index_event)

### sl_xapian_delete_prefix

```c
int sl_xapian_delete_prefix(const char *db_path, const char *path, char *errbuf, size_t errlen)
```

Declared at etc/spotlight/xapian/sl_xapian.h line 50; no definition in the scanned sources.

Called by: [sl_xapian_index_event](sl_xapian.c.md#sl_xapian_index_event)

### sl_xapian_reindex_subtree

```c
int sl_xapian_reindex_subtree(const char *db_path, const char *volume_path, const char *path, const char *oldpath, char *errbuf, size_t errlen)
```

Declared at etc/spotlight/xapian/sl_xapian.h line 54; no definition in the scanned sources.

Called by: [sl_xapian_index_event](sl_xapian.c.md#sl_xapian_index_event)

### sl_xapian_mark_dirty

```c
int sl_xapian_mark_dirty(const char *db_path, char *errbuf, size_t errlen)
```

Declared at etc/spotlight/xapian/sl_xapian.h line 60; no definition in the scanned sources.

Called by: [sl_xapian_index_event](sl_xapian.c.md#sl_xapian_index_event)

### sl_xapian_free_paths

```c
void sl_xapian_free_paths(char **paths, size_t count)
```

Declared at etc/spotlight/xapian/sl_xapian.h line 63; no definition in the scanned sources.

Called by: [sl_xapian_close_query](sl_xapian.c.md#sl_xapian_close_query)
