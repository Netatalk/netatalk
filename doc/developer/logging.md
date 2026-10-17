# Error Checking, Logging, and Diagnostics

We want rigid error checking and concise log messages without burying the
relevant function call in repetitive checks. Netatalk provides logging through
`<atalk/logger.h>` and error checking macros through `<atalk/errchk.h>`.

The implementations live in `libatalk/util/logger.c` and the two headers
above. Worker diagnostic actions are in `etc/afpd/afp_dsi.c`, with cache
dumps/statistics in the corresponding cache modules.

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

A basic function template is:

```c
int func(void)
{
    EC_INIT;

    /* Your code here. */

    EC_STATUS(0);

EC_CLEANUP:
    EC_EXIT;
}
```

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

## Collecting a useful failure report

Record the source/build revision, server and client versions, configuration
path, execution mode, storage/search backends, and triggering AFP operation.
`afpd -V` reports compiled features, which does not prove that a configured
UAM loaded or a volume opened. Capture the first failure and nearby session
messages as well as the final client error.

Select logging by subsystem so a failure can be followed through transport,
handler, storage, and database layers. For example, an isolated test service
can enable `afpd`, `dsi`, `cnid`, `ad`, `uams`, or `sl` logging through
`log level`. Detailed debug output also depends on build settings. Check
the actual log destination and permissions before collecting output.
`test/integration/logreopen_test.sh` covers log reopening.

`-F` starts the daemon using the selected configuration; it is not a syntax
check command. Reproduce with the relevant test harness or isolated service
instead of starting another server against active volumes.

## Worker dumps and error interpretation

SIGINT to the intended DSI session worker requests a cache dump and toggles
detailed diagnostic logging. The action runs in the main loop. The parent
and controller have different signal behavior, so select the worker PID
deliberately.

Dump/log paths are constructed dynamically. Locate the file matching
`afpd.<worker-pid>.*` in tmpdir() instead of assuming a fixed path.

Session-close logs summarize directory, metadata, resource-fork, descriptor,
hint, and idle-worker statistics. Interpret them using
[Caching and Consistency](caching.md), and compare cold/warm runs using
[Performance Analysis](profiling.md).

Distinguish the layer that failed before mapping an AFP error:

| Symptom | Implementation boundary to inspect |
| --- | --- |
| Missing volume | Definition loading, authorization filtering, UUID/storage initialization, and retained reload state in [Volumes](volumes.md) |
| Transient database error versus missing ID | The unsigned CNID error contract in [Persistent File Identity](cnid.md) |
| Failed login | Built/enabled UAM, account policy, credential backend, and effective identity in [Authentication](authentication.md) |
| Lost session | Transport state, tickles, and reconnect transfer in [AFP Daemon](afpd.md); deliberate reset termination in [Caching](caching.md) |
| Empty search results | Selected backend, catalog/index coverage, query scope, and access filtering in [Indexed Spotlight Search](indexing.md) |

Discovery advertisements do not prove listener reachability, login, or volume
access. Likewise, contention is not evidence of database corruption, and a
catalog reset is not an ordinary first diagnostic step.

Choose a regression test at the failing boundary using `test/README.md`.
For a bug that spans layers, include both an internal edge-case test and a
client-visible AFP case, with backend, storage mode, and concurrency recorded.
