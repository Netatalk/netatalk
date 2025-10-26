# AGENTS.md

Netatalk is a free, open-source AFP 3.4 file server for \*NIX/BSD systems,
enabling Mac clients to share files over native Apple Filing Protocol. For agents
working on this codebase, this document provides essential context, build
commands, code patterns, and testing guidance.

## Project Structure Overview

**Core Components:**


- **netatalk** (`etc/netatalk/`) — Service controller daemon; manages afpd and cnid_metad lifecycle
- **afpd** (`etc/afpd/`) — Main AFP daemon; handles client connections, file operations, metadata
- **cnid_metad** + **cnid_dbd** (`libatalk/cnid/`) — CNID metadata databases for file tracking
- **libatalk** (`libatalk/`) — Shared library with protocol layers (DSI, ASP, ATP, NBP), AdoublE, Unicode conversion

**Key subdirectories in libatalk:**

- `dsi/` — DSI session layer (AFP over TCP/IP, port 548)
- `adouble/` — AppleDouble metadata format (._ files and extended attributes)
- `unicode/` — Character set conversion (UTF8-MAC, Apple codepages)
- `util/`, `acl/`, `vfs/` — Filesystem utilities and ACL support

## Build System

Netatalk uses **Meson** (only build system since v4.0.0). Out-of-tree builds required.

### Setup Commands

```bash
# Development build with debug symbols
meson setup build -Dbuildtype=debug

# Full-featured build with all optional components
meson setup build -Dbuildtype=debug -Dwith-tests=true -Dwith-testsuite=true

# Compile and install
meson compile -C build
meson install -C build

# Run all tests (outputs to build/meson-logs/testlog.txt)
meson test -C build

# Verify capabilities
netatalk -V    # service controller
afpd -V        # AFP daemon
```

### Key Build Options

- `-Dwith-afpstats` — D-Bus statistics interface (enabled by default)
- `-Dwith-spotlight` — Spotlight search support (requires tinysparql)
- `-Dwith-appletalk` — AppleTalk transport for legacy Macs (disabled by default)
- `-Dwith-debug` — Verbose debug code
- `-Dwith-tests` — Unit tests (requires "last" CNID backend)
- `-Dwith-testsuite` — Integration tests

Build options defined in `meson_options.txt`; feature gates output to `config.h`.

## Code Style & Patterns

### Error Handling

Use `EC_*` macros from `include/atalk/errchk.h` instead of raw if/goto chains:

```c
EC_INIT;                                    /* Expands to: int ret = 0; */
EC_ZERO_LOG(stat(path, &st));              /* Check, log on failure, goto cleanup */
EC_NULL_LOG(ptr = malloc(size));           /* Check for NULL, log, goto cleanup */
/* ... work ... */
EC_CLEANUP:
    /* cleanup code */
    return ret;
```

Macros: `EC_INIT`, `EC_ZERO_LOG`, `EC_NULL_LOG`, `EC_CHECK_LOG`, `EC_CHECK_LOG_ERR`, `EC_CUSTOM`.

### Logging

Use `LOG(log_level, logtype_component, "format", args)` from `include/atalk/logger.h`:

```c
LOG(log_error, logtype_afpd, "Failed to open volume: %s", path);
LOG(log_debug, logtype_default, "Connection from %s", client_ip);
```

Available logtypes: `logtype_default`, `logtype_afpd`, `logtype_cnid`, etc.

### Configuration

- **Single config file**: `afp.conf` (parsed into `AFPObj` struct)
- **Parsing**: `etc/afpd/afp_config.c` (parsing logic), `etc/afpd/afp_options.c` (defaults)
- **New options**: Add to `meson_options.txt` → configure in `meson.build` → reference in `config.h` → parse in `afp_config.c`

### File Metadata

- **Default**: Extended attributes (xattr)
- **Legacy**: AppleDouble v2 files (._ prefix); conversion via `dbd` tool
- **Charset handling**: UTF8-MAC (AFP 3.x), Apple codepages (AFP 2.x)

### Threading Model

- **afpd**: Process-per-connection (forking daemon)
- **cnid_dbd**, **netatalk**: Thread-safe; use mutexes where needed
- **Example**: `afpstats` (`etc/afpd/afpstats.{h,c}`) manages child process stats via `server_child_t` with D-Bus exposure

## Testing & Validation

### Unit Tests (afpd component tests)

```bash
# Build with tests enabled
meson setup build -Dbuildtype=debug -Dwith-tests=true
meson compile -C build

# Run tests from within build directory
meson test -C build
```

Located in `test/afpd/`. Requires "last" CNID backend. Stateless tests, run automatically during build.

### Integration Tests (System testsuite - Docker recommended)

Build and run test container:

