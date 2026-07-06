# Name

fce_ev_script — Netatalk Filesystem Change Events notification helper

# Synopsis

**fce_ev_script** [-h] [-v *version*] [-e *event*] [-i *id*] [-P *path*] [-S *source*] [-p *pid*] [-u *user*]

# Description

**fce_ev_script** is a notification helper for Netatalk's Filesystem Change
Event (FCE) mechanism.  It is intended to be used with the **fce notify script**
option in *afp.conf*.

When **afpd** emits an FCE event and **fce notify script** is configured,
**afpd** executes the configured script in the background and passes event
details as command-line options.

By default, **fce_ev_script** writes events to syslog with the tag
*netatalk-fce*.  If the **NETATALK_FCE_LOG_FILE** environment variable is set,
events are appended to that file instead.

> **Note:** This utility requires netatalk to be built with FCE support enabled
> (the *-Dwith-fce=true* meson build option, which is enabled by default).

# Options

**-h**
: Show help text and exit.

**-v** *version*
: FCE script argument version.

**-e** *event*
: FCE event name, such as *FCE_FILE_CREATE*.

**-i** *id*
: FCE event id.

**-P** *path*
: Event path.

**-S** *source*
: Source path for move or rename events.

**-p** *pid*
: Process id of the **afpd** process that emitted the event.

**-u** *user*
: AFP session user that emitted the event.

# Environment

**NETATALK_FCE_LOG_FILE**
: Append event lines to this file instead of syslog.  The file must be writable
by the AFP session users that may trigger events.

**NETATALK_FCE_LOGGER**
: Logger command to use when **NETATALK_FCE_LOG_FILE** is not set.  Default:
*logger*.

**NETATALK_FCE_LOG_TAG**
: Syslog tag to use when **NETATALK_FCE_LOG_FILE** is not set.  Default:
*netatalk-fce*.

# Execution Context

**afpd** executes the configured notification script as the AFP session user,
not necessarily as root or as the user that started Netatalk.

The configured script path must be executable by that user, and every parent
directory in the path must be searchable by that user.  If file logging is
enabled with **NETATALK_FCE_LOG_FILE**, the log file must also be writable by
that user.

# Examples

Configure **afpd** to run the installed helper for each FCE event:

    [Global]
    fce notify script = /usr/local/libexec/netatalk/fce_ev_script
    fce version = 2
    fce events = fmod,fdel,ddel,fcre,dcre,fmov,dmov,login,logout

Replace */usr/local/libexec* with the libexec directory configured for your
installation.

For diagnostic file logging, wrap the helper and set **NETATALK_FCE_LOG_FILE**:

    #!/bin/sh
    NETATALK_FCE_LOG_FILE=/tmp/fce.log
    export NETATALK_FCE_LOG_FILE
    exec /usr/local/libexec/netatalk/fce_ev_script "$@"

# See Also

**afpd**(8), **afp.conf**(5), **fce_listen**(1)
