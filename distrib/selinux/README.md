# Netatalk SELinux Policy Module

This sample policy targets **SQLite CNID and CNID Spotlight** on SELinux-enabled
Linux systems. It confines `netatalk` and `afpd` in the `netatalk_t` domain.
Validate the policy with your distribution, authentication setup, and shared
volumes before deploying it in enforcing mode.

## Recommended configuration

Migrating from the deprecated **dbd CNID backend to SQLite is recommended for
security-conscious deployments**. SQLite operates in-process, eliminating the
`cnid_metad` and `cnid_dbd` daemons and their database communication channels.
SQLite is the default CNID backend when compiled in; CNID is the default
Spotlight backend.

Set these options explicitly in `afp.conf` and check volume sections and presets
for overrides:

```ini
[Global]
cnid scheme = sqlite
spotlight backend = cnid
```

Spotlight search still needs `spotlight = yes` on the volumes where it is wanted.
The CNID Spotlight backend searches filenames recorded in the CNID database; it
does not provide Localsearch's full-text content indexing.

The dbd backend remains available in Netatalk, but **this policy provides no
compatibility mode for it**. `cnid_metad` and `cnid_dbd` receive a separate
executable label with no execution permission from `netatalk_t`.

Localsearch Spotlight is not recommended for this configuration. Its
`dbus-session.conf` receives a separate label that `netatalk_t` cannot read, and
Netatalk is not allowed to execute `dbus-daemon`. Avahi service discovery can
still use the **system bus**. The module does not enable Localsearch's private
session bus, shell hooks, or arbitrary helper execution.

Check `afpd -v` and `netatalk -v` for the backends present in your installation.
The build installs `dbus-session.conf` only when Localsearch support is enabled;
existing copies still need the restricted label when installing this policy.

### Migrating existing dbd volumes

Before installing this policy:

1. Disconnect AFP clients, stop Netatalk, and verify that `afpd`, `cnid_metad`,
   and `cnid_dbd` have stopped. Keep Netatalk stopped throughout the migration.
2. Back up `afp.conf`, volume metadata, CNID databases, and the state files
   `afp_signature.conf` and `afp_voluuid.conf`.
3. Set `cnid scheme = sqlite` for every volume, including presets and dynamic
   home volumes. Verify SQLite is compiled in. Set `spotlight backend = cnid`
   wherever Spotlight is used.
4. Rebuild each volume's SQLite CNID database using the administrator-run
   `dbd` utility, following [dbd(1)](../../doc/manpages/man1/dbd.1.md).
   For example, `dbd -f /srv/afp/share` recreates the configured volume's CNID
   table. This is a rebuild, not an in-place conversion of Berkeley DB files;
   `-f` deletes the target table's existing records. Preserve backups and
   validate client references after migration.
5. Install the policy and label database and shared-volume paths as described
   below. Restart Netatalk and verify AFP operations and searches. Confirm that
   neither deprecated daemon nor a private Spotlight bus starts.

## Build and installation

The supplied file contexts assume executables in `/usr/sbin`, configuration in
`/etc/netatalk`, state in `/var/lib/netatalk`, and the default lock file at
`/var/lock/netatalk` (also covered as `/run/lock/netatalk`). Netatalk's state path
is configurable; check `afpd -v` and `netatalk -v`. Executables are also matched
in `/usr/bin` for distributions that merge `/usr/sbin` into it. For a source
build using this layout, set `--prefix=/usr --sysconfdir=/etc -Dwith-statedir-path=/var/lib`.
Adapt the file contexts or add persistent local mappings for other layouts
**before starting the service**, including mappings for excluded files.

On Fedora, install build and inspection tools:

```shell
sudo dnf install selinux-policy-devel selinux-policy-targeted \
    policycoreutils-python-utils libselinux-utils setools-console rpm-build make
```

Compile without installing anything or requiring root:

```shell
./netatalk.sh --build
```

With Netatalk stopped, install and relabel the existing standard paths:

```shell
sudo ./netatalk.sh
```

This also generates a policy man page and an RPM for distribution. The RPM
relabels the same standard paths on installation and removal. Neither route
relabels custom paths or chooses which volumes to export. Restart Netatalk only
after completing the site-specific labeling below. Existing processes do not
change domain when a policy or executable label changes.

### Shared volumes and custom database paths

Assign `netatalk_share_t` only to the directories that AFP should serve. For
example, for an existing dedicated share:

```shell
sudo semanage fcontext -a -t netatalk_share_t '/srv/afp/share(/.*)?'
sudo restorecon -Rv /srv/afp/share
```

For an existing dedicated directory selected with `vol dbpath`:

```shell
sudo semanage fcontext -a -t netatalk_cnid_t '/srv/afp-database(/.*)?'
sudo restorecon -Rv /srv/afp-database
```

Database labeling must cover the directory, SQLite databases, and their
`-wal`, `-shm`, and journal files. New files inherit the database directory's
label. Prefer a dedicated database directory. For `vol dbnest` or a database
inside a share, inspect the actual layout and add specific database mappings;
do not relabel the whole shared volume as database storage. Local fcontext
rules take precedence over module rules, so check overlapping mappings.

Parent directories must be searchable by the service. Home directories,
alternate authentication modules, and shares also served by Samba may need
additional site-specific policy; this sample does not grant access to all home
directories or all filesystem content. Maintenance tools run under the
administrator's policy, rather than gaining service-domain access automatically.

