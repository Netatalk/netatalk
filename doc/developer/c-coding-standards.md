# C Coding Standards

Netatalk targets C11. Use C11 features when adding code or modernizing older
code, and preserve the behavior that existing AFP clients, utilities, and
platforms rely on. This guide combines the project's C conventions with
guidance for the SonarQube Cloud rules applied to Netatalk.

## Formatting and naming

### AStyle formatting

Use Artistic Style (AStyle) 3.6.19 or later. Run the formatter from the
repository root with the project's settings.
[`.astylerc`](https://github.com/Netatalk/netatalk/blob/main/.astylerc)

```sh
./contrib/scripts/codefmt.sh -s c
```

The script formats `.h`, `.c`, and `.cc` files recursively. To run AStyle
directly with the same formatting options, use the following command.

```sh
astyle --options=.astylerc --recursive --suffix=none '*.h' '*.c' '*.cc'
```

- Use One True Brace Style (`--style=1tbs`). Put a function's opening brace
  on its own line and attach control-block braces with a preceding space.
  Write `} else {` on one line. Use braces even for a single statement.
  AStyle adds braces to unbraced single-statement conditionals
  (`--add-braces`).
- Indent each level by four spaces (`--indent=spaces=4`). Continuation
  indentation is capped at 60 spaces relative to the previous line
  (`--max-continuation-indent=60`). Keep lines within 80 columns, with an
  exception for trailing comments. The formatter's wrapping check uses
  `--max-code-length=80` and `--max-code-length-mode=ignore-side-comments`.
- Separate unrelated blocks and labels with a single blank line
  (`--break-blocks`). AStyle removes other blank lines inside functions
  (`--delete-empty-lines`). Remove trailing whitespace.
- Align pointer declarators with the name, as in `char *path`
  (`--align-pointer=name`). The `.cc` files formatted by the same command
  also align reference declarators with the name (`--align-reference=name`).
- Put one space after commas and control-flow keywords and around binary
  operators (`--pad-comma`, `--pad-header`, `--pad-oper`). Keep unary operators
  against their operands. Omit padding inside parentheses (`--unpad-paren`)
  and before semicolons (`--pad-semicolon=none`). For example,
  `for (size_t i = 0; i < len; i++)`.

This byte replacement loop shows brace placement, pointer alignment, and
spacing in a wrapped function declaration and loop header.

```c
#include <stddef.h>

static void replace_byte(unsigned char *buf, size_t len, unsigned char from,
                         unsigned char to)
{
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == from) {
            buf[i] = to;
        } else {
            buf[i] = buf[i];
        }
    }
}
```

### Source conventions

- Use `snake_case` for functions and variables and `UPPER_SNAKE_CASE` for
  macro constants. Put one variable declaration or definition on each line.
  Alphabetize lists of symbols and files when their order is flexible. For
  `getopt()`, alphabetize flags first and options that take arguments second,
  as in `"flsA:D:m:r:"`.
- Omit spaces just inside brackets, as in `buf[i]`.
- Use `/* ... */` comments and align the leading `*` in multiline comments.
  Use trailing comments where they clarify a declaration. The project permits
  `//NOSONAR` for analyzer suppressions. To wrap a long string, place adjacent
  string literals on separate lines. Check that commas separate distinct
  string elements in arrays and that a joined literal uses compatible
  prefixes.

Use a named constant for the retry policy and a comment to explain what
the limit counts.

```c
#include <stdbool.h>

#define MAX_ATTEMPTS 3U

static bool can_retry(unsigned int failed_attempts)
{
    /* The limit includes the initial attempt. */
    return failed_attempts < MAX_ATTEMPTS;
}
```

**Related rules**

`c:S817`, `c:S1110`, `c:S2323`, `c:S2681`, `c:S3728`,
`c:S3972`, `c:S3973`.

## Declarations, headers, and portability

- Put system includes first, followed by third-party, libatalk, and local includes.
  Alphabetize inside each group unless a dependency requires a specific order.
  Keep includes before declarations, use conventional `<header.h>` or
  `"header.h"` forms, and give each guarded header a matching `#ifndef` and
  `#define`. Use portable paths and the build's include directories.
- Give functions explicit prototypes. Use `(void)` for a function that takes
  no arguments. Put function declarations at file scope, keep internal
  functions `static` in every declaration, and remove redundant declarations
  and specifiers. Use modern C declaration syntax and `typedef` for type
  aliases. Keep callback signatures compatible with their declared type and
  use `&handler` when taking a function's address.
- Place `_U_` after the name in an intentionally unused declaration, as in
  `int signo _U_` for a signal handler that ignores the signal number. Use it
  for parameters required by a callback signature and for declarations used
  only in some build configurations. The generated `config.h` defines `_U_`
  as `__attribute__((unused))` on compilers that support it, suppressing
  unused-declaration warnings. On other compilers it expands to nothing.
- Use Meson feature checks and generated configuration macros for library
  capabilities. Check the signature and behavior of each platform's API and
  preserve fallback paths. Use the project's compatibility helpers in
  `include/atalk/compat.h` and `libatalk/compat/`.
- Use standard preprocessor directives and `defined(MACRO)` syntax. Keep
  directives outside macro arguments, pass every required argument, and avoid
  undefining or redefining macros. Keep keywords and predefined names out of
  project macro names. If platform code needs a packing pragma, pair its push
  and pop so the alignment setting stays local. Restore the previous warning
  configuration when ending a local pragma-based suppression.
- Prefer standard escape sequences and plain source spellings. Use explicit
  escapes for control characters. Avoid trigraphs, digraphs, multicharacter
  constants, and identifiers reserved by later language revisions. Use
  uppercase `L` in integer suffixes where the type requires it. Put a
  preprocessor line-continuation backslash at the end of its line.
- Choose supported APIs when replacing obsolete POSIX interfaces or deprecated
  functions. When deprecating a project interface, explain the reason and name
  its replacement if one is available.

Group the system headers ahead of libatalk headers. Use the compatibility
helper to read a file's modification time across supported platforms.

```c
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>

#include <atalk/compat.h>

static bool mtime_is_after(const struct stat *st, struct timespec cutoff)
{
    const struct timespec mtime = atalk_stat_mtime_timespec(st);
    return mtime.tv_sec > cutoff.tv_sec
           || (mtime.tv_sec == cutoff.tv_sec && mtime.tv_nsec > cutoff.tv_nsec);
}
```

**Related rules**

`c:S796`, `c:S797`, `c:S798`, `c:S818`, `c:S819`,
`c:S820`, `c:S824`, `c:S833`, `c:S936`, `c:S954`, `c:S955`, `c:S956`,
`c:S959`, `c:S961`, `c:S969`, `c:S977`, `c:S1123`, `c:S1133`,
`c:S1198`, `c:S1760`, `c:S1761`, `c:S1762`, `c:S1874`, `c:S1911`,
`c:S1916`, `c:S2479`, `c:S2754`, `c:S3231`, `c:S3689`, `c:S3730`,
`c:S3744`, `c:S3805`, `c:S3806`, `c:S4263`, `c:S5259`, `c:S5266`,
`c:S5297`, `c:S5501`, `c:S5825`, `c:S5978`, `c:S6991`, `c:S6996`.

## Control flow and local state

- Initialize values before their first read. Declare locals in the smallest
  scope that uses them and keep loop counters in the loop when possible. Avoid
  shadowing. Remove dead stores, redundant increments, unused declarations,
  and empty or unreachable code. Check external entry points, callbacks, and
  optional build configurations before removing an apparently unused symbol.
- Make each conditional distinct and reachable. Combine branches that do
  identical work. Use `while` for condition-driven loops, integer counters
  for counted loops, and a clear termination condition for loops and
  recursion. Simplify a loop whose body can run at most once.
- Cover the valid cases in every `switch` and always include a `default`
  fallback, placed first or last. Keep case labels at switch scope and write
  single-value cases as individual labels. Put other labels outside the switch.
  Use forward jumps to local cleanup labels where they simplify resource
  release. Keep jumps out of nested blocks.
- Give each statement a clear effect. Keep assignments and increments out of
  expressions where they obscure evaluation. Evaluate side effects before a
  call when their order matters. Use Boolean values directly with Boolean
  operators and assign `true` explicitly. Use logical and bitwise operators
  according to the operand types and intent.
- Track work described by TODO or FIXME comments. Give intentionally empty
  callbacks a comment explaining why an empty body is correct.

Handle each declared status explicitly and place the fallback last.

```c
enum stream_status {
    STREAM_AT_END,
    STREAM_FAILED,
    STREAM_READY
};

static const char *stream_status_name(enum stream_status status)
{
    switch (status) {
    case STREAM_AT_END:
        return "end of stream";

    case STREAM_FAILED:
        return "stream error";

    case STREAM_READY:
        return "ready";

    default:
        return "unknown stream status";
    }
}
```

**Related rules**

`c:S108`, `c:S836`, `c:S872`, `c:S878`, `c:S897`, `c:S905`, `c:S916`,
`c:S999`, `c:S1036`, `c:S1065`, `c:S1116`, `c:S1117`,
`c:S1134`, `c:S1135`, `c:S1144`, `c:S1172`, `c:S1186`, `c:S1219`,
`c:S1264`, `c:S1481`, `c:S1656`, `c:S1751`, `c:S1763`, `c:S1764`,
`c:S1854`, `c:S1862`, `c:S1871`, `c:S1909`, `c:S2123`, `c:S2190`,
`c:S2193`, `c:S2583`, `c:S2668`, `c:S2757`, `c:S2761`, `c:S3562`,
`c:S3923`, `c:S3935`, `c:S3936`, `c:S4524`, `c:S5261`, `c:S5263`,
`c:S5381`, `c:S5570`, `c:S5955`.

## Types, arithmetic, and wire data

- Use types that reflect the API and the data. Use `size_t` for object sizes,
  the API's signed result type for I/O that can report `-1`, and fixed-width
  integers for fields whose width is defined by a protocol or file format.
  Check the result before converting its type. Range-check narrowing and
  signed-to-unsigned conversions and preserve pointer qualifiers.
- Refactor to use matching types and avoid implicit conversions. Use an
  explicit cast whenever a type conversion is necessary.
- Never cast away `const`. If the data must be modified, declare it without
  `const` at its origin. If the API only reads the data, refactor it to accept
  a pointer to `const` data.
- Check arithmetic before evaluating it. Check sums and products against the
  type's limits before using them as sizes or offsets. Check divisors and the
  valid range of shift counts. Test a signed value for negativity while its
  type can still represent negative values.
- Keep protocol fields in their documented byte order. Use the existing
  serialization helpers or copy bytes to an aligned object and convert with
  the matching host/network conversion. Validate the available length before
  reading a field. Use explicit serialization for wire data. Structure
  padding, bit-field layout and alignment may vary by compiler and platform.
- Preserve enum types and use distinct values unless a documented alias is
  intended. Use bit-field types wide enough for their values and unsigned
  types for named one-bit fields. Compare structure fields individually when
  padding can affect bytewise equality.
- Use `sizeof` on the actual array or object and pass a length along with a
  decayed array. Avoid pointer-to-integer conversions and their converse.
  Use array indexing for element access and compare or subtract pointers
  only within the same array, including its one-past position where valid.
  Keep side effects out of operands used only for sizing.
- Express array capacity as a separate parameter. Keep `restrict` and
  `static` array parameter bounds out of new interfaces under this profile.
  Any variable-length array needs a positive bound that fits the available
  stack.

Check the record size and count before computing how many bytes a set of
fixed-size records requires. The caller supplies a writable output pointer.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool record_bytes(size_t count, size_t record_size, size_t *bytes)
{
    if (record_size == 0 || count > SIZE_MAX / record_size) {
        return false;
    }

    *bytes = count * record_size;
    return true;
}
```

**Related rules**

`c:S814`, `c:S841`, `c:S859`, `c:S860`, `c:S1767`,
`c:S1768`, `c:S1831`, `c:S1836`, `c:S1905`, `c:S1913`, `c:S2216`,
`c:S2665`, `c:S2753`, `c:S3135`, `c:S3491`, `c:S3518`, `c:S3729`,
`c:S3949`, `c:S5000`, `c:S5276`, `c:S5279`, `c:S5283`, `c:S5491`,
`c:S5494`, `c:S5658`, `c:S7116`.

See [DSI](https://netatalk.io/developer/md_developer_2dsi)
and [Mac Metadata and Fork Storage](https://netatalk.io/developer/md_developer_2appledouble)
for protocol and on-disk formats.

## Memory, buffers, and string handling

- Document which pointers are borrowed and which transfer ownership. Check
  allocation results before use. Release owned allocations once, including
  on failure, and stop using all aliases after release. Keep addresses of
  local objects within their lifetime. Use `const` for data read through a
  pointer when it is never modified. Check nullable inputs before any
  dereference, including when a macro uses the referenced object.
- When growing a buffer with `realloc()`, calculate a valid nonzero size
  first and store the result separately until success. Preserve the original
  allocation on failure. Pass the real buffer capacity to memory operations
  and check each source and destination range before copying. Remove writes
  to array elements that are overwritten before any read.
- Validate client-supplied offsets, counts, and indices before pointer
  arithmetic, reads, copies, or allocations. For an offset and length, check
  that the offset is within the buffer, then compare the requested length
  with the remaining bytes. Validate lengths and encoding before treating a
  protocol field as a C string.
- Use bounded string operations such as `strnlen()`, `strlcpy()`, and
  `strlcat()` where suitable. Pass the full destination capacity to
  `strlcpy()` and `strlcat()`, including the space reserved for the ending NUL.
  Both require valid NUL-terminated source strings. For `strlcat()`, ensure the
  destination already contains a NUL within its capacity. Check for truncation
  when it would change behavior, and verify how much data is accessible before
  calling `strnlen()`. A result equal to the bound means no NUL was found
  within it. For existing `strncat()` calls, the limit is the number of source
  characters the remaining destination can hold, excluding its final NUL.
- For `snprintf()`, handle a negative return as an error and check whether
  a nonnegative result equals or exceeds the destination capacity. Use
  trusted format literals with matching argument types and order. Pass
  external text as an argument to a format such as `"%s"` once its
  termination is established. If using `scanf()` or `fscanf()` with
  `%s`, set a field width that leaves space for the terminator and check how
  many fields were assigned. Pass individual fields of an aggregate when a
  variadic API needs those values.

Format a validated C string with a literal format. The caller supplies
`capacity` writable bytes and uses the label only when the function
returns `true`.

```c
#include <stdbool.h>
#include <stdio.h>

static bool format_entry_label(char *label, size_t capacity, const char *name)
{
    int length = snprintf(label, capacity, "Entry %s", name);
    return length >= 0 && (size_t)length < capacity;
}
```

**Related rules**

`c:S946`, `c:S1079`, `c:S2259`, `c:S2275`, `c:S2637`,
`c:S3457`, `c:S3519`, `c:S3520`, `c:S3529`, `c:S3584`, `c:S3590`,
`c:S4143`, `c:S5262`, `c:S5270`, `c:S5273`, `c:S5278`, `c:S5281`,
`c:S5293`, `c:S5350`,
`c:S5782`, `c:S6069`, `c:S6655`, `csecurity:S9340`.

## Errors, resource cleanup, and diagnostics

- Check the documented success and error result of each operation that can
  fail, including APIs marked `nodiscard`. Use the result type and argument
  order required by the API. Make every exit path match the function's
  contract, including AFP error codes where applicable. Declare genuinely
  nonreturning functions accordingly and ensure their paths never return.
- Initialize resource handles before any path can reach cleanup. Use a
  sentinel appropriate to each resource, such as `-1` for file descriptors,
  and a shared cleanup path when releasing several resources. Close owned
  descriptors and streams, preserve the original failure as appropriate,
  and keep closed handles out of later operations. Account for partial
  reads or writes where the API permits them.
- Use Netatalk's `LOG()` with an appropriate severity and subsystem in
  daemons. Put actionable context in the message, report `errno` only for
  operations that set it, and keep secrets out of diagnostics. When using
  `<atalk/errchk.h>`, choose a macro that matches the operation's failure
  condition and the caller's error contract.

Once the stream opens, both read paths reach the same close operation.
Publish the version byte only after the read and close succeed.

```c
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <atalk/logger.h>

static int read_version_byte(const char *path, unsigned char *version)
{
    FILE *stream = fopen(path, "rb");
    int byte;
    int ret = -1;

    if (stream == NULL) {
        LOG(log_error, logtype_default, "Open failed: %s", strerror(errno));
        return -1;
    }

    byte = fgetc(stream);

    if (byte == EOF) {
        LOG(log_error, logtype_default, "Unable to read version byte");
        goto cleanup;
    }

    ret = 0;
cleanup:

    if (fclose(stream) == EOF) {
        LOG(log_error, logtype_default, "Close failed: %s", strerror(errno));
        return -1;
    }

    if (ret == 0) {
        *version = (unsigned char)byte;
    }

    return ret;
}
```

**Related rules**

`c:S935`, `c:S2095`, `c:S2234`, `c:S3588`, `c:S3807`,
`c:S5267`, `c:S5271`, `c:S5277`, `c:S5485`, `c:S5488`, `c:S5982`.

See [Error Checking, Logging, and Diagnostics](https://netatalk.io/developer/md_developer_2logging).

## Privileges, files, and external inputs

- Use existing volume path resolution and access checks for client
  filenames. A substring check alone cannot establish that a path stays
  within a volume. Keep checks and operations tied to the same object where
  possible, for example by operating on an open descriptor. Create files
  with appropriately restrictive permissions from the start.
- Preserve authentication, supplementary-group, and effective UID/GID
  transitions. Check calls that change identities, directories, or root
  directories before proceeding. For a `chroot()` path, move the working
  directory inside the new root as part of the transition. Grant Linux
  capabilities only for the operations that require them. Include the account
  check with `pam_acct_mgmt()` in PAM login flows.
- Always build temporary file paths using `tmpdir()` from `<atalk/util.h>`
  as the base directory. Never hard-code `/tmp/...` paths.
- Create temporary files atomically with `mkstemp()` or an appropriate
  equivalent, in a directory with suitable permissions. Use at least six
  trailing `X` characters where the API requires a template and clean up the
  file at the end of its lifetime.
- Keep client data separate from shell commands, LDAP filter syntax, code
  evaluation, and environment variable names or values that affect process
  behavior. Prefer structured APIs and use the destination API's validation or
  escaping for values it must accept. Configure XML parsers to prevent
  untrusted documents from resolving external entities. Keep configurable
  network addresses in configuration.

Create a fixed internal filename relative to a trusted directory descriptor.
`O_EXCL` makes creation fail if the name already exists. The caller checks
for `-1` and closes the descriptor on success.

```c
#include <fcntl.h>

static int create_private_state_file(int dir_fd)
{
    return openat(dir_fd, "state", O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
}
```

**Related rules**

`c:S1081`, `c:S1313`, `c:S2612`, `c:S2755`, `c:S5280`,
`c:S5308`, `c:S5443`, `c:S5802`, `c:S5824`, `c:S5832`, `c:S5847`,
`c:S5849`, `csecurity:S2076`, `csecurity:S2078`, `csecurity:S5334`,
`csecurity:S6547`.

See [Authentication and Session Identity](https://netatalk.io/developer/md_developer_2authentication) and
[Volume Lifecycle](https://netatalk.io/developer/md_developer_2volumes).

## Secrets, cryptography, and transport

- Keep credentials and keys out of source and logs. Use the `explicit_bzero()`
  interface in `<atalk/compat.h>` to clear sensitive buffers whose contents
  must be erased, including on error paths.
- Use `atalk_ct_memcmp()` from `<atalk/constant_time.h>` for constant-time
  equality checks on secrets such as authentication tokens.
- Use a cryptographically secure source for keys, nonces, and other values
  that must be unpredictable. Use supported crypto library primitives and
  choose algorithms, modes, padding, and key sizes appropriate for the
  security requirement.
- Use encrypted transport for new security-sensitive connections. When
  changing legacy AFP interoperability code, preserve the documented
  compatibility behavior and explain any security-sensitive tradeoffs in
  review.

Compare fixed-length tokens using Netatalk's constant-time helper and clear
the disposable candidate afterward. The caller supplies two separate
buffers of `TOKEN_SIZE` bytes and checks the candidate length before calling.

```c
#include <stdbool.h>
#include <stdint.h>

#include <atalk/compat.h>
#include <atalk/constant_time.h>

#define TOKEN_SIZE 32

static bool check_token(uint8_t *candidate, const uint8_t *expected)
{
    bool match = atalk_ct_memcmp(candidate, expected, TOKEN_SIZE) == 0;
    explicit_bzero(candidate, TOKEN_SIZE);
    return match;
}
```

**Related rules**

`c:S2068`, `c:S2245`, `c:S4423`, `c:S4426`, `c:S4790`,
`c:S4830`, `c:S5332`, `c:S5527`, `c:S5542`, `c:S5547`, `c:S5798`,
`c:S6418`.

## Concurrency and performance

- Establish which process or thread owns mutable state. Initialize and
  destroy mutexes at their documented lifecycle boundaries. Pair lock and
  unlock calls on every path and release nested locks in reverse acquisition
  order. Keep blocking I/O outside critical sections when practicable. Bound
  client-influenced waits to protect service availability.
- Use the appropriate synchronization primitive for shared state. Reserve
  `volatile` for cases with defined volatile semantics, such as a
  `volatile sig_atomic_t` flag for communication with a signal handler.
- Measure before optimizing. Describe the workload and results for performance
  changes. See
  [Flamegraph Profiling](https://netatalk.io/developer/md_developer_2profiling).

Take a snapshot under the mutex so callers can format or log it after
unlocking. The caller supplies an initialized mutex and a private output
variable. Every access to the shared counter uses the same mutex.

```c
#include <pthread.h>
#include <stdint.h>

static int snapshot_requests(pthread_mutex_t *mutex, const uint64_t *requests,
                             uint64_t *snapshot)
{
    int ret = pthread_mutex_lock(mutex);

    if (ret != 0) {
        return ret;
    }

    uint64_t count = *requests;
    ret = pthread_mutex_unlock(mutex);

    if (ret == 0) {
        *snapshot = count;
    }

    return ret;
}
```

**Related rules**

`c:S3687`, `c:S5314`, `c:S5486`, `c:S5487`, `c:S5489`,
`c:S6200`, `csecurity:S6390`.

## Doxygen comments

- Start every `.c` and `.h` file with a `/*! ... */` block containing a
  bare `@file` and explicit `@brief`. Describe the interface in the header
  and the implementation in the source. Keep extended prose, conventions,
  and diagrams in the source file.
- Document functions at their definitions, including inline functions
  defined in headers. For most functions, use `@brief`, `@param` and
  `@returns` where applicable. Mark parameters with `[in]`, `[out]`, or
  `[in,out]`, and explain constraints, ownership, and return values.
  Describe side effects and concurrency requirements where relevant.
- Use `/*!< ... */` for trailing Doxygen comments, `@code ... @endcode` for
  code blocks in source comments, and CommonMark for lists and other
  formatting.

This header documents the type at file scope and uses trailing comments
for member units and meaning.

```c
/*!
 * @file
 * @brief Request counter snapshot type
 */

#ifndef NETATALK_REQUEST_COUNTS_H
#define NETATALK_REQUEST_COUNTS_H

#include <stdint.h>

struct request_counts {
    uint64_t completed; /*!< Requests completed since startup */
    uint64_t failed;    /*!< Completed requests that returned an error */
};

#endif
```

This example documents the input extent, the output on failure, and the byte
order. It assumes the declarations from `<arpa/inet.h>`, `<stdbool.h>`,
`<stddef.h>`, `<stdint.h>`, and `<string.h>`.

```c
/*!
 * @brief Read a 16-bit network-order field
 *
 * @param[in]  buf        buffer containing at least len readable bytes
 * @param[in]  len        readable buffer length in bytes
 * @param[in]  offset     field offset in bytes
 * @param[out] value      non-null output pointer for the host-order value
 * @returns true if the field was read, false if it extends past the buffer
 *          and value is left unchanged
 */
static bool read_wire_u16(const uint8_t *buf, size_t len, size_t offset,
                          uint16_t *value)
{
    uint16_t field;

    if (offset > len || sizeof(field) > len - offset) {
        return false;
    }

    memcpy(&field, buf + offset, sizeof(field));
    *value = ntohs(field);
    return true;
}
```

## Static analysis and review

Netatalk uses the SonarQube Cloud **Netatalk way** quality profile for C.
Use the [live profile](https://sonarcloud.io/organizations/netatalk/rules?qprofile=AZLP55-mAtPTfzv1rpEr&activation=true)
to inspect currently active rules.

New code must introduce no Security, Reliability, or Maintainability issues.
Changed code must be free of Security and Reliability issues. If a finding is a
false positive, explain the reasoning during review. Keep any necessary
`//NOSONAR` suppression local and record why the code is safe, since it
suppresses all issues reported on that line.
