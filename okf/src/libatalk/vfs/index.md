# src/libatalk/vfs

* [acl.c](acl.c.md) - Reducing ACLs to the entries that mirror the mode bits, in the VFS layer.
* [ea_ad.c](ea_ad.c.md) - Store Extended Attributes inside .AppleDouble folders as follows:
* [ea_sys.c](ea_sys.c.md) - The sys EA backend: AFP extended attributes in native filesystem EAs.
* [extattr.c](extattr.c.md) - Platform wrappers for extended attribute system calls, from Samba.
* [unix.c](unix.c.md) - Unix file operations of the VFS layer: modes, duplication, deletion, and renaming.
* [vfs.c](vfs.c.md) - VFS operation tables for the AppleDouble layouts and the EA and ACL layers.
