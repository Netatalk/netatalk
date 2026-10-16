# Netatalk Controller Daemon

The `netatalk` controller starts and supervises the AFP listener, registers
service advertisements, and manages a private D-Bus daemon when Localsearch
requires it. AFP workers own client requests, filesystem access, and CNID
connections. See the [Architecture Overview](developer.md) for the
relationship between the controller, listener, and workers.

## Key modules

| Location | Responsibility |
| --- | --- |
| `etc/netatalk/netatalk.c` | Startup validation, service state, event callbacks, reload, and shutdown |
| `etc/netatalk/afp_zeroconf.c`, `etc/netatalk/afp_zeroconf.h` | Service discovery facade and service type constants |
| `etc/netatalk/afp_avahi.c`, `etc/netatalk/afp_mdns.c` | Avahi and mDNSResponder implementations |
| `libatalk/util/netatalk_conf.c` | Shared configuration parsing and volume loading |
| `etc/netatalk/meson.build`, `meson.build` | Controller dependencies and event-library selection |
| `distrib/initscripts/` | Platform service templates |

## Startup and service state

main() parses command-line options, validates the execution mode, takes the
PID-file lock, and calls afp_config_parse(). It loads volume definitions
through load_afp_conf_vols() with `LV_ALL` before starting services. The
controller needs those definitions for service advertisements and optional
indexer setup, even though it does not serve volume operations itself.

run_process() uses fork() and execv(). run_afpd() passes `-d` to keep the
listener in foreground mode and `-F` to select the configuration file. It
also passes `-u` in single-user mode. Startup then installs event watchers,
performs optional Localsearch setup, registers Zeroconf services, and enters
the event loop.

The `afpd_pid` and `dbus_pid` slots encode both child PIDs and service policy:

| Slot value | Meaning |
| --- | --- |
| `NETATALK_SRV_NEEDED` (`-1`) | Service needs starting or restarting |
| `NETATALK_SRV_OPTIONAL` (`0`) | Optional service is disabled |
| `NETATALK_SRV_ERROR` (also `-1`) | Start failed or tracked child exited; retry is needed |
| Child PID | Service is running |

sigchld_impl() reaps children with waitpid() and marks an exited tracked
service for restart. The one-second timer_impl() retries required services
unless shutdown has begun. Restart counters are diagnostic; they do not
limit the number of attempts. The controller tracks the listener and its
optional D-Bus child, while the listener tracks AFP session workers.

## Event library and signals

Meson's `with-eventloop` option accepts `auto`, `libevent`, or `libev`;
`auto` prefers libevent. Both implementations wrap the same behavior
functions. Libevent uses event_new() callbacks and event_base_dispatch();
libev uses signal/timer watchers and ev_run(). The listener and workers
have independent poll loops described in [AFP Daemon](afpd.md).

| Function | Controller action |
| --- | --- |
| timer_impl() | Restart required services |
| sigchld_impl() | Reap children and report when shutdown can finish |
| sighup_impl() | Reload volumes, refresh advertisements, and signal the listener |
| sigterm_impl() | Enter shutdown, suppress restarts, and send SIGTERM to services |
| sigquit_impl() | Forward SIGQUIT to services |

In normal mode, SIGHUP reloads the controller's volume list and refreshes
Zeroconf registration before signaling `afpd`. The listener separately
reparses its configuration and signals workers. Options of an already-open
volume have their own reload rules; see [Configuration Internals](configuration.md).

SIGTERM sets `in_shutdown` and forwards SIGTERM to tracked services. The
event adapter arranges loop exit after `KILL_GRACETIME` (five seconds).
If all tracked children exit sooner, sigchld_impl() permits an earlier exit.
After the loop, surviving tracked services receive SIGKILL.

SIGQUIT forwards the signal without setting `in_shutdown` or starting the
SIGTERM grace timer. Its children remain subject to ordinary restart checks.

## Spotlight service setup

any_volume_uses_localsearch() gates private D-Bus startup in builds with
Localsearch support. Spotlight volumes using only the CNID or Xapian
backend do not require this service.

For Localsearch, the controller sets the D-Bus address, dconf profile, and
XDG environment, then starts the configured D-Bus executable. The dconf
profile is installed from `config/dconf/netatalk`. set_sl_volumes() writes
the database source and locks for the indexer's directory settings, then
invokes `dconf compile` through fork/exec. Directory configuration depends
on the installed profile, generated files, and compile result.

Search queries execute in AFP workers. Backend implementations live in
`etc/spotlight/`; see [Indexed Spotlight Search](indexing.md) for backend
selection, indexing, and query ownership.

## Service discovery

The Zeroconf facade selects the compiled Avahi or mDNSResponder implementation.
Its service types include `_afpovertcp._tcp` for AFP, `_adisk._tcp` for
Time Machine, and `_device-info._tcp` for device identity.

Configuration supplies the advertised server name, port, model, and eligible
Time Machine volumes. Listener setup and filesystem authorization have
separate lifetimes from registration. When changing advertisements, check
both backend implementations and the volume/configuration inputs they use.

## Single-user execution

`--single-user` (`-u`) requires a non-root caller and `-P` with an absolute
PID-file path in a private caller-owned directory. validate_singleuser_config()
also requires:

* A caller-owned configuration file, without group/other write access,
  inside a private caller-owned directory.
* `uam list = uams_srp.so` and a private SRP verifier directory containing
  the caller's verifier.
* At least one static volume, successful loading of every configured volume,
  and SQLite CNID for every volume.
* Volume access under the caller's identity and caller-owned or creatable
  CNID state.
* No AppleTalk, afpstats, Homes volumes, admin identity, or forced user/group
  identities.
* Only the CNID backend for volumes with Spotlight enabled.

The controller passes the mode to `afpd`; singleuser_admits_uid() in
`etc/afpd/auth.c` restricts login to the calling UID. SIGHUP is ignored,
so configuration changes require a restart. The controller, shared parser,
and authentication path enforce the policy together. See
[Authentication and Session Identity](authentication.md) and
`test/integration/singleuser_test.sh` for the login contract and integration
coverage.

## Platform service integration

Service definitions are generated from `distrib/initscripts/`. Consult the
platform template when changing startup flags or reload behavior:
`systemd.netatalk.service.in` defines systemd integration, while
`macos.netatalk.plist.in` runs the controller in foreground mode under launchd.
Templates preserve installation-path substitutions needed by different builds.

AppleTalk daemons have independent service scripts. Their lifecycle is
described in [AppleTalk](ddp.md).
