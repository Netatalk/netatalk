# CNID Backends

Unlike other protocols such as SMB or NFS,
the AFP protocol mostly refers to files and directories by ID rather than by path.
These IDs are called CNIDs (Catalog Node Identifiers).
A typical AFP request uses a directory ID and a filename,
for example: "server, please open the file named 'Test' in the directory with ID 167".
Mac aliases work by CNID, with a fallback to the absolute path in more recent AFP clients.
(This fallback applies only to the Finder, not to applications.)

Every file in an AFP volume must have a unique file ID,
and according to the AFP specification CNIDs must never be reused.
IDs are 32-bit numbers, and directories share the same pool,
so it is depleted after roughly 4 billion files and directories have been written.

What happens then depends on the backend.
The *dbd* backend stops allocating IDs, and no new files can be written to the volume.
The *sqlite* and *mysql* backends empty the CNID table, restart numbering,
and trigger all clients connected to the volume to reconnect
so that everyone uses the new IDs.
This keeps a long-lived volume writable, but it violates the spec
and breaks Mac aliases pointing at the old IDs.
Both backends log a warning as a volume approaches the ceiling,
so it can optionally be renumbered with the **dbd** utility beforehand,
during a maintenance window.

Netatalk maps IDs to files and directories in the host filesystem.
Several CNID backends are available and can be selected with the **cnid scheme** option in *afp.conf*.
A CNID backend is essentially a database storing ID ↔ name mappings.

For CNID backends that use a zero-configuration database,
the database files are located by default in a *netatalk/CNID* subdirectory
of your system's state directory path, e.g. */var/lib*.
You can change the state directory path with *-Dwith-statedir-path=PATH*
at compile time.

The **dbd** command-line utility can verify, repair, and rebuild the CNID database
of any backend, not just *dbd*.

> **NOTE:** Keep the following CNID-related considerations in mind:

- Don't nest volumes unless **vol dbnest = yes** is set.
- CNID backends are databases, so they make **afpd** a combined file
  server and database system.
- If the filesystem runs out of space,
  the database will become corrupted.
  You can work around this by using the **vol dbpath** option
  to store database files in a different location.
- Be careful with CNID databases on NFS-mounted volumes.
  Serving AFP volumes over NFS is already risky,
  and placing a database there compounds the risk of corruption.
  Use the **vol dbpath** option to store databases on a local disk
  if you must use NFS-mounted volumes.

The following sections describe the CNID backends included with Netatalk.
You can choose to build one or several of them at compile time.
Run *afpd -v* to see which backends are available and which one is the default.

## dbd

The "Database Daemon" backend is built on Berkeley DB.
Rather than accessing the database directly,
**afpd** processes communicate with one or more **cnid_dbd** daemon processes over a wire protocol.
The **cnid_dbd** daemon is responsible for database reads and updates.

It is deprecated and not recommended for new deployments.

## mysql

CNID backend using a MySQL or MariaDB server.
This is the most performant and scalable backend option,
and is recommended for large deployments with many concurrent users.

The SQL server must be provisioned by the system administrator,
and Netatalk configured to connect to it over a Unix socket or a network interface.

## sqlite

A fast and lightweight CNID database backend that uses the SQLite v3 embedded database engine.
It has a simpler architecture compared to *dbd* because the database is accessed directly by **afpd**,
which means that it does not require separate controller daemon processes.
And compared to *mysql*, it does not require a separate SQL server
to be set up and maintained by the system administrator.

The tradeoff is that SQLite's embedded database is not the most scalable
for concurrent write operations.
It is therefore recommended for deployments with up to 20 concurrent users,
or for system administrators who do not want to set up MySQL or MariaDB.
