# src/etc/papd

* [auth.c](auth.c.md) - UAM registration and loading for papd printer authentication.
* [comment.c](comment.c.md) - DSC comment state stack of the papd PostScript parser.
* [comment.h](comment.h.md) - 1 function, 2 types.
* [file.c](file.c.md) - Print job buffers of papd: lines, appended data, and status replies.
* [file.h](file.h.md) - 1 type.
* [headers.c](headers.c.md) - DSC header comment handlers of papd.
* [lp.c](lp.c.md) - Interface to lpr system.
* [lp.h](lp.h.md) - 3 functions, includes 1 project header.
* [magics.c](magics.c.md) - PostScript job type detection and the papd job loop.
* [main.c](main.c.md) - The papd daemon: printers, NBP registration, and the accept loop.
* [ppd.c](ppd.c.md) - PPD file reading for the printer of a papd job.
* [ppd.h](ppd.h.md) - 2 types.
* [print_cups.c](print_cups.c.md) - CUPS printing backend of papd.
* [print_cups.h](print_cups.h.md) - 1 type.
* [printcap.c](printcap.c.md) - routines for dealing with the terminal capability data base based on termcap
* [printcap.h](printcap.h.md) - No functions or types.
* [printer.h](printer.h.md) - 1 type.
* [queries.c](queries.c.md) - PostScript query answers of papd.
* [session.c](session.c.md) - One PAP connection of papd: reading jobs and sending replies.
* [session.h](session.h.md) - includes 1 project header.
* [uam.c](uam.c.md) - UAM module loading and the server services that papd offers UAMs.
* [uam_auth.h](uam_auth.h.md) - interface between uam.c and auth.c
