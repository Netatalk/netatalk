---
type: C Source File
title: "bin/afppasswd/afppasswd_migrate.c"
description: "18 functions, 1 type, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/afppasswd/afppasswd_migrate.c"
tags: ["bin/afppasswd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/afppasswd](../afppasswd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afppasswd_migrate.h](afppasswd_migrate.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/srp.h](../../include/atalk/srp.h.md)
* System headers: `dirent.h`, `errno.h`, `fcntl.h`, `inttypes.h`, `pwd.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### lookup_uid

```c
static int lookup_uid(const char *name, uid_t *uid)
```

Defined at lines 48 to 60.

Called by: [add_record](afppasswd_migrate.c.md#add_record)

### split_path

```c
static int split_path(const char *path, char parent[MAXPATHLEN+1], char basename[MAXPATHLEN+1])
```

Defined at lines 62 to 103.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### validate_parent

```c
static int validate_parent(int fd, const char *path, uid_t administrator_uid)
```

Defined at lines 105 to 119.

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### validate_source

```c
static int validate_source(int fd, const char *path, uid_t administrator_uid, struct stat *st)
```

Defined at lines 121 to 139.

Calls: [srp_verifier_mode_is_safe](../../include/atalk/srp.h.md#srp_verifier_mode_is_safe)

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### free_records

```c
static void free_records(struct migrate_record *records, size_t count)
```

Defined at lines 141 to 149.

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### add_record

```c
static int add_record(struct migrate_record **records, size_t *count, size_t *capacity, const char *line, size_t length, size_t source_length, size_t line_number)
```

Defined at lines 151 to 248.

Calls: [lookup_uid](afppasswd_migrate.c.md#lookup_uid), [srp_valid_fields](../../include/atalk/srp.h.md#srp_valid_fields), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username)

Called by: [read_records](afppasswd_migrate.c.md#read_records)

### read_records

```c
static int read_records(int source_fd, struct migrate_record **records, size_t *count)
```

Defined at lines 250 to 314.

Calls: [add_record](afppasswd_migrate.c.md#add_record)

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### source_unchanged

```c
static int source_unchanged(int source_fd, const struct migrate_record *records, size_t count)
```

Defined at lines 316 to 343.

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### write_all

```c
static int write_all(int fd, const char *data, size_t length)
```

Defined at lines 345 to 364.

Called by: [create_verifiers](afppasswd_migrate.c.md#create_verifiers)

### uid_filename

```c
static int uid_filename(uid_t uid, char *name, size_t size)
```

Defined at lines 366 to 370.

Called by: [create_verifiers](afppasswd_migrate.c.md#create_verifiers), [remove_staging](afppasswd_migrate.c.md#remove_staging)

### create_verifiers

```c
static int create_verifiers(int directory_fd, const char *path, const struct migrate_record *records, size_t count, uid_t administrator_uid)
```

Defined at lines 372 to 430.

Calls: [srp_verifier_mode_is_safe](../../include/atalk/srp.h.md#srp_verifier_mode_is_safe), [uid_filename](afppasswd_migrate.c.md#uid_filename), [write_all](afppasswd_migrate.c.md#write_all)

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### remove_staging

```c
static void remove_staging(int parent_fd, const char *temporary_name, const struct migrate_record *records, size_t count)
```

Defined at lines 432 to 451.

Calls: [uid_filename](afppasswd_migrate.c.md#uid_filename)

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### create_staging_directory

```c
static int create_staging_directory(int parent_fd, const char *basename, uid_t administrator_uid, char temporary_name[MAXPATHLEN+1])
```

Defined at lines 453 to 492.

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### source_still_at_path

```c
static int source_still_at_path(int parent_fd, const char *basename, const struct stat *source_st)
```

Defined at lines 494 to 501.

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### reject_stale_staging

```c
static int reject_stale_staging(int parent_fd, const char *basename, const char *path)
```

Defined at lines 503 to 557.

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### link_backup

```c
static int link_backup(int parent_fd, const char *basename, char backup_name[MAXPATHLEN+1])
```

Defined at lines 559 to 592.

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### restore_source

```c
static enum restore_result restore_source(int parent_fd, const char *basename, const char *backup_name)
```

Defined at lines 600 to 626.

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### afppasswd_migrate_srp

```c
int afppasswd_migrate_srp(const char *path, uid_t administrator_uid)
```

Defined at lines 628 to 832.

Calls: [create_staging_directory](afppasswd_migrate.c.md#create_staging_directory), [create_verifiers](afppasswd_migrate.c.md#create_verifiers), [free_records](afppasswd_migrate.c.md#free_records), [link_backup](afppasswd_migrate.c.md#link_backup), [read_records](afppasswd_migrate.c.md#read_records), [reject_stale_staging](afppasswd_migrate.c.md#reject_stale_staging), [remove_staging](afppasswd_migrate.c.md#remove_staging), [restore_source](afppasswd_migrate.c.md#restore_source), [source_still_at_path](afppasswd_migrate.c.md#source_still_at_path), [source_unchanged](afppasswd_migrate.c.md#source_unchanged), [split_path](afppasswd_migrate.c.md#split_path), [validate_parent](afppasswd_migrate.c.md#validate_parent), [validate_source](afppasswd_migrate.c.md#validate_source)

Called by: [main](afppasswd.c.md#main)

# Types

### struct migrate_record

Defined at line 40.
* `char * line`
* `size_t length`
* `size_t source_length`
* `char * username`
* `uid_t uid`

# Typedefs and enums

* `enum restore_result`: `RESTORE_FAILED`, `RESTORE_CLEANUP_INCOMPLETE`, `RESTORE_COMPLETE`

# Macros

* Undocumented: `MIGRATE_ATTEMPTS`
