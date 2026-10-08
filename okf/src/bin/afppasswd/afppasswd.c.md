---
type: C Source File
title: "bin/afppasswd/afppasswd.c"
description: "AFP user password utility."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/afppasswd/afppasswd.c"
tags: ["bin/afppasswd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/afppasswd](../afppasswd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afppasswd_migrate.h](afppasswd_migrate.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/constant_time.h](../../include/atalk/constant_time.h.md)
* [atalk/srp.h](../../include/atalk/srp.h.md)
* [atalk/uam.h](../../include/atalk/uam.h.md)
* System headers: `arpa/inet.h`, `crack.h`, `ctype.h`, `errno.h`, `fcntl.h`, `gcrypt.h`, `inttypes.h`, `pwd.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### unhex

```c
static int unhex(unsigned char x)
```

Defined at lines 81 to 84.

Called by: [convert_passwd](afppasswd.c.md#convert_passwd), [randnum_read_keyfd](afppasswd.c.md#randnum_read_keyfd), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

### initialize_libgcrypt

```c
static int initialize_libgcrypt(void)
```

Defined at lines 94 to 105.

Called by: [main](afppasswd.c.md#main)

### parse_minimum_uid

```c
static int parse_minimum_uid(const char *value, uid_t *uid)
```

Defined at lines 107 to 127.

Called by: [main](afppasswd.c.md#main)

### validate_opened_file

```c
static int validate_opened_file(int fd, const char *path)
```

Defined at lines 129 to 164.

Called by: [open_credential_file](afppasswd.c.md#open_credential_file), [open_credential_for_replacement](afppasswd.c.md#open_credential_for_replacement), [randnum_ensure_keyfile](afppasswd.c.md#randnum_ensure_keyfile)

### open_credential_file

```c
static int open_credential_file(const char *path, int access_mode)
```

Defined at lines 166 to 183.

Calls: [validate_opened_file](afppasswd.c.md#validate_opened_file)

Called by: [randnum_open_keyfile](afppasswd.c.md#randnum_open_keyfile), [update_passwd](afppasswd.c.md#update_passwd)

### find_append_position

```c
static int find_append_position(FILE *fp, const char *path, off_t *pos)
```

Defined at lines 185 to 214.

Called by: [update_passwd](afppasswd.c.md#update_passwd)

### open_credential_for_replacement

```c
static int open_credential_for_replacement(const char *path)
```

Defined at lines 216 to 239.

Calls: [validate_opened_file](afppasswd.c.md#validate_opened_file)

Called by: [create_file](afppasswd.c.md#create_file), [randnum_write_keyfile](afppasswd.c.md#randnum_write_keyfile)

### randnum_make_keypath

```c
static int randnum_make_keypath(const char *path, char *keypath, size_t keypath_size)
```

Defined at lines 241 to 255.

Calls: [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [randnum_ensure_keyfile](afppasswd.c.md#randnum_ensure_keyfile), [randnum_open_keyfile](afppasswd.c.md#randnum_open_keyfile)

### randnum_read_keyfd

```c
static int randnum_read_keyfd(int keyfd, uint8_t key[DES_KEY_SZ], const char *keypath)
```

Defined at lines 257 to 304.

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [unhex](afppasswd.c.md#unhex)

Called by: [convert_passwd](afppasswd.c.md#convert_passwd), [randnum_ensure_keyfile](afppasswd.c.md#randnum_ensure_keyfile), [randnum_open_keyfile](afppasswd.c.md#randnum_open_keyfile)

### randnum_open_keyfile

```c
static int randnum_open_keyfile(const char *path, int *keyfd_out)
```

Defined at lines 306 to 332.

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [open_credential_file](afppasswd.c.md#open_credential_file), [randnum_make_keypath](afppasswd.c.md#randnum_make_keypath), [randnum_read_keyfd](afppasswd.c.md#randnum_read_keyfd)

Called by: [update_passwd](afppasswd.c.md#update_passwd)

### randnum_write_keyfile

```c
static int randnum_write_keyfile(const char *keypath)
```

Defined at lines 334 to 368.

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [open_credential_for_replacement](afppasswd.c.md#open_credential_for_replacement)

Called by: [randnum_ensure_keyfile](afppasswd.c.md#randnum_ensure_keyfile)

Uses file-scope variables: `hextable`

### randnum_ensure_keyfile

```c
static int randnum_ensure_keyfile(const char *path, int flags)
```

Defined at lines 370 to 414.

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [randnum_make_keypath](afppasswd.c.md#randnum_make_keypath), [randnum_read_keyfd](afppasswd.c.md#randnum_read_keyfd), [randnum_write_keyfile](afppasswd.c.md#randnum_write_keyfile), [validate_opened_file](afppasswd.c.md#validate_opened_file)

Called by: [main](afppasswd.c.md#main)

### convert_passwd

```c
static int convert_passwd(char *passwd_buf, char *newpwd, const int keyfd)
```

Defined at lines 418 to 485.

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [randnum_read_keyfd](afppasswd.c.md#randnum_read_keyfd), [unhex](afppasswd.c.md#unhex)

Called by: [update_passwd](afppasswd.c.md#update_passwd)

Uses file-scope variables: `hextable`

### srp_compute_verifier

```c
static int srp_compute_verifier(const char *username, const char *password, const unsigned char *salt, unsigned char *v_out)
```

Defined at lines 493 to 558.

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

Uses file-scope variables: `srp_N_bytes` in [include/atalk/srp.h](../../include/atalk/srp.h.md), `srp_g_byte` in [include/atalk/srp.h](../../include/atalk/srp.h.md)

### srp_encode_hex

```c
static void srp_encode_hex(char *out_hex, const unsigned char *salt, const unsigned char *verifier)
```

Defined at lines 564 to 580.

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

Uses file-scope variables: `hextable`

Mentioned in the documentation of: [srp_write_record](afppasswd.c.md#srp_write_record)

### open_srp_verifier_directory

```c
static int open_srp_verifier_directory(const char *path)
```

Defined at lines 582 to 622.

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [create_srp_directory](afppasswd.c.md#create_srp_directory), [disable_srp_verifier](afppasswd.c.md#disable_srp_verifier), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

### srp_uid_filename

```c
static int srp_uid_filename(uid_t uid, char *name, size_t size)
```

Defined at lines 624 to 628.

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [open_srp_verifier](afppasswd.c.md#open_srp_verifier)

### validate_srp_verifier_file

```c
static int validate_srp_verifier_file(int fd, uid_t uid, const char *path, int administrative)
```

Defined at lines 630 to 647.

Calls: [srp_verifier_mode_is_safe](../../include/atalk/srp.h.md#srp_verifier_mode_is_safe)

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [open_srp_verifier](afppasswd.c.md#open_srp_verifier)

### open_srp_verifier

```c
static int open_srp_verifier(int dirfd, const char *path, uid_t uid, int create)
```

Defined at lines 649 to 700.

Calls: [srp_uid_filename](afppasswd.c.md#srp_uid_filename), [validate_srp_verifier_file](afppasswd.c.md#validate_srp_verifier_file)

Called by: [create_srp_directory](afppasswd.c.md#create_srp_directory), [disable_srp_verifier](afppasswd.c.md#disable_srp_verifier), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

### collect_new_password

```c
static int collect_new_password(char *password, size_t password_size, const char *pass, int flags)
```

Defined at lines 715 to 785.

Read and validate a new SRP password before anything is written.

From -w when given, else prompted for and confirmed. Applies the length bound and, when built with cracklib, the dictionary check.

Parameters:
* `password`: receives the accepted password
* `password_size`: size of that buffer, SRP_PASSWDLEN + 1
* `pass`: -w value, "" when not given
* `flags`: OPT_* bits, for OPT_NOCRACK

Returns: 0 on success, -1 on any rejection (message already printed)

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

### srp_write_record

```c
static int srp_write_record(int fd, const char *path, const char *name, const char *hex_buf, uid_t owner)
```

Defined at lines 803 to 856.

Write one verifier record as the whole content of a store file.

Under the write lock: "name:hex_salt:hex_verifier\n" from offset 0, truncate to that length, fsync, and hand the file to its owner when asked.

Parameters:
* `fd`: the uid-named verifier file, open for writing
* `path`: the verifier directory, for messages
* `name`: the record's username, already validated
* `hex_buf`: SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN bytes from [srp_encode_hex()](afppasswd.c.md#srp_encode_hex), not NUL-terminated
* `owner`: uid the durable record is handed to, root's enrollment step; (uid_t) -1 leaves ownership alone

Returns: 0 on success, -1 on failure (message already printed)

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

### update_srp_passwd

```c
static int update_srp_passwd(const char *path, const char *name, uid_t uid, int flags, const char *pass)
```

Defined at lines 858 to 1058.

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [collect_new_password](afppasswd.c.md#collect_new_password), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [open_srp_verifier](afppasswd.c.md#open_srp_verifier), [open_srp_verifier_directory](afppasswd.c.md#open_srp_verifier_directory), [srp_compute_verifier](afppasswd.c.md#srp_compute_verifier), [srp_encode_hex](afppasswd.c.md#srp_encode_hex), [srp_valid_fields](../../include/atalk/srp.h.md#srp_valid_fields), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [srp_write_record](afppasswd.c.md#srp_write_record), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [unhex](afppasswd.c.md#unhex)

Called by: [main](afppasswd.c.md#main)

### disable_srp_verifier

```c
static int disable_srp_verifier(const char *path, const char *name, uid_t uid)
```

Defined at lines 1061 to 1142.

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [open_srp_verifier](afppasswd.c.md#open_srp_verifier), [open_srp_verifier_directory](afppasswd.c.md#open_srp_verifier_directory), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [main](afppasswd.c.md#main)

### create_srp_directory

```c
static int create_srp_directory(const char *path, uid_t minuid)
```

Defined at lines 1144 to 1222.

Calls: [open_srp_verifier](afppasswd.c.md#open_srp_verifier), [open_srp_verifier_directory](afppasswd.c.md#open_srp_verifier_directory), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [main](afppasswd.c.md#main)

Uses file-scope variables: `buf`

### private_store_is_callers

```c
static int private_store_is_callers(const struct stat *st, uid_t uid)
```

Defined at lines 1227 to 1231.

Whether a stat result is the caller's own mode-0700 directory.

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier)

### create_private_srp_verifier

```c
static int create_private_srp_verifier(const char *path, uid_t uid, int flags, const char *pass)
```

Defined at lines 1248 to 1424.

Bootstrap a single-user server's private SRP verifier directory.

Only the calling user's own uid file is created. Refusals that need no password come first, the password and record next, the store last, and a create that fails after the file exists removes it again.

Parameters:
* `path`: the verifier directory, created when absent
* `uid`: the calling user, owner of the directory and the record
* `flags`: OPT_FORCE replaces an existing verifier, OPT_NOCRACK skips the dictionary check
* `pass`: -w value, "" to prompt

Returns: 0 on success, -1 on failure (message already printed)

Calls: [collect_new_password](afppasswd.c.md#collect_new_password), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [open_srp_verifier_directory](afppasswd.c.md#open_srp_verifier_directory), [private_store_is_callers](afppasswd.c.md#private_store_is_callers), [srp_compute_verifier](afppasswd.c.md#srp_compute_verifier), [srp_encode_hex](afppasswd.c.md#srp_encode_hex), [srp_uid_filename](afppasswd.c.md#srp_uid_filename), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [srp_write_record](afppasswd.c.md#srp_write_record), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [validate_srp_verifier_file](afppasswd.c.md#validate_srp_verifier_file)

Called by: [main](afppasswd.c.md#main)

### valid_hex_or_disabled

```c
static int valid_hex_or_disabled(const char *field, size_t len)
```

Defined at lines 1428 to 1447.

Called by: [valid_randnum_record](afppasswd.c.md#valid_randnum_record)

### valid_randnum_record

```c
static int valid_randnum_record(const char *fields)
```

Defined at lines 1449 to 1458.

Calls: [valid_hex_or_disabled](afppasswd.c.md#valid_hex_or_disabled)

Called by: [update_passwd](afppasswd.c.md#update_passwd)

### update_passwd

```c
static int update_passwd(const char *path, const char *name, int flags, const char *pass)
```

Defined at lines 1461 to 1690.

Calls: [convert_passwd](afppasswd.c.md#convert_passwd), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [find_append_position](afppasswd.c.md#find_append_position), [open_credential_file](afppasswd.c.md#open_credential_file), [randnum_open_keyfile](afppasswd.c.md#randnum_open_keyfile), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [valid_randnum_record](afppasswd.c.md#valid_randnum_record)

Called by: [main](afppasswd.c.md#main)

Uses file-scope variables: `buf`

### create_file

```c
static int create_file(const char *path, uid_t minuid)
```

Defined at lines 1694 to 1733.

Calls: [open_credential_for_replacement](afppasswd.c.md#open_credential_for_replacement), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [main](afppasswd.c.md#main)

Uses file-scope variables: `buf`

### print_usage

```c
static void print_usage(void)
```

Defined at lines 1736 to 1764.

Called by: [main](afppasswd.c.md#main)

### main

```c
int main(int argc, char **argv)
```

Defined at lines 1766 to 2022.

Calls: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp), [create_file](afppasswd.c.md#create_file), [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [create_srp_directory](afppasswd.c.md#create_srp_directory), [disable_srp_verifier](afppasswd.c.md#disable_srp_verifier), [initialize_libgcrypt](afppasswd.c.md#initialize_libgcrypt), [parse_minimum_uid](afppasswd.c.md#parse_minimum_uid), [print_usage](afppasswd.c.md#print_usage), [randnum_ensure_keyfile](afppasswd.c.md#randnum_ensure_keyfile), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [update_passwd](afppasswd.c.md#update_passwd), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

# Macros

* Undocumented: `AFPPASSWD_OPTSTRING`, `DES_KEY_SZ`, `FORMAT`, `FORMAT_LEN`, `HEXPASSWDLEN`, `OPT_ADDUSER`, `OPT_CREATE`, `OPT_DISABLE`, `OPT_FORCE`, `OPT_ISROOT`, `OPT_MIGRATE`, `OPT_NOCRACK`, `OPT_RANDNUM`, `PASSWDLEN`, `PASSWD_ILLEGAL`, `SRP_PASSWDLEN`, `UID_START`

# File-scope variables

`buf`, `hextable`
