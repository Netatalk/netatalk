# src/bin/nad

* [crc16.c](crc16.c.md) - CRC-16/XMODEM for BinHex and MacBinary archives.
* [crc16.h](crc16.h.md) - Incremental CRC-16/XMODEM checksum interface for nad.
* [ftw.c](ftw.c.md) - File tree walker from the GNU C Library for the nad commands.
* [ftw.h](ftw.h.md) - File tree traversal callbacks, flags and entry point for nad.
* [hqx.c](hqx.c.md) - BinHex 4.0 encoding and decoding for the nad archive commands.
* [hqx.h](hqx.h.md) - BinHex file, header and fork I/O interface.
* [macbin.c](macbin.c.md) - MacBinary encoding and decoding for the nad archive commands.
* [macbin.h](macbin.h.md) - MacBinary file, header and fork I/O interface.
* [megatron.c](megatron.c.md) - Conversion driver of the nad archive commands bin, hex, unbin, and unhex.
* [megatron.h](megatron.h.md) - Fork conversion options, Finder metadata and volume context types.
* [nad.c](nad.c.md) - The nad command: entry point and subcommand dispatch.
* [nad.h](nad.h.md) - Command entry points, volume helpers and shared state for nad.
* [nad_adouble.c](nad_adouble.c.md) - AppleDouble metadata and CAP name encoding for the nad archive and StuffIt commands.
* [nad_adouble.h](nad_adouble.h.md) - AppleDouble file and fork I/O interface for nad conversions.
* [nad_cp.c](nad_cp.c.md) - The nad cp command: copy entries and their metadata, assigning CNIDs resolved for the destination paths.
* [nad_find.c](nad_find.c.md) - The nad find command: search a volume for entries by name.
* [nad_ls.c](nad_ls.c.md) - The nad ls command: list entries with their Mac metadata.
* [nad_mkdir.c](nad_mkdir.c.md) - The nad mkdir command: create directories with CNIDs and metadata.
* [nad_mv.c](nad_mv.c.md) - The nad mv command: move entries with their CNIDs and metadata.
* [nad_rm.c](nad_rm.c.md) - The nad rm command: delete entries with their CNIDs and metadata.
* [nad_rmdir.c](nad_rmdir.c.md) - The nad rmdir command: delete empty directories with their CNIDs and metadata.
* [nad_set.c](nad_set.c.md) - The nad set command: change the Mac metadata of an entry.
* [nad_stuffit.c](nad_stuffit.c.md) - The nad sit and unsit commands: create and extract StuffIt archives.
* [nad_util.c](nad_util.c.md) - Volume, signal, and CNID helpers shared by the nad commands.
