---
type: C Source File
title: "etc/uams/uams_srp.c"
description: "14 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_srp.c"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/uams](../uams.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/constant_time.h](../../include/atalk/constant_time.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/srp.h](../../include/atalk/srp.h.md)
* [atalk/uam.h](../../include/atalk/uam.h.md)
* System headers: `arpa/inet.h`, `ctype.h`, `errno.h`, `fcntl.h`, `gcrypt.h`, `inttypes.h`, `pwd.h`, `stdarg.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Function tables

### uams_srp

Initialized at line 1018 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_srp.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_srp.c.md#uam_setup)

# Functions

### strip_leading_zeros

```c
static const unsigned char * strip_leading_zeros(const unsigned char *buf, size_t len, size_t *out_len)
```

Defined at lines 78 to 88.

Strip leading zero bytes from a big-endian integer buffer.

Returns: pointer to the first non-zero byte (or last byte if all zeros).

Called by: [srp_logincont](uams_srp.c.md#srp_logincont)

### mgf1_sha1

```c
static int mgf1_sha1(const unsigned char *seed, size_t seed_len, unsigned char *out, size_t out_len)
```

Defined at lines 98 to 133.

MGF1 mask generation function (PKCS#1 v2.1) with SHA-1.

Parameters:
* `seed`: The seed for the mask generation.
* `seed_len`: The length of the seed.
* `out`: The output buffer.
* `out_len`: The length of the output buffer.

Returns: 0 on success, -1 on failure.

Called by: [srp_logincont](uams_srp.c.md#srp_logincont)

### sha1_multi

```c
static int sha1_multi(unsigned char *out,...)
```

Defined at lines 145 to 172.

Compute SHA-1 incrementally from multiple buffers.

Arguments after `out` are pairs of (const unsigned char *data, size_t len), terminated by a NULL data pointer.

Parameters:
* `out`: 20-byte buffer for the SHA-1 digest.

Returns: 0 on success, -1 on failure.

Called by: [srp_logincont](uams_srp.c.md#srp_logincont), [srp_setup](uams_srp.c.md#srp_setup)

### mpi_to_padded_buf

```c
static void mpi_to_padded_buf(unsigned char *buf, size_t nbytes, gcry_mpi_t m)
```

Defined at lines 181 to 191.

Write an MPI to a buffer as a big-endian integer, zero-padded.

Parameters:
* `buf`: Output buffer (must be at least `nbytes`).
* `nbytes`: Target length with leading zero padding.
* `m`: MPI to serialize.

Called by: [srp_logincont](uams_srp.c.md#srp_logincont), [srp_setup](uams_srp.c.md#srp_setup)

### write_uint16_be

```c
static void write_uint16_be(unsigned char **p, uint16_t val)
```

Defined at lines 199 to 204.

Write a 2-byte big-endian unsigned integer and advance the pointer.

Parameters:
* `p`: Pointer to the write position (advanced by 2).
* `val`: Value to write.

Called by: [srp_logincont](uams_srp.c.md#srp_logincont), [srp_setup](uams_srp.c.md#srp_setup)

### read_uint16_be

```c
static uint16_t read_uint16_be(unsigned char **p)
```

Defined at lines 212 to 217.

Read a 2-byte big-endian unsigned integer and advance the pointer.

Parameters:
* `p`: Pointer to the read position (advanced by 2).

Returns: The decoded 16-bit value.

Called by: [srp_logincont](uams_srp.c.md#srp_logincont)

### srp_lookup_verifier

```c
static enum srp_verifier_status srp_lookup_verifier(const char *path, const char *username, uid_t uid, unsigned char *salt_out, gcry_mpi_t *v_out)
```

Defined at lines 250 to 436.

Look up a user's salt and verifier from their SRP verifier file.

The verifier directory is administrator-owned, or owned by the serving user when a single-user afpd runs as that user. An enrolled user's numeric-UID file is owned and writable only by that user and contains exactly one record: username:hex_salt:hex_verifier. A root-owned, disabled placeholder denotes a non-enrolled user. It is distinguished from a root-owned active verifier, which remains unsafe.

Parameters:
* `path`: Path to the verifier directory.
* `username`: User to look up.
* `uid`: Numeric uid used as the verifier filename.
* `salt_out`: Buffer for the salt (SRP_SALT_LEN bytes).
* `v_out`: MPI set to the verifier on success.

Returns: a status identifying an active, missing, disabled, or unsafe verifier.

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [srp_valid_fields](../../include/atalk/srp.h.md#srp_valid_fields), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [srp_verifier_mode_is_safe](../../include/atalk/srp.h.md#srp_verifier_mode_is_safe), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [srp_setup](uams_srp.c.md#srp_setup)

### srp_session_free

```c
static void srp_session_free(void)
```

Defined at lines 441 to 457.

Release all per-session SRP state and zero sensitive buffers.

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [srp_login](uams_srp.c.md#srp_login), [srp_login_ext](uams_srp.c.md#srp_login_ext), [srp_logincont](uams_srp.c.md#srp_logincont), [srp_setup](uams_srp.c.md#srp_setup)

Uses file-scope variables: `session_B`, `session_B_buf`, `session_N`, `session_b`, `session_g`, `session_salt`, `session_username`, `session_v`, `srppwd`

### srp_setup

```c
static int srp_setup(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 468 to 625.

SRP Round 1 setup.

Looks up the user's verifier from the verifier directory, generates the server ephemeral key pair (b, B), and builds the FPLoginExt response containing the SRP group parameters, salt, and server public ephemeral B.

Returns: AFPERR_AUTHCONT on success, AFPERR_NOTAUTH if user not found.

Calls: [mpi_to_padded_buf](uams_srp.c.md#mpi_to_padded_buf), [sha1_multi](uams_srp.c.md#sha1_multi), [srp_lookup_verifier](uams_srp.c.md#srp_lookup_verifier), [srp_session_free](uams_srp.c.md#srp_session_free), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [write_uint16_be](uams_srp.c.md#write_uint16_be)

Called by: [srp_login](uams_srp.c.md#srp_login), [srp_login_ext](uams_srp.c.md#srp_login_ext)

Uses file-scope variables: `session_B`, `session_B_buf`, `session_N`, `session_b`, `session_g`, `session_salt`, `session_username`, `session_v`, `srp_N_bytes` in [include/atalk/srp.h](../../include/atalk/srp.h.md), `srp_g_byte` in [include/atalk/srp.h](../../include/atalk/srp.h.md), `srppwd`

Mentioned in the documentation of: [srp_login](uams_srp.c.md#srp_login), [srp_login_ext](uams_srp.c.md#srp_login_ext)

### srp_login

```c
static int srp_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 633 to 679.

FPLogin handler for SRP UAM.

Parses the username from the non-extended login request, validates the user, and delegates to [srp_setup()](uams_srp.c.md#srp_setup) for Round 1.

Calls: [srp_session_free](uams_srp.c.md#srp_session_free), [srp_setup](uams_srp.c.md#srp_setup), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [uam_setup](uams_srp.c.md#uam_setup)

Uses file-scope variables: `session_username`, `srppwd`

### srp_login_ext

```c
static int srp_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 687 to 730.

FPLoginExt handler for SRP UAM.

Parses the UTF-8 username from the extended login request, validates the user, and delegates to [srp_setup()](uams_srp.c.md#srp_setup) for Round 1.

Calls: [srp_session_free](uams_srp.c.md#srp_session_free), [srp_setup](uams_srp.c.md#srp_setup), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [uam_setup](uams_srp.c.md#uam_setup)

Uses file-scope variables: `session_username`, `srppwd`

### srp_logincont

```c
static int srp_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 741 to 992.

SRP Round 2: FPLoginCont handler.

Parses the client's public ephemeral A and proof M1, computes the server-side shared secret S and session key K, verifies M1, and responds with server proof M2 to complete mutual authentication.

Returns: AFP_OK on success, AFPERR_NOTAUTH on proof mismatch.

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [mgf1_sha1](uams_srp.c.md#mgf1_sha1), [mpi_to_padded_buf](uams_srp.c.md#mpi_to_padded_buf), [read_uint16_be](uams_srp.c.md#read_uint16_be), [sha1_multi](uams_srp.c.md#sha1_multi), [srp_session_free](uams_srp.c.md#srp_session_free), [strip_leading_zeros](uams_srp.c.md#strip_leading_zeros), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [write_uint16_be](uams_srp.c.md#write_uint16_be)

Called by: [uam_setup](uams_srp.c.md#uam_setup)

Uses file-scope variables: `session_B_buf`, `session_N`, `session_b`, `session_salt`, `session_username`, `session_v`, `srp_N_bytes` in [include/atalk/srp.h](../../include/atalk/srp.h.md), `srp_g_byte` in [include/atalk/srp.h](../../include/atalk/srp.h.md), `srppwd`

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 996 to 1011.

Calls: [srp_login](uams_srp.c.md#srp_login), [srp_login_ext](uams_srp.c.md#srp_login_ext), [srp_logincont](uams_srp.c.md#srp_logincont), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_srp](uams_srp.c.md#uams_srp)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 1013 to 1016.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_srp](uams_srp.c.md#uams_srp)

# Typedefs and enums

* `enum srp_verifier_status`: `SRP_VERIFIER_OK`, `SRP_VERIFIER_MISSING`, `SRP_VERIFIER_NOT_ENROLLED`, `SRP_VERIFIER_DISABLED`, `SRP_VERIFIER_UNSAFE`, `SRP_VERIFIER_INVALID`

# Macros

* Undocumented: `SRP_AUTH_FAILURE`, `SRP_CLIENT_PROOF`, `SRP_INIT_MARKER`, `SRP_SERVER_PROOF`, `SRP_SESSION_KEY_LEN`, `unhex`

# File-scope variables

`session_B`, `session_B_buf`, `session_N`, `session_b`, `session_g`, `session_salt`, `session_username`, `session_v`, `srppwd`
