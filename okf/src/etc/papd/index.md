# src/etc/papd

* [auth.c](auth.c.md) - UAM registration and loading for papd printer authentication.
* [comment.c](comment.c.md) - DSC comment state stack of the papd PostScript parser.
* [comment.h](comment.h.md) - PostScript comment parser callbacks and parser stack interface.
* [file.c](file.c.md) - Print job buffers of papd: lines, appended data, and status replies.
* [file.h](file.h.md) - Print job buffer types and spooling helpers for papd.
* [headers.c](headers.c.md) - DSC header comment handlers of papd.
* [lp.c](lp.c.md) - Interface to lpr system.
* [lp.h](lp.h.md) - Print job submission, queue control and spool I/O interface.
* [magics.c](magics.c.md) - PostScript job type detection and the papd job loop.
* [main.c](main.c.md) - The papd daemon: printers, NBP registration, and the accept loop.
* [ppd.c](ppd.c.md) - PPD file reading for the printer of a papd job.
* [ppd.h](ppd.h.md) - PostScript printer description feature and font lookup interface.
* [print_cups.c](print_cups.c.md) - CUPS printing backend of papd.
* [print_cups.h](print_cups.h.md) - CUPS printer discovery, status and job submission interface.
* [printcap.c](printcap.c.md) - routines for dealing with the terminal capability data base based on termcap
* [printcap.h](printcap.h.md) - Printcap entry lookup and capability parsing interface.
* [printer.h](printer.h.md) - Printer configuration and authentication state types for papd.
* [queries.c](queries.c.md) - PostScript query answers of papd.
* [session.c](session.c.md) - One PAP connection of papd: reading jobs and sending replies.
* [session.h](session.h.md) - PAP client session entry point for papd.
* [uam.c](uam.c.md) - UAM module loading and the server services that papd offers UAMs.
* [uam_auth.h](uam_auth.h.md) - Authentication module types and loading interface for papd.
