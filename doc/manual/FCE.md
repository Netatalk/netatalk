# Filesystem Change Events

Netatalk includes a filesystem change event (FCE) mechanism that allows **afpd**
processes to report selected AFP session and filesystem activity.

There are two ways to capture emitted FCE events:

- send FCE packets over UDP to one or more listener applications
- execute a local notification script for every emitted event

The two methods are independent.  A server may use either one, or both at the
same time.

FCE support must be enabled at build time.  The Meson option is
*-Dwith-fce=true*, which is enabled by default.

## Events

The following event names are used in the **fce events** option:

- file modification (**fmod**)
- file deletion (**fdel**)
- directory deletion (**ddel**)
- file creation (**fcre**)
- directory creation (**dcre**)
- file move or rename (**fmov**)
- directory move or rename (**dmov**)
- user login (**login**)
- user logout (**logout**)

FCE protocol version 1 supports the basic file and directory events:

- **fmod**
- **fdel**
- **ddel**
- **fcre**
- **dcre**

FCE protocol version 2 adds event metadata such as the process id and user name,
and is required for move, login, and logout events:

- **fmov**
- **dmov**
- **login**
- **logout**

For new deployments, use **fce version = 2**.

## Capturing Events with UDP

The **fce listener** option sends FCE events as UDP datagrams to a host and
optional port:

```ini
[Global]
fce listener = localhost:12250
fce version = 2
fce events = fmod,fdel,ddel,fcre,dcre,fmov,dmov,login,logout
```

If no port is specified, Netatalk uses port *12250*.
Multiple listeners can be configured by adding **fce listener** more than once.

Netatalk includes a simple listener application, **fce_listen**, that can be used
for testing:

```sh
$ fce_listen -h localhost -p 12250
Listening for Netatalk FCE datagrams on localhost:12250...
FCE Start
ID: 1, Event: FCE_LOGIN, pid: 429924, user: myuser, Path:
ID: 2, Event: FCE_FILE_CREATE, pid: 429924, user: myuser, Path: /srv/afp/untitled folder
ID: 3, Event: FCE_DIR_MOVE, pid: 429924, user: myuser, source: /srv/afp/untitled folder, Path: /srv/afp/My Folder
```

UDP delivery is useful for external indexers, audit collectors, or other
programs that should receive events without being executed by **afpd**.
Because UDP is not reliable, receivers should use the FCE event id to detect
missing packets.

If a burst of filesystem activity causes packet loss, **fce sendwait** can add a
small delay between emitted UDP events:

```ini
[Global]
fce listener = localhost:12250
fce sendwait = 10
```

The value is in milliseconds and must be between *0* and *999*.

## Capturing Events with a Script

The **fce notify script** option executes a local script for every emitted FCE
event:

```ini
[Global]
fce notify script = /usr/local/libexec/netatalk/fce_ev_script
fce version = 2
fce events = fmod,fdel,ddel,fcre,dcre,fmov,dmov,login,logout
```

The script method does not require **fce listener**.
It can be used by itself, or together with UDP listeners.

Netatalk runs the script in the background through */bin/sh -c*.
The configured script path is followed by command-line options describing the
event.  For example:

```sh
running /usr/local/libexec/netatalk/fce_ev_script -v 2 -e FCE_FILE_CREATE -i 2 -P '/srv/afp/Example File' -p 60246 -u 'myuser' as user 503
```

The options passed to the script are:

- **-v VERSION**: FCE script argument version
- **-e EVENT**: event name, such as **FCE_FILE_CREATE**
- **-i ID**: event id
- **-P PATH**: event path, when the event has a path
- **-S SOURCE**: source path for move or rename events
- **-p PID**: **afpd** process id
- **-u USER**: AFP session user

The **-p** and **-u** options are available with **fce version = 2**.

The installed **fce_ev_script** helper accepts these options and writes a
human-readable event line to syslog with the tag *netatalk-fce*.
The source script is maintained as *contrib/scripts/fce_ev_script.sh*.

Example output:

```text
FCE Event: FCE_FILE_MOVE, protocol: 2, ID: 4, pid: 429924, user: myuser, source: /srv/afp/old.txt, path: /srv/afp/new.txt
```

## Script Execution Environment

The notification script is executed as the AFP session user, not necessarily as
root or as the user that started Netatalk.

This has a few practical consequences:

- the script path must be executable by the AFP session user
- every parent directory in the script path must be searchable by that user
- files written by the script must be writable by that user
- relative paths should be avoided because the helper changes directory to */*
  before executing the command

The build system installs the helper under Netatalk's libexec directory.
Replace */usr/local/libexec* with the libexec directory configured for your
installation.
Configure **afpd** with that installed path:

```ini
[Global]
fce notify script = /usr/local/libexec/netatalk/fce_ev_script
```

For diagnostic file logging, wrap the helper and set
**NETATALK_FCE_LOG_FILE**:

```sh
#!/bin/sh
NETATALK_FCE_LOG_FILE=/tmp/fce.log
export NETATALK_FCE_LOG_FILE
exec /usr/local/libexec/netatalk/fce_ev_script "$@"
```

If file logging is enabled but */tmp/fce.log* is not updated, check the owner
and mode of the log file.

## Event Filtering and Coalescing

The **fce events** option controls which events are emitted:

```ini
[Global]
fce events = fcre,dcre,fmov,dmov
```

The **fce ignore names** option suppresses events for selected filenames.
The default ignored name is *.DS_Store*.

```ini
[Global]
fce ignore names = .DS_Store,Thumbs.db
```

The **fce ignore directories** option suppresses events below selected absolute
directory paths.  The paths must not end with a slash.

```ini
[Global]
fce ignore directories = /srv/afp/cache,/srv/afp/tmp
```

The **fce coalesce** option can reduce noisy create and delete event bursts:

```ini
[Global]
fce coalesce = all
```

File modification events are delayed by **fce holdfmod**, which defaults to
*60* seconds.  For immediate script testing, set it to *0*:

```ini
[Global]
fce holdfmod = 0
```