For example, a share at `/mnt/afp/share` needs search permission on both
`/mnt` and `/mnt/afp`. Do not grant `netatalk_t` search access to every mounted
filesystem merely to support one share. Instead, add a local type for precisely
the required parent directories:

```te
# netatalk-local.te
module netatalk_local 1.0;

require {
    type netatalk_t;
    attribute file_type;
    class dir search;
}

type netatalk_share_parent_t;
typeattribute netatalk_share_parent_t file_type;

allow netatalk_t netatalk_share_parent_t:dir search;
```

Compile and install this small local module, then label each parent directory
exactly (without a recursive `(/.*)?` expression):

```shell
checkmodule -M -m -o netatalk-local.mod netatalk-local.te
semodule_package -o netatalk-local.pp -m netatalk-local.mod
sudo semodule -i netatalk-local.pp
sudo semanage fcontext -a -f d -t netatalk_share_parent_t '/mnt'
sudo semanage fcontext -a -f d -t netatalk_share_parent_t '/mnt/afp'
sudo restorecon -v /mnt /mnt/afp
```

Label `/mnt/afp/share(/.*)?` as `netatalk_share_t` as above. For a different
layout, label only the ancestors that Netatalk must traverse. Dynamic home
shares need a deliberate site-specific policy design; do not use this example
to label all home directories as traversal parents.

## Policy overview

| Type | Purpose |
| --- | --- |
| `netatalk_t` | Netatalk and AFP processes |
| `netatalk_exec_t` | Supported daemon executables |
| `netatalk_legacy_exec_t` | Deprecated daemons, not executable by Netatalk |
| `netatalk_etc_t` | Supported configuration files |
| `netatalk_dbus_conf_t` | Excluded private session-bus configuration |
| `netatalk_var_lib_t` | State directory, signatures, and volume UUIDs |
| `netatalk_cnid_t` | SQLite databases and auxiliary files, including mapping and locking |
| `netatalk_share_t` | Administrator-selected AFP share content |
| `netatalk_lock_t` | PID lock files |
| `netatalk_log_t` | Log files |

The policy uses named file transitions for standard state, lock, and log
creation instead of allowing writes to generic `/var` and lock files.
It does not grant generic binary or shell execution, or directly grant generic
port binding. Distribution interfaces can add conditional permissions: for
example, Fedora's NSS support allows generic port binding when `nis_enabled`
is enabled. Keep that boolean disabled for this configuration and review the
effective policy and active booleans on your host.
Reference policy currently labels TCP 548 as `dhcpd_port_t`, so the policy uses
`corenet_tcp_bind_dhcpd_port`. This type also covers other ports; check
`semanage port -l` on the target distribution. A site needing an exclusive AFP
port type must define that type and update the port mapping and bind rule
together. Nonstandard AFP ports require corresponding policy configuration.

SELinux restrictions apply to correctly labeled files and confined processes
in enforcing mode. They do not prevent an administrator from changing the
configuration, labels, or installed policy. `neverallow` assertions help catch
accidental policy expansions at build/link time; they are not runtime deny
rules that override other modules. Some distributions disable these checks by
default (`expand-check=0` in `semanage.conf`); enable them in the test policy
store when validating assertions and also inspect effective permissions below.

## Validation and troubleshooting

On an enforcing test host, verify:

- `ps -eZ` shows both supported daemons in `netatalk_t` after a service restart.
- `matchpathcon` and `ls -Z` agree for executables, configuration, state, and shares.
  In particular, `dbus-session.conf` must not have `netatalk_etc_t`.
- AFP authentication, file creation/read/write/rename/delete, metadata updates,
  concurrent SQLite sessions, and CNID Spotlight searches work.
- Avahi discovery works with the system bus.
- Attempts from the confined service to execute either deprecated daemon or
  `dbus-daemon`, or to read `dbus-session.conf`, are denied. Test configuration
  changes only against disposable volumes. An administrator's unconfined shell
  is not an equivalent negative test.

Use `sesearch` against the installed policy to inspect effective allows. These
queries should return no allows, including conditional ones:

```shell
sesearch --allow -s netatalk_t -t netatalk_legacy_exec_t -c file -p execute
sesearch --allow -s netatalk_t -t netatalk_dbus_conf_t -c file -p read
if seinfo -t dbusd_exec_t | grep -q '^[[:space:]]*dbusd_exec_t[[:space:]]*$'; then
    sesearch --allow -s netatalk_t -t dbusd_exec_t -c file -p execute
fi
```

The D-Bus query runs only when the optional policy provides `dbusd_exec_t`.
The guard checks for the type in `seinfo`'s output because an empty result
(`Types: 0`) can still have a successful exit status.

To inspect recent AVC denials:

```shell
sudo ./netatalk.sh --audit
```

No matching audit records may produce a nonzero exit status. Check the selected
backends, Unix permissions, labels, local fcontext overrides, and parent-directory
access before adding permissions. Denials for excluded backends are intentional:
correct the configuration or complete the migration. Do not feed these denials
into `audit2allow`. The former `--update` option has been removed; the helper no
longer appends generated rules to the policy.

## Further reading

- [afp.conf(5)](https://netatalk.io/manual/en/afp.conf.5)
- [SELinux Project Wiki](https://github.com/SELinuxProject/selinux/wiki)
- [Red Hat SELinux Guide](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/9/html/using_selinux/index)
- [Fedora SELinux Guide](https://docs.fedoraproject.org/en-US/quick-docs/getting-started-with-selinux/)
