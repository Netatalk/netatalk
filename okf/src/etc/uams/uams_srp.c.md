---
type: C Source File
title: "etc/uams/uams_srp.c"
description: "SRP (Secure Remote Password) User Authentication Method for AFP."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_srp.c"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Initialized at line 1072 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_srp.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_srp.c.md#uam_setup)

# Functions

### strip_leading_zeros

```c
static const unsigned char * strip_leading_zeros(const unsigned char *buf, size_t len, size_t *out_len)
```

Defined at lines 84 to 94.

Skip the leading zero bytes of a big-endian integer.

The last byte stays, so a zero value keeps 1 byte.

Parameters:
* `buf`: integer bytes
* `len`: length of buf
* `out_len`: receives the length without the leading zeros

Returns: the first byte that is not 0, or the last byte of an all-zero buffer

Called by: [srp_logincont](uams_srp.c.md#srp_logincont)

### mgf1_sha1

```c
static int mgf1_sha1(const unsigned char *seed, size_t seed_len, unsigned char *out, size_t out_len)
```

Defined at lines 104 to 139.

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

Defined at lines 151 to 178.

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

Defined at lines 187 to 197.

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

Defined at lines 205 to 210.

Write a 2-byte big-endian unsigned integer and advance the pointer.

Parameters:
* `p`: Pointer to the write position (advanced by 2).
* `val`: Value to write.

Called by: [srp_logincont](uams_srp.c.md#srp_logincont), [srp_setup](uams_srp.c.md#srp_setup)

### read_uint16_be

```c
static uint16_t read_uint16_be(unsigned char **p)
```

Defined at lines 218 to 223.

Read a 2-byte big-endian unsigned integer and advance the pointer.

Parameters:
* `p`: Pointer to the read position (advanced by 2).

Returns: The decoded 16-bit value.

Called by: [srp_logincont](uams_srp.c.md#srp_logincont)

### srp_lookup_verifier

```c
static enum srp_verifier_status srp_lookup_verifier(const char *path, const char *username, uid_t uid, unsigned char *salt_out, gcry_mpi_t *v_out)
```

Defined at lines 256 to 442.

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

Defined at lines 447 to 463.

Release all per-session SRP state and zero sensitive buffers.

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [srp_login](uams_srp.c.md#srp_login), [srp_login_ext](uams_srp.c.md#srp_login_ext), [srp_logincont](uams_srp.c.md#srp_logincont), [srp_setup](uams_srp.c.md#srp_setup)

Uses file-scope variables: `session_B`, `session_B_buf`, `session_N`, `session_b`, `session_g`, `session_salt`, `session_username`, `session_v`, `srppwd`

### srp_setup

```c
static int srp_setup(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 483 to 640.

Answer round 1 of an SRP login with the group, the salt, and the public ephemeral B of the server.

The call reads the verifier and the salt of the user from the verifier directory. It then picks a random b until B = k*v + g^b mod N is not 0. The reply of 413 bytes holds the context, the group index, N, g, the salt, and B. Like an Apple server, the call answers a user without a usable verifier with AFPERR_NOTAUTH and the 2-byte init marker.

Parameters:
* `obj`: session object
* `ibuf`: UAM data of the request, unused
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_NOTAUTH without a usable verifier, or AFPERR_MISC without a verifier path or on a failure

Calls: [mpi_to_padded_buf](uams_srp.c.md#mpi_to_padded_buf), [sha1_multi](uams_srp.c.md#sha1_multi), [srp_lookup_verifier](uams_srp.c.md#srp_lookup_verifier), [srp_session_free](uams_srp.c.md#srp_session_free), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [write_uint16_be](uams_srp.c.md#write_uint16_be)

Called by: [srp_login](uams_srp.c.md#srp_login), [srp_login_ext](uams_srp.c.md#srp_login_ext)

Uses file-scope variables: `session_B`, `session_B_buf`, `session_N`, `session_b`, `session_g`, `session_salt`, `session_username`, `session_v`, `srp_N_bytes` in [include/atalk/srp.h](../../include/atalk/srp.h.md), `srp_g_byte` in [include/atalk/srp.h](../../include/atalk/srp.h.md), `srppwd`

### srp_login

```c
static int srp_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 657 to 703.

Read the user name of an FPLogin request and start an SRP login.

The request holds the name as a Pascal string, padded to an even address. The user has to pass uam_checkuser before srp_setup runs.

Parameters:
* `obj`: session object
* `uam_pwd`: unused until the continuation
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the result of srp_setup, AFPERR_NOTAUTH for an unknown or refused user, or AFPERR_PARAM

Calls: [srp_session_free](uams_srp.c.md#srp_session_free), [srp_setup](uams_srp.c.md#srp_setup), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [uam_setup](uams_srp.c.md#uam_setup)

Uses file-scope variables: `session_username`, `srppwd`

### srp_login_ext

```c
static int srp_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 721 to 764.

Read the user name of an FPLoginExt request and start an SRP login.

The name comes as an AFP name of type 3, a UTF-8 string with a 2-byte length. The user has to pass uam_checkuser before srp_setup runs.

Parameters:
* `obj`: session object
* `uname`: user name field of the request
* `uam_pwd`: unused until the continuation
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the result of srp_setup, AFPERR_NOTAUTH for an unknown or refused user, or AFPERR_PARAM

Calls: [srp_session_free](uams_srp.c.md#srp_session_free), [srp_setup](uams_srp.c.md#srp_setup), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [uam_setup](uams_srp.c.md#uam_setup)

Uses file-scope variables: `session_username`, `srppwd`

### srp_logincont

```c
static int srp_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 785 to 1036.

Check the proof of round 2 of an SRP login and answer with the proof of the server.

The request holds the AFP ID, the step marker, the public ephemeral A, and the proof M1. The call refuses A mod N = 0 and u = 0. It computes S = (A * v^u)^b mod N and derives the 40-byte session secret K from S with MGF1-SHA1. M1 has to match in constant time, and the reply holds the step marker and M2. Every exit frees the session state.

Parameters:
* `obj`: session object, unused
* `uam_pwd`: receives the user entry after a matching proof
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFP_OK, SRP_AUTH_FAILURE for a wrong proof, AFPERR_NOTAUTH for a malformed request, AFPERR_PARAM without round 1, or AFPERR_MISC

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [mgf1_sha1](uams_srp.c.md#mgf1_sha1), [mpi_to_padded_buf](uams_srp.c.md#mpi_to_padded_buf), [read_uint16_be](uams_srp.c.md#read_uint16_be), [sha1_multi](uams_srp.c.md#sha1_multi), [srp_session_free](uams_srp.c.md#srp_session_free), [strip_leading_zeros](uams_srp.c.md#strip_leading_zeros), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [write_uint16_be](uams_srp.c.md#write_uint16_be)

Called by: [uam_setup](uams_srp.c.md#uam_setup)

Uses file-scope variables: `session_B_buf`, `session_N`, `session_b`, `session_salt`, `session_username`, `session_v`, `srp_N_bytes` in [include/atalk/srp.h](../../include/atalk/srp.h.md), `srp_g_byte` in [include/atalk/srp.h](../../include/atalk/srp.h.md), `srppwd`

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 1047 to 1062.

Initialize libgcrypt and register the SRP login.

Parameters:
* `obj`: AFP object, unused
* `path`: path of the module

Returns: 0, or -1 for a libgcrypt version mismatch or a failed registration

Calls: [srp_login](uams_srp.c.md#srp_login), [srp_login_ext](uams_srp.c.md#srp_login_ext), [srp_logincont](uams_srp.c.md#srp_logincont), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_srp](uams_srp.c.md#uams_srp)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 1067 to 1070.

Unregister the SRP login.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_srp](uams_srp.c.md#uams_srp)

# Typedefs and enums

* `enum srp_verifier_status`: `SRP_VERIFIER_OK`, `SRP_VERIFIER_MISSING`, `SRP_VERIFIER_NOT_ENROLLED`, `SRP_VERIFIER_DISABLED`, `SRP_VERIFIER_UNSAFE`, `SRP_VERIFIER_INVALID`

# Macros

* Undocumented: `SRP_AUTH_FAILURE`, `SRP_CLIENT_PROOF`, `SRP_INIT_MARKER`, `SRP_SERVER_PROOF`, `SRP_SESSION_KEY_LEN`, `unhex`

# File-scope variables

`session_B`, `session_B_buf`, `session_N`, `session_b`, `session_g`, `session_salt`, `session_username`, `session_v`, `srppwd`
