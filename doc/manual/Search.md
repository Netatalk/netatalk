# Search and Indexing

Netatalk provides support for Spotlight-compatible search and indexing on Apple clients.
This allows users to search for files and metadata on Netatalk volumes using the Spotlight search feature in macOS.

For Classic Mac OS clients and very early Mac OS X clients,
Netatalk provides support for the older search feature called Catalog Search,
which is used by the Sherlock app.

## Indexed Search for Spotlight

You can enable Netatalk's Spotlight-compatible search and indexing
either globally or on a per-volume basis with the **spotlight = yes** option.

> ***WARNING:*** Once Spotlight is enabled for a single volume,
all other volumes for which Spotlight is disabled will not be searchable at all.

The **dbus-daemon** binary must be installed for the Spotlight feature.
The path to **dbus-daemon** is determined at compile time by the dbus-daemon build system option.

If **dbus-daemon** is installed at a different path,
use the global option **dbus daemon** to specify it.
For example, for Solaris with Tracker from OpenCSW:

    dbus daemon = /opt/csw/bin/dbus-daemon

## Catalog Search for Classic Mac OS

Netatalk performs Catalog Search for Classic Mac OS clients out of the box,
without any special configuration.

However it uses slow recursive directory scanning by default.
For better performance, you can enable the **search db = yes** option to allow Catalog Search
to query the CNID database instead of scanning directories.
