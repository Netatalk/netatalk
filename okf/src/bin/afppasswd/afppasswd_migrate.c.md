---
type: C Source File
title: "bin/afppasswd/afppasswd_migrate.c"
description: "Migration of the legacy flat SRP verifier file into one verifier file per user."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/afppasswd/afppasswd_migrate.c"
tags: ["bin/afppasswd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 61 to 73.

Look up the UID of a local user name.

Parameters:
* `name`: user name
* `uid`: receives the UID

Returns: 0, or -1 for an unknown user

Called by: [add_record](afppasswd_migrate.c.md#add_record)

### split_path

```c
static int split_path(const char *path, char parent[MAXPATHLEN+1], char basename[MAXPATHLEN+1])
```

Defined at lines 87 to 128.

Split the path of the legacy verifier file into its directory and its file name.

A path without a slash lives in `.`, and a file at the root lives in `/`. An empty or too long path, a trailing slash, `.`, or `..` fails.

Parameters:
* `path`: file path
* `parent`: receives the directory
* `basename`: receives the file name

Returns: 0, or -1 after a message

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### validate_parent

```c
static int validate_parent(int fd, const char *path, uid_t administrator_uid)
```

Defined at lines 139 to 153.

Check that the directory of the legacy verifier file belongs to the administrator and is not writable by group or other.

Parameters:
* `fd`: open directory
* `path`: file path for the message
* `administrator_uid`: expected owner

Returns: 0, or -1 after a message

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### validate_source

```c
static int validate_source(int fd, const char *path, uid_t administrator_uid, struct stat *st)
```

Defined at lines 165 to 183.

Check that the legacy verifier file is a private single-link regular file of the administrator.

Parameters:
* `fd`: open file
* `path`: file path for the messages
* `administrator_uid`: expected owner
* `st`: receives the stat result

Returns: 0, or -1 after a message

Calls: [srp_verifier_mode_is_safe](../../include/atalk/srp.h.md#srp_verifier_mode_is_safe)

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### free_records

```c
static void free_records(struct migrate_record *records, size_t count)
```

Defined at lines 191 to 199.

Free the migration records and their array.

Parameters:
* `records`: record array
* `count`: number of records

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### add_record

```c
static int add_record(struct migrate_record **records, size_t *count, size_t *capacity, const char *line, size_t length, size_t source_length, size_t line_number)
```

Defined at lines 218 to 315.

Parse one line of the legacy verifier file into a migration record.

The line has to hold a valid user name, a colon, and valid SRP fields, without a NUL. The user has to exist on the system, and no two records may share a user name or a UID. The array grows by doubling from 16 entries.

Parameters:
* `records`: record array, grown as needed
* `count`: number of records
* `capacity`: allocated entries
* `line`: line with its newline
* `length`: length of line
* `source_length`: length of the line in the file, without an added newline
* `line_number`: line number for the messages

Returns: 0, or -1 after a message

Calls: [lookup_uid](afppasswd_migrate.c.md#lookup_uid), [srp_valid_fields](../../include/atalk/srp.h.md#srp_valid_fields), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username)

Called by: [read_records](afppasswd_migrate.c.md#read_records)

### read_records

```c
static int read_records(int source_fd, struct migrate_record **records, size_t *count)
```

Defined at lines 329 to 393.

Read every record of the legacy verifier file.

The call rewinds a dup of the descriptor and reads the file from its start. A line over SRP_USERNAME_MAX_LEN + SRP_FORMAT_LEN bytes fails as malformed, and a last line without a newline gets one.

Parameters:
* `source_fd`: open legacy file
* `records`: receives the record array
* `count`: receives the number of records

Returns: 0, or -1 after a message

Calls: [add_record](afppasswd_migrate.c.md#add_record)

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### source_unchanged

```c
static int source_unchanged(int source_fd, const struct migrate_record *records, size_t count)
```

Defined at lines 405 to 432.

Read the legacy verifier file again and compare it byte for byte with the records read before.

Parameters:
* `source_fd`: open legacy file
* `records`: records from read_records
* `count`: number of records

Returns: 1 when the file still holds the same bytes, or 0 for a change or a read error

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### write_all

```c
static int write_all(int fd, const char *data, size_t length)
```

Defined at lines 444 to 463.

Write every byte of a buffer, looping over short writes.

A write that returns 0 counts as EIO.

Parameters:
* `fd`: file
* `data`: bytes to write
* `length`: number of bytes

Returns: 0, or -1 with errno set

Called by: [create_verifiers](afppasswd_migrate.c.md#create_verifiers)

### uid_filename

```c
static int uid_filename(uid_t uid, char *name, size_t size)
```

Defined at lines 473 to 477.

Write the decimal UID that names a migrated verifier file.

Parameters:
* `uid`: user ID
* `name`: receives the file name
* `size`: size of name

Returns: 0, or -1 when the name does not fit

Called by: [create_verifiers](afppasswd_migrate.c.md#create_verifiers), [remove_staging](afppasswd_migrate.c.md#remove_staging)

### create_verifiers

```c
static int create_verifiers(int directory_fd, const char *path, const struct migrate_record *records, size_t count, uid_t administrator_uid)
```

Defined at lines 494 to 552.

Create a verifier file for each record in the staging directory.

Each file gets the decimal UID as its name, mode 0600, and the record as its content. An active record goes to its user as the owner, and a disabled record stays with the administrator. The call checks the result of each file on the disk.

Parameters:
* `directory_fd`: open staging directory
* `path`: legacy file path for the messages
* `records`: records to write
* `count`: number of records
* `administrator_uid`: owner of disabled records

Returns: 0, or -1 after a message

Calls: [srp_verifier_mode_is_safe](../../include/atalk/srp.h.md#srp_verifier_mode_is_safe), [uid_filename](afppasswd_migrate.c.md#uid_filename), [write_all](afppasswd_migrate.c.md#write_all)

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### remove_staging

```c
static void remove_staging(int parent_fd, const char *temporary_name, const struct migrate_record *records, size_t count)
```

Defined at lines 563 to 582.

Remove the verifier files of the records and then the staging directory.

Parameters:
* `parent_fd`: open directory of the legacy file
* `temporary_name`: name of the staging directory
* `records`: records whose files to remove
* `count`: number of records

Calls: [uid_filename](afppasswd_migrate.c.md#uid_filename)

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### create_staging_directory

```c
static int create_staging_directory(int parent_fd, const char *basename, uid_t administrator_uid, char temporary_name[MAXPATHLEN+1])
```

Defined at lines 597 to 636.

Create the staging directory `.name.migrate.PID` next to the legacy file.

The directory gets mode 0700 and the administrator as its owner. A failure removes it again.

Parameters:
* `parent_fd`: open directory of the legacy file
* `basename`: name of the legacy file
* `administrator_uid`: owner of the directory
* `temporary_name`: receives the name of the directory

Returns: the open directory, or -1 after a message

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### source_still_at_path

```c
static int source_still_at_path(int parent_fd, const char *basename, const struct stat *source_st)
```

Defined at lines 647 to 654.

Tell whether the path of the legacy file still names the same single-link file.

Parameters:
* `parent_fd`: open directory of the legacy file
* `basename`: name of the legacy file
* `source_st`: stat result of the file that the migration read

Returns: nonzero when the device, inode, and link count still match, or 0

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### reject_stale_staging

```c
static int reject_stale_staging(int parent_fd, const char *basename, const char *path)
```

Defined at lines 667 to 721.

Refuse a migration while a staging directory of an earlier run exists.

Any entry of the directory that starts with `.name.migrate.` stops the migration, so the administrator can inspect it first.

Parameters:
* `parent_fd`: open directory of the legacy file
* `basename`: name of the legacy file
* `path`: legacy file path for the messages

Returns: 0, or -1 after a message

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### link_backup

```c
static int link_backup(int parent_fd, const char *basename, char backup_name[MAXPATHLEN+1])
```

Defined at lines 734 to 767.

Keep the legacy file under a backup name through a hard link.

The call takes the first free name among `name.legacy`, `name.legacy.1`, and onward, within MIGRATE_ATTEMPTS tries.

Parameters:
* `parent_fd`: open directory of the legacy file
* `basename`: name of the legacy file
* `backup_name`: receives the backup name

Returns: 0, or -1 after a message

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### restore_source

```c
static enum restore_result restore_source(int parent_fd, const char *basename, const char *backup_name)
```

Defined at lines 785 to 811.

Put the legacy file back under its name from the backup and remove the backup.

Parameters:
* `parent_fd`: open directory of the legacy file
* `basename`: name of the legacy file
* `backup_name`: backup name from link_backup

Returns: RESTORE_COMPLETE, RESTORE_CLEANUP_INCOMPLETE when the backup stays, or RESTORE_FAILED when the original remains under the backup name

Called by: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp)

### afppasswd_migrate_srp

```c
int afppasswd_migrate_srp(const char *path, uid_t administrator_uid)
```

Defined at lines 831 to 1035.

Replace a legacy flat SRP verifier file with a verifier directory at the same path.

The file and its directory have to be private to the administrator, and a shared lock on the file fails while afpd holds it. Every record has to parse and name a local user. The new directory takes shape in a staging sibling. Once the file proves unchanged and a hard link keeps it as `name.legacy`, a rename puts the directory in place.

Any failure after the original name goes away rolls back to the backup. The messages name the backup when that fails as well.

Parameters:
* `path`: legacy verifier file
* `administrator_uid`: owner of the file, the directory, and disabled records

Returns: 0, or -1 after a message

Calls: [create_staging_directory](afppasswd_migrate.c.md#create_staging_directory), [create_verifiers](afppasswd_migrate.c.md#create_verifiers), [free_records](afppasswd_migrate.c.md#free_records), [link_backup](afppasswd_migrate.c.md#link_backup), [read_records](afppasswd_migrate.c.md#read_records), [reject_stale_staging](afppasswd_migrate.c.md#reject_stale_staging), [remove_staging](afppasswd_migrate.c.md#remove_staging), [restore_source](afppasswd_migrate.c.md#restore_source), [source_still_at_path](afppasswd_migrate.c.md#source_still_at_path), [source_unchanged](afppasswd_migrate.c.md#source_unchanged), [split_path](afppasswd_migrate.c.md#split_path), [validate_parent](afppasswd_migrate.c.md#validate_parent), [validate_source](afppasswd_migrate.c.md#validate_source)

Called by: [main](afppasswd.c.md#main)

# Types

### struct migrate_record

Defined at line 46.
* `char * line`
* `size_t length`
* `size_t source_length`
* `char * username`
* `uid_t uid`

# Typedefs and enums

* `enum restore_result`: `RESTORE_FAILED`, `RESTORE_CLEANUP_INCOMPLETE`, `RESTORE_COMPLETE`

# Macros

* Undocumented: `MIGRATE_ATTEMPTS`
