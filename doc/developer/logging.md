# Error checking and logging

We want rigid error checking and concise log messages without burying the
relevant function call in repetitive checks. Netatalk provides logging through
`<atalk/logger.h>` and error checking macros through `<atalk/errchk.h>`.

## Logging

Use `LOG(level, type, format, ...)` for daemon diagnostics. Both the severity
and subsystem are required. Choose a subsystem such as `logtype_afpd`,
`logtype_cnid`, or `logtype_dsi` so its logging configuration applies.
Use `logtype_default` when no specific subsystem is appropriate.

Use `log_error` for failures, `log_warning` for recoverable conditions worth
attention, `log_info` for general information, and `log_debug` or the more
verbose debug levels for diagnostic detail. The available levels and subsystems
are defined in `include/atalk/logger.h`.

```c
LOG(log_error, logtype_afpd, "stat failed: %s", strerror(errno));
LOG(log_debug, logtype_dsi, "Received %zu bytes", received);
```

Keep messages concise and include enough context to diagnose the problem.
Log a failure where its context is available, avoiding duplicate messages as
the error propagates. Do not log credentials or other secrets.
Only report `strerror(errno)` when the failed operation sets `errno`.

## Error checking macros

The checking macros evaluate an expression and jump to the cleanup label only
when their failure condition is met:

| Macro family | Failure condition |
| --- | --- |
| `EC_ZERO` | The expression is nonzero |
| `EC_NULL` | The expression is `NULL` |
| `EC_NEG1` | The expression is `-1` |

Each family has these variants, using `EC_ZERO` as an example:

* `EC_ZERO(expr)` checks without logging and sets `ret` to `-1` on failure.
* `EC_ZERO_LOG(expr)` also logs the stringified expression and
  `strerror(errno)` at `log_error` with `logtype_default`.
* `EC_ZERO_LOGSTR(expr, format, ...)` logs a caller-supplied message and sets
  `ret` to `-1` on failure.
* `EC_ZERO_LOG_ERR(expr, error)` logs the expression and `strerror(errno)`,
  and sets `ret` to the supplied error value on failure.

Use the variants that log `errno` only with operations that set it on failure.
Check `include/atalk/errchk.h` for the complete definitions and additional macros.

`EC_INIT` declares `int ret = 0`; `EC_STATUS(value)` assigns it without jumping.
`EC_CLEANUP:` defines the `cleanup:` label, and `EC_EXIT` returns `ret`.
`EC_FAIL` sets `ret` to `-1` and jumps to cleanup unconditionally;
`EC_EXIT_STATUS(value)` assigns the supplied value and jumps to cleanup.

Initialize every resource used by cleanup before a check can jump there.
Choose the check to match the called function's return contract. The basic
checking macros replace errors with `-1`, so use an explicit check or a suitable
error-value variant when the caller needs a particular error code, such as an
AFP protocol error.

## Examples

These examples assume the standard headers for `errno`, allocation, strings,
and `stat()`, together with `<atalk/logger.h>` and `<atalk/errchk.h>`.
The caller supplies a valid pathname and output structure. Both examples
release the duplicated pathname on success and failure and return 0 or -1.

Without error checking macros:

```c
static int file_stat(const char *name, struct stat *st)
{
    int ret = 0;
    char *path = NULL;

    path = strdup(name);
    if (path == NULL) {
        LOG(log_error, logtype_default, "strdup failed: %s", strerror(errno));
        ret = -1;
        goto cleanup;
    }

    if (stat(path, st) != 0) {
        LOG(log_error, logtype_default, "stat failed: %s", strerror(errno));
        ret = -1;
        goto cleanup;
    }

cleanup:
    free(path);
    return ret;
}
```

With error checking macros:

```c
static int file_stat(const char *name, struct stat *st)
{
    EC_INIT;
    char *path = NULL;

    EC_NULL_LOG(path = strdup(name));
    EC_ZERO_LOG(stat(path, st));

EC_CLEANUP:
    free(path);
    EC_EXIT;
}
```
