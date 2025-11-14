Error checking and logging
==========================

We want rigid error checking and concise log messages.
At the same time, we want to avoid overly verbose code where the relevant function call
is buried in error checking and logging statements.
In order to address both error checking and code readability,
we provide a set of error checking macros in <atalk/errchk.h>.
Every macro comes in four flavours: EC\_*CHECK*, EC\_*CHECK*\_LOG, EC\_*CHECK*\_LOGSTR
and EC\_*CHECK*\_LOG\_ERR where *CHECK* should be substituted with one of the supported tests:

* *ZERO* - the value equals 0
* *NULL* - the value equals NULL
* *NEG1* - the value equals -1

To explain the loggging logic of each error checking macro flavor,
using the *ZERO* test as an example:

* EC\_ZERO checks that the value equals 0 with a standard log message
* EC\_ZERO\_LOG additionally logs the stringified function call
* EC\_ZERO\_LOGSTR allows specifying an arbitrary string to log
* EC\_ZERO\_LOG\_ERR allows specifying the return value

The macros EC_CHECK* unconditionally jump to the cleanup label defined by the *EC_CLEANUP* macro
where the necessary cleanup can be done alongside validating the return value.

Examples
--------

stat() without EC macro:

```c
static int func(const char *name) {
    int ret = 0;
    ...
    if ((ret = stat(name, &some_struct_stat)) != 0) {
        LOG(...);
        /* often needed to explicitly set the error indicating return value */
        ret = -1;
        goto cleanup;
    }

    return ret;

cleanup:
    ...
    return ret;
}
```

stat() with EC macro:

```c
static int func(const char *name) {
    /* expands to int ret = 0; */
    EC_INIT;

    char *uppername = NULL;
    EC_NULL(uppername = strdup(name));
    EC_ZERO(strtoupper(uppername));

    /* expands to complete if block from above */
    EC_ZERO(stat(uppername, &some_struct_stat));

    EC_STATUS(0);

EC_CLEANUP:
    if (uppername) {
        free(uppername);
    }
    EC_EXIT;
}
```

A boilerplate function template is:

```c
int func(void)
{
    EC_INIT;

    ...your code here...

    EC_STATUS(0);

EC_CLEANUP:
    EC_EXIT;
}
```