```bash
# Build Alpine container image
docker build --no-cache -f distrib/docker/testsuite_alp.Dockerfile -t netatalk-test:latest .

# Run testsuite interactively (e.g., afp_lantest)
docker run --rm -it --network host --cap-add=NET_ADMIN \
  --volume "<local_dir>:/mnt/afpshare" \
  --volume "<local_dir>:/mnt/afpbackup" \
  --env AFP_USER=test --env AFP_PASS=test --env AFP_GROUP=test \
  --env INSECURE_AUTH=true --env SHARE_NAME='File Sharing' \
  --env TESTSUITE=lan --env TEST_FLAGS="-n 2" \
  --name netatalk-test netatalk-test:latest
```

Located in `test/testsuite/`. Requires full Netatalk instance with configured volumes and test users.

**Alternative test suites**: `TESTSUITE=login` (afp_logintest), `TESTSUITE=spectest` (afp_spectest), `TESTSUITE=speed` (afp_speedtest).

For local native testing, see the [Testing wiki](https://github.com/Netatalk/netatalk/wiki/Testing#local---run-testsuite-natively).

### Code Formatting Check

Enforce formatting **before committing**:

```bash
./contrib/scripts/codefmt.sh -v -s c        # Check C formatting (.astyle)
./contrib/scripts/codefmt.sh -v -s markdown # Check markdown
./contrib/scripts/codefmt.sh -v -s meson    # Check meson build files
./contrib/scripts/codefmt.sh -v -s perl     # Check Perl scripts
./contrib/scripts/codefmt.sh -v -s shell    # Check shell scripts
./contrib/scripts/codefmt.sh -v             # Check all
```

## Common Development Tasks

### Adding a New Configuration Option

1. Define in `meson_options.txt` with `type: 'boolean'` or `type: 'string'`
2. Reference in `meson.build` build definitions (e.g., `if get_option('with-feature')`)
3. Access in C code via `config.h` (e.g., `#ifdef HAVE_FEATURE`)
4. Parse user values in `etc/afpd/afp_config.c` and store in config struct
5. Set defaults in `etc/afpd/afp_options.c`

### Adding a New afpd Command

1. Implement handler in appropriate file under `etc/afpd/` (e.g., `file.c`, `directory.c`)
2. Register in dispatcher (typically `switch.c`)
3. Add test in `test/afpd/` if applicable
4. Update man pages in `doc/manpages/man1/` or `man8/`

### Daemon Lifecycle Management

See `etc/netatalk/netatalk.c` for:

- Process spawning (`run_process()`)
- Signal handling (SIGTERM graceful shutdown, SIGHUP reload)
- Child process cleanup

## Commit & PR Guidelines

### Commit Message Format

```text
component: description, GitHub #issue

More detailed explanation if needed. Wrap at ~72 characters.
Reference issue number at end.
```

Examples:

- `afpd: fix thread safety in afpstats, GitHub #2401`
- `libatalk: improve Unicode normalization for UTF8-MAC, GitHub #1234`
- `meson: allow shared/static library configuration`

### PR Title Format

- **main branch**: No tags, descriptive one-liner
- **stable branch** (e.g., `branch-netatalk-4-2`): Prepend `[4.2]` tag

Examples:

- `Fix AppleDouble header parsing for corrupted files`

- `[4.2] meson: Allow choosing shared or static libraries to build`

### Before Committing

- Format code: `./contrib/scripts/codefmt.sh -v -s c`
- Run tests: `meson test -C build` (or subset: `meson test -C build --suite afpd`)
- Verify no SonarQube regressions (CI enforces static analysis)

## Development Workflow

1. **Create feature branch** from `main` or appropriate stable branch
2. **Make changes** following code style and patterns above
3. **Test locally**: `meson test -C build`
4. **Format code**: `./contrib/scripts/codefmt.sh -v -s c`
5. **Commit** with proper message format
6. **Push** and open Pull Request with descriptive title and summary
7. **Code review**: Automated checks (formatting, SonarQube) + peer review
8. **Rebase merge** only (no branch merges allowed)

## Documentation & References

- **Architecture deep dive**: `doc/developer/developer.md` (protocol stacks, error checking patterns)
- **Configuration reference**: `doc/manual/Configuration.md` (charset, volumes, options)
- **Build details per OS**: `COMPILATION.md`
- **Quick start**: `INSTALL.md`
- **Contributing**: `CONTRIBUTING.md` (PR workflow, copyright headers, license info, coding standards)

## Important Files to Know

- `meson.build` — Top-level build config; version at line 2
- `meson_options.txt` — Feature flags and build options
- `afp.conf` (generated) — Runtime configuration template
- `etc/afpd/afpstats.{h,c}` — D-Bus statistics interface (thread-safe child process management)
- `etc/netatalk/netatalk.c` — Service controller (daemon lifecycle)
- `include/atalk/errchk.h` — Error checking macros
- `include/atalk/logger.h` — Logging interface
- `.github/workflows/` — CI/CD checks (build, test, formatting, static analysis)

## Version Info

Current development version in `meson.build` line 2: `version: '4.4.0dev'`

Stable branches follow `branch-netatalk-X-Y` naming (e.g., `branch-netatalk-3.2`).

## Cross-Platform Considerations

Supports: Linux (Alpine, Arch, Debian, Ubuntu, RHEL, SuSE, Fedora), macOS, *BSD, Solaris, illumos
