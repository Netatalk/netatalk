# Search

Netatalk provides support for Spotlight-compatible search and indexing on macOS clients.
This allows users to search for files and metadata on Netatalk volumes in macOS.

> **Note:** The Netatalk development team has observed that in macOS,
remote file systems can be searched only through *Finder Search* and not through
the *Spotlight Search* widget in the menu bar. This is a limitation of macOS and not of Netatalk.
>
> Open a Finder window and use the search box in the top right corner, enter a search term,
and make sure to select the Network neighborhood or the specific Netatalk volume in the search scope options.

For Classic Mac OS clients and very early Mac OS X clients,
Netatalk provides support for the older search feature called Catalog Search,
which is used by the Sherlock app in Mac OS.

## Spotlight Search for macOS

Netatalk's Spotlight-compatible search can be controlled with the **spotlight** option.
Enabled by default, you can turn it on or off globally and per volume.

Spotlight query handling uses a pluggable backend architecture.
The AFP Spotlight RPC protocol layer is shared,
while query execution is delegated to a backend selected per volume.

You can select the backend with the volume option **spotlight backend**.
If not set, the default is **cnid**.

Example:

    [Global]
    spotlight = yes

    [Documents]
    path = /srv/afp/docs
    spotlight backend = cnid

    [Media]
    path = /srv/afp/media
    spotlight backend = xapian

    [Private]
    path = /srv/afp/private
    spotlight = no

Available Spotlight backends:

- **cnid**
  Uses *cnid_find()* against the CNID database for filename-oriented searches.
  It has no extra runtime dependencies beyond the *talloc* library and Netatalk's CNID support.

- **localsearch**
  Uses the LocalSearch/Tracker SPARQL backend for broader file contents and metadata indexing/search.
  This backend requires LocalSearch/Tracker, TinySPARQL, GLib, D-Bus, and related tooling.

- **xapian**
  Uses a per-volume Xapian index maintained by Netatalk for filename, plain-text content, and MIME type search.
  This backend requires xapian-core and libmagic.

The **xapian** backend stores a shared per-volume index. To avoid indexing
private file contents into that shared index, it indexes body text only for
regular files that are world-readable, readable by the indexing process,
plain-text-like, and below the backend's text size limit. Files readable to
a specific AFP user but not world-readable can still match filename and type
queries, but may not match content queries such as `kMDItemTextContent`.

### Building Spotlight backends

The Meson option **with-spotlight-backends** controls which Spotlight backends are built.
Supported values are **cnid**, **localsearch**, and **xapian**.

Example:

    meson setup build -Dwith-spotlight-backends=cnid,localsearch,xapian

The default build enables the selected backends when their dependencies are available.

### D-Bus daemon for localsearch

The *dbus-daemon* binary is required when using the **localsearch** backend.
Its path is determined at compile time by the dbus-daemon build system option.

If *dbus-daemon* is installed at a different path, use the global option
**dbus daemon** to specify it.
For example, for Solaris with Tracker from OpenCSW:

    dbus daemon = /opt/csw/bin/dbus-daemon

## Catalog Search for Classic Mac OS

Netatalk leverages the CNID database for fast filename search on shared volumes.
You can also control whether to use fast database search or the slower recursive directory scanning
through the **search db** option.
