# Configuration Internals

The shared configuration loader turns `afp.conf` into process options and
volume definitions. Its interpretation depends on option scope, lookup
helpers, caller identity, and the stage of the session lifecycle. The
maintained option reference is [afp.conf(5)](https://netatalk.io/manual/en/afp.conf.5.html).

## Key modules

| Location | Responsibility |
| --- | --- |
| `libatalk/util/netatalk_conf.c` | Parsing, validation, inheritance, substitutions, and volume construction |
| `include/atalk/netatalk_conf.h`, `include/atalk/globals.h`, `include/atalk/volume.h` | Loader API, process options, and volume state |
| `etc/afpd/afp_options.c`, `etc/afpd/afp_config.c` | AFP command-line parsing and listener/UAM setup |

## Parsing and ownership

afp_config_parse() loads an INI dictionary through iniparser, configures
logging early, parses global settings, validates bounds and aliases, and
initializes charset and extension-map state. Iniparser can be a system
dependency or Meson subproject. Paths represented by `_PATH_*` macros come
from the build configuration.

INIPARSER_GETSTR() and ordinary string lookups return dictionary-owned
storage. A duplication helper returns an owned heap copy. A field that must
outlive the dictionary needs independent storage; conversely, freeing a
borrowed value corrupts the dictionary. afp_config_free() releases parsed
configuration state, while volume teardown has its own lifecycle.

The controller and AFP listener parse their own configuration. The
controller passes `-F` to its child; it does not hand the listener a shared
live dictionary. `-F` selects configuration for running the daemon, rather
than providing a config-only syntax check.

## Sections and precedence

`[Global]` and `[Homes]` have special roles. Other sections define volumes
or supply presets selected by `vol preset`.

The internal `getoption_*` helpers look in the volume section, then its
selected preset, then use the supplied default. The `vdgoption_*` helpers
add a Global fallback. Some fields instead use an already-parsed global
value as their default. Consequently, a volume key placed under Global is
inherited only if its particular lookup path supports that behavior.

`vol dbpath` illustrates why precedence and interpretation must be read
together: a Global base path without substitutions gets the volume name
appended; a volume/preset path selects the directory directly.

Typed aliases also require validation-aware precedence. An unusable value
must not accidentally count as an explicit override that disables a safe
default. Follow the existing parser and regression cases when adding an
alias, rather than implementing a separate string-presence test.

## Constructing volumes

load_afp_conf_vols() builds definitions through creatvol(). The caller's
flags and session context affect the result:

| Flag | Meaning |
| --- | --- |
| `LV_DEFAULT` | Load in session context with authorization filtering |
| `LV_ALL` | Load without ordinary user authorization filtering |
| `LV_FORCE` | Force loading past the modification-time check |

Construction resolves presets, authorization, paths, storage selection,
charsets, UUIDs, and flags. Substitution through volxlate() depends on the
available user, client, and path context. Controller startup cannot assume
that values requiring a logged-in user are already resolvable.

Some volume settings also change process options. In particular,
apply_multiprotocol_defaults() supplies strict-locking and per-access
validation defaults unless usable explicit values override them.
Resource-fork admission remains volume-dependent. See
[Volume Lifecycle](volumes.md) and [Caching and Consistency](caching.md).

## Reload boundaries

Reload signaling and process actions are described in
[Netatalk Controller Daemon](netatalk.md) and [AFP Daemon](afpd.md).
The loader's treatment of existing volume objects limits what those
actions can change.

creatvol() retains an existing matching volume's options. Removed volumes
are handled according to their open/deleted state and `close vol` policy.
Reloading therefore does not guarantee that changed access, storage, or
other volume settings take effect in an already-loaded session. A fresh
session or restart can be necessary to construct fresh volume state.

## Adding or changing an option

Choose its process/volume scope and lifetime first. Then use the matching
lookup helper, define units and bounds, and track explicit overrides where
policy depends on them. Check allocation overflow as well as the numeric
range: DSI buffering multiplies a quantum by a readahead count, and
resource-fork configuration in KiB becomes a runtime byte budget.

Update the user-facing reference and any relevant template, then verify
default, explicit, invalid, inherited, and reload behavior. An option that
selects a storage format does not itself migrate existing files or databases.
See `test/afpd/subtests_conf.c` for parser regression patterns.
