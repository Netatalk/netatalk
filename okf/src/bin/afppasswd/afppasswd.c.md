---
type: C Source File
title: "bin/afppasswd/afppasswd.c"
description: "AFP user password utility."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/afppasswd/afppasswd.c"
tags: ["bin/afppasswd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 87 to 90.

Return the value of a hex digit, without checking that it is one.

Parameters:
* `x`: hex digit

Returns: 0 to 15 for a hex digit

Called by: [convert_passwd](afppasswd.c.md#convert_passwd), [randnum_read_keyfd](afppasswd.c.md#randnum_read_keyfd), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

### initialize_libgcrypt

```c
static int initialize_libgcrypt(void)
```

Defined at lines 105 to 116.

Initialize libgcrypt with the system random number generator and check its version.

Libgcrypt needs gcry_check_version() before any other call. The unit test of the SRP helpers calls this as well, since it does not run main.

Returns: 0, or -1 after a message for a libgcrypt older than UAM_NEED_LIBGCRYPT_VERSION

Called by: [main](afppasswd.c.md#main)

### parse_minimum_uid

```c
static int parse_minimum_uid(const char *value, uid_t *uid)
```

Defined at lines 128 to 148.

Parse the decimal minimum UID of the -u option.

The value has to be a plain decimal number that fits uid_t, without a sign, leading space, or trailing characters.

Parameters:
* `value`: option value
* `uid`: receives the UID

Returns: 0, or -1 for a value that does not parse

Called by: [main](afppasswd.c.md#main)

### validate_opened_file

```c
static int validate_opened_file(int fd, const char *path)
```

Defined at lines 160 to 195.

Check that an open credential file is private to the administrator.

The file has to be a regular file with exactly one hard link, owned by the effective UID, and without group or other permissions.

Parameters:
* `fd`: open file
* `path`: file path for the messages

Returns: 0, or -1 after a message

Called by: [open_credential_file](afppasswd.c.md#open_credential_file), [open_credential_for_replacement](afppasswd.c.md#open_credential_for_replacement), [randnum_ensure_keyfile](afppasswd.c.md#randnum_ensure_keyfile)

### open_credential_file

```c
static int open_credential_file(const char *path, int access_mode)
```

Defined at lines 205 to 222.

Open an existing credential file without following a symbolic link and check it with validate_opened_file.

Parameters:
* `path`: file path
* `access_mode`: O_RDONLY or O_RDWR

Returns: the descriptor, or -1 after a message

Calls: [validate_opened_file](afppasswd.c.md#validate_opened_file)

Called by: [randnum_open_keyfile](afppasswd.c.md#randnum_open_keyfile), [update_passwd](afppasswd.c.md#update_passwd)

### find_append_position

```c
static int find_append_position(FILE *fp, const char *path, off_t *pos)
```

Defined at lines 236 to 265.

Find the offset where a new record can go at the end of a credential file.

The file has to be empty or end with a newline, so an incomplete last record fails.

Parameters:
* `fp`: open file
* `path`: file path for the messages
* `pos`: receives the end offset

Returns: 0, or -1 after a message

Called by: [update_passwd](afppasswd.c.md#update_passwd)

### open_credential_for_replacement

```c
static int open_credential_for_replacement(const char *path)
```

Defined at lines 277 to 300.

Open or create a credential file for new content, and leave it empty with mode 0600.

The call does not follow a symbolic link, and an existing file has to pass validate_opened_file before it gets truncated.

Parameters:
* `path`: file path

Returns: the descriptor, or -1 after a message

Calls: [validate_opened_file](afppasswd.c.md#validate_opened_file)

Called by: [create_file](afppasswd.c.md#create_file), [randnum_write_keyfile](afppasswd.c.md#randnum_write_keyfile)

### randnum_make_keypath

```c
static int randnum_make_keypath(const char *path, char *keypath, size_t keypath_size)
```

Defined at lines 310 to 324.

Build the path of the `.key` file next to a RandNum password file.

Parameters:
* `path`: RandNum password file
* `keypath`: receives the path with `.key` appended
* `keypath_size`: size of keypath

Returns: 0, or -1 after a message for a path that is too long

Calls: [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [randnum_ensure_keyfile](afppasswd.c.md#randnum_ensure_keyfile), [randnum_open_keyfile](afppasswd.c.md#randnum_open_keyfile)

### randnum_read_keyfd

```c
static int randnum_read_keyfd(int keyfd, uint8_t key[DES_KEY_SZ], const char *keypath)
```

Defined at lines 337 to 384.

Read the DES secret of a RandNum `.key` file from its start.

The file has to hold 16 hex digits, with an optional newline after them. The call wipes the text it read.

Parameters:
* `keyfd`: open `.key` file
* `key`: receives the 8-byte DES secret
* `keypath`: path for the messages, or NULL

Returns: 0, or -1 after a message for a read error or a malformed file

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [unhex](afppasswd.c.md#unhex)

Called by: [convert_passwd](afppasswd.c.md#convert_passwd), [randnum_ensure_keyfile](afppasswd.c.md#randnum_ensure_keyfile), [randnum_open_keyfile](afppasswd.c.md#randnum_open_keyfile)

### randnum_open_keyfile

```c
static int randnum_open_keyfile(const char *path, int *keyfd_out)
```

Defined at lines 395 to 421.

Open the `.key` file of a RandNum password file and check that it holds a valid DES secret.

Parameters:
* `path`: RandNum password file
* `keyfd_out`: receives the open `.key` file

Returns: 0, or -1 after a message when the file is missing, not private, or malformed

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [open_credential_file](afppasswd.c.md#open_credential_file), [randnum_make_keypath](afppasswd.c.md#randnum_make_keypath), [randnum_read_keyfd](afppasswd.c.md#randnum_read_keyfd)

Called by: [update_passwd](afppasswd.c.md#update_passwd)

### randnum_write_keyfile

```c
static int randnum_write_keyfile(const char *keypath)
```

Defined at lines 432 to 466.

Write a new random DES secret to a RandNum `.key` file.

The secret comes from GCRY_STRONG_RANDOM and goes out as 16 hex digits and a newline, in a file that open_credential_for_replacement empties first.

Parameters:
* `keypath`: path of the `.key` file

Returns: 0, or -1 after a message

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [open_credential_for_replacement](afppasswd.c.md#open_credential_for_replacement)

Called by: [randnum_ensure_keyfile](afppasswd.c.md#randnum_ensure_keyfile)

Uses file-scope variables: `hextable`

### randnum_ensure_keyfile

```c
static int randnum_ensure_keyfile(const char *path, int flags)
```

Defined at lines 478 to 522.

Make sure that a RandNum password file has a valid `.key` file.

A missing `.key` file gets a new secret. An existing file has to be private, and an invalid secret gets replaced only with -f.

Parameters:
* `path`: RandNum password file
* `flags`: option flags, OPT_FORCE to replace an invalid secret

Returns: 0, or -1 after a message

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [randnum_make_keypath](afppasswd.c.md#randnum_make_keypath), [randnum_read_keyfd](afppasswd.c.md#randnum_read_keyfd), [randnum_write_keyfile](afppasswd.c.md#randnum_write_keyfile), [validate_opened_file](afppasswd.c.md#validate_opened_file)

Called by: [main](afppasswd.c.md#main)

### convert_passwd

```c
static int convert_passwd(char *passwd_buf, char *newpwd, const int keyfd)
```

Defined at lines 538 to 605.

Encrypt a new RandNum password into hex, or decrypt a stored hex password, with the DES secret.

Without newpwd, the call turns the 16 hex digits of passwd_buf into 8 bytes and decrypts them in place. With newpwd, it encrypts the 8 bytes of newpwd in place and writes their hex form to passwd_buf. The secret comes from the `.key` file in DES ECB mode.

Parameters:
* `passwd_buf`: hex password field of at least 16 bytes
* `newpwd`: new 8-byte password, encrypted on return, or NULL
* `keyfd`: open `.key` file

Returns: 0, or -1 when the secret or a cipher call fails

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [randnum_read_keyfd](afppasswd.c.md#randnum_read_keyfd), [unhex](afppasswd.c.md#unhex)

Called by: [update_passwd](afppasswd.c.md#update_passwd)

Uses file-scope variables: `hextable`

### srp_compute_verifier

```c
static int srp_compute_verifier(const char *username, const char *password, const unsigned char *salt, unsigned char *v_out)
```

Defined at lines 622 to 687.

Compute the SRP verifier of a user name and password with a salt.

The call computes `x = SHA1(salt | SHA1(username | ":" | password))` and `v = g^x mod N`, with the group of [srp.h](../../include/atalk/srp.h.md). The verifier goes out as SRP_NBYTES big-endian bytes with leading zeros.

Parameters:
* `username`: user name of up to SRP_USERNAME_MAX_LEN bytes
* `password`: password of up to 255 bytes
* `salt`: SRP_SALT_LEN bytes of salt
* `v_out`: receives the SRP_NBYTES verifier

Returns: 0, or -1 for a name or password that is too long, or a failed hash

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

Uses file-scope variables: `srp_N_bytes` in [include/atalk/srp.h](../../include/atalk/srp.h.md), `srp_g_byte` in [include/atalk/srp.h](../../include/atalk/srp.h.md)

### srp_encode_hex

```c
static void srp_encode_hex(char *out_hex, const unsigned char *salt, const unsigned char *verifier)
```

Defined at lines 700 to 716.

Write the salt and the verifier as upper-case hex, separated by a colon.

The call writes no NUL, so out_hex needs SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN bytes.

Parameters:
* `out_hex`: receives the text
* `salt`: SRP_SALT_LEN bytes of salt
* `verifier`: SRP_NBYTES verifier

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

Uses file-scope variables: `hextable`

Mentioned in the documentation of: [srp_write_record](afppasswd.c.md#srp_write_record)

### open_srp_verifier_directory

```c
static int open_srp_verifier_directory(const char *path)
```

Defined at lines 729 to 769.

Open the SRP verifier directory and check its owner and mode.

The directory has to be owned by root, or by the calling user when that user is not root and runs a private server. It must not be writable by group or other. A regular file at the path is a legacy flat verifier file, and the message points to afppasswd -m.

Parameters:
* `path`: directory path

Returns: the directory descriptor, or -1 after a message

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [create_srp_directory](afppasswd.c.md#create_srp_directory), [disable_srp_verifier](afppasswd.c.md#disable_srp_verifier), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

### srp_uid_filename

```c
static int srp_uid_filename(uid_t uid, char *name, size_t size)
```

Defined at lines 779 to 783.

Write the decimal UID that names the verifier file of a user.

Parameters:
* `uid`: user ID
* `name`: receives the file name
* `size`: size of name

Returns: 0, or -1 when the name does not fit

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [open_srp_verifier](afppasswd.c.md#open_srp_verifier)

### validate_srp_verifier_file

```c
static int validate_srp_verifier_file(int fd, uid_t uid, const char *path, int administrative)
```

Defined at lines 799 to 816.

Check the type, owner, link count, and mode of an open verifier file.

The file has to be a regular file with one link, owned by uid. An administrative caller also accepts root as the owner and skips the mode check, since it sets the mode afterwards. Any other caller needs a mode that srp_verifier_mode_is_safe accepts.

Parameters:
* `fd`: open verifier file
* `uid`: expected owner
* `path`: verifier directory for the message
* `administrative`: nonzero for root's add, disable, or initialization

Returns: 0, or -1 after a message

Calls: [srp_verifier_mode_is_safe](../../include/atalk/srp.h.md#srp_verifier_mode_is_safe)

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [open_srp_verifier](afppasswd.c.md#open_srp_verifier)

### open_srp_verifier

```c
static int open_srp_verifier(int dirfd, const char *path, uid_t uid, int create)
```

Defined at lines 833 to 884.

Open the verifier file of a user in the verifier directory, or create it for an administrative caller.

The file name is the decimal UID, and the call does not follow a symbolic link. With create, a new file starts root-owned, and every file gets mode 0600 after the checks. Without create, a file that the user cannot open is a root-owned placeholder, and the message sends the user to the administrator.

Parameters:
* `dirfd`: open verifier directory
* `path`: verifier directory for the messages
* `uid`: user ID
* `create`: nonzero for root's add, disable, or initialization

Returns: the descriptor, or -1 after a message

Calls: [srp_uid_filename](afppasswd.c.md#srp_uid_filename), [validate_srp_verifier_file](afppasswd.c.md#validate_srp_verifier_file)

Called by: [create_srp_directory](afppasswd.c.md#create_srp_directory), [disable_srp_verifier](afppasswd.c.md#disable_srp_verifier), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

### collect_new_password

```c
static int collect_new_password(char *password, size_t password_size, const char *pass, int flags)
```

Defined at lines 899 to 969.

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

Defined at lines 987 to 1040.

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

Defined at lines 1061 to 1261.

Set the SRP password of a user, through root's -a or through the user.

The verifier file holds one record for the user. Root's -a also accepts an empty file, or the record of an earlier user of the UID. A user has to give the old password, which the call checks against the stored verifier in constant time. A disabled record stops a user.

The new password goes through collect_new_password and gets a fresh random salt. Then srp_write_record replaces the record, and root's write hands the file to the user, which enrolls the account.

Parameters:
* `path`: verifier directory
* `name`: user name
* `uid`: UID of the user
* `flags`: option flags: OPT_ISROOT, OPT_ADDUSER, OPT_NOCRACK
* `pass`: -w value, or "" to prompt

Returns: 0, or -1 after a message

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [collect_new_password](afppasswd.c.md#collect_new_password), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [open_srp_verifier](afppasswd.c.md#open_srp_verifier), [open_srp_verifier_directory](afppasswd.c.md#open_srp_verifier_directory), [srp_compute_verifier](afppasswd.c.md#srp_compute_verifier), [srp_encode_hex](afppasswd.c.md#srp_encode_hex), [srp_valid_fields](../../include/atalk/srp.h.md#srp_valid_fields), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [srp_write_record](afppasswd.c.md#srp_write_record), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [unhex](afppasswd.c.md#unhex)

Called by: [main](afppasswd.c.md#main)

### disable_srp_verifier

```c
static int disable_srp_verifier(const char *path, const char *name, uid_t uid)
```

Defined at lines 1276 to 1357.

Disable the SRP account of a user with a root-owned placeholder record.

The call creates the verifier file when needed and takes a write lock without waiting. It revokes the enrollment first: the file becomes root-owned with mode 0600 before the placeholder replaces the record. The file and the directory reach the disk before the call returns.

Parameters:
* `path`: verifier directory
* `name`: user name
* `uid`: UID of the user

Returns: 0, or -1 after a message

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [open_srp_verifier](afppasswd.c.md#open_srp_verifier), [open_srp_verifier_directory](afppasswd.c.md#open_srp_verifier_directory), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [main](afppasswd.c.md#main)

### create_srp_directory

```c
static int create_srp_directory(const char *path, uid_t minuid)
```

Defined at lines 1371 to 1449.

Create the SRP verifier directory with a disabled placeholder record for each account.

The directory gets mode 0755 and has to be new. Every account from minuid up with a valid user name gets a root-owned placeholder file named after its UID, which revokes any enrollment. The first failed file ends the run.

Parameters:
* `path`: directory path
* `minuid`: lowest UID that gets a placeholder

Returns: 0, or -1 after a message

Calls: [open_srp_verifier](afppasswd.c.md#open_srp_verifier), [open_srp_verifier_directory](afppasswd.c.md#open_srp_verifier_directory), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [main](afppasswd.c.md#main)

Uses file-scope variables: `buf`

### private_store_is_callers

```c
static int private_store_is_callers(const struct stat *st, uid_t uid)
```

Defined at lines 1459 to 1463.

Tell whether a stat result is a directory of the caller without group or other permissions.

Parameters:
* `st`: stat result
* `uid`: UID of the caller

Returns: nonzero for such a directory, or 0

Called by: [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier)

### create_private_srp_verifier

```c
static int create_private_srp_verifier(const char *path, uid_t uid, int flags, const char *pass)
```

Defined at lines 1480 to 1656.

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

Defined at lines 1667 to 1686.

Check that a RandNum record field holds only hex digits or only `*`

Parameters:
* `field`: field text
* `len`: field length

Returns: 1 for a valid field, or 0

Called by: [valid_randnum_record](afppasswd.c.md#valid_randnum_record)

### valid_randnum_record

```c
static int valid_randnum_record(const char *fields)
```

Defined at lines 1697 to 1706.

Check the fields of a RandNum record after the user name.

The record needs two fields of 16 characters and one of 8, separated by colons and ended by a newline. Each field holds hex digits or `*` only.

Parameters:
* `fields`: record text after the user name and its colon

Returns: nonzero for a valid record, or 0

Calls: [valid_hex_or_disabled](afppasswd.c.md#valid_hex_or_disabled)

Called by: [update_passwd](afppasswd.c.md#update_passwd)

### update_passwd

```c
static int update_passwd(const char *path, const char *name, int flags, const char *pass)
```

Defined at lines 1727 to 1956.

Set the RandNum password of a user in the RandNum password file, for root.

The file has to be private, and its `.key` file has to hold a valid DES secret. The record of the user has to exist, except with -a, which appends a new record to a file that ends with a newline. The new password comes from -w or from two prompts and has up to 8 characters. Unless -n turns it off, cracklib checks the password.

The call encrypts the password with convert_passwd, as the RandNum UAM expects, and writes the record under a write lock.

Parameters:
* `path`: RandNum password file
* `name`: user name
* `flags`: option flags: OPT_ISROOT, OPT_ADDUSER, OPT_NOCRACK
* `pass`: -w value, or "" to prompt

Returns: 0, or -1 after a message

Calls: [convert_passwd](afppasswd.c.md#convert_passwd), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [find_append_position](afppasswd.c.md#find_append_position), [open_credential_file](afppasswd.c.md#open_credential_file), [randnum_open_keyfile](afppasswd.c.md#randnum_open_keyfile), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [valid_randnum_record](afppasswd.c.md#valid_randnum_record)

Called by: [main](afppasswd.c.md#main)

Uses file-scope variables: `buf`

### create_file

```c
static int create_file(const char *path, uid_t minuid)
```

Defined at lines 1970 to 2009.

Create the RandNum password file with a disabled record for each account.

The call empties the file and sets mode 0600. Every account from minuid up with a valid user name gets a record of `*` fields.

Parameters:
* `path`: RandNum password file
* `minuid`: lowest UID that gets a record

Returns: 0, or -1 after a message

Calls: [open_credential_for_replacement](afppasswd.c.md#open_credential_for_replacement), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [main](afppasswd.c.md#main)

Uses file-scope variables: `buf`

### print_usage

```c
static void print_usage(void)
```

Defined at lines 2015 to 2043.

Print the usage of afppasswd for root and for users to stderr.

Called by: [main](afppasswd.c.md#main)

### main

```c
int main(int argc, char **argv)
```

Defined at lines 2059 to 2315.

Manage the SRP verifiers and RandNum passwords of afpd.

A user can change the SRP password of the calling account, or create the private verifier directory of a single-user server with -c -p. Root can add or reset a user with -a and disable one with -d. Root can also create a credential store with -c and migrate a flat verifier file with -m. The -r option switches to RandNum. An inherited setuid-root installation drops its privileges before any option parses.

Parameters:
* `argc`: argument count
* `argv`: argument vector

Returns: 0, or -1 after a message

Calls: [afppasswd_migrate_srp](afppasswd_migrate.c.md#afppasswd_migrate_srp), [create_file](afppasswd.c.md#create_file), [create_private_srp_verifier](afppasswd.c.md#create_private_srp_verifier), [create_srp_directory](afppasswd.c.md#create_srp_directory), [disable_srp_verifier](afppasswd.c.md#disable_srp_verifier), [initialize_libgcrypt](afppasswd.c.md#initialize_libgcrypt), [parse_minimum_uid](afppasswd.c.md#parse_minimum_uid), [print_usage](afppasswd.c.md#print_usage), [randnum_ensure_keyfile](afppasswd.c.md#randnum_ensure_keyfile), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [update_passwd](afppasswd.c.md#update_passwd), [update_srp_passwd](afppasswd.c.md#update_srp_passwd)

# Macros

* Undocumented: `AFPPASSWD_OPTSTRING`, `DES_KEY_SZ`, `FORMAT`, `FORMAT_LEN`, `HEXPASSWDLEN`, `OPT_ADDUSER`, `OPT_CREATE`, `OPT_DISABLE`, `OPT_FORCE`, `OPT_ISROOT`, `OPT_MIGRATE`, `OPT_NOCRACK`, `OPT_RANDNUM`, `PASSWDLEN`, `PASSWD_ILLEGAL`, `SRP_PASSWDLEN`, `UID_START`

# File-scope variables

`buf`, `hextable`
