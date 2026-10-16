# src/etc/atalkd

* [aep.c](aep.c.md) - AppleTalk Echo Protocol replies of atalkd.
* [atserv.h](atserv.h.md) - AppleTalk service sockets and packet dispatch callback types.
* [config.c](config.c.md) - Reading and writing atalkd.conf, with the kernel interface list as the fallback.
* [gate.h](gate.h.md) - AppleTalk gateway and route list types for atalkd.
* [interface.h](interface.h.md) - AppleTalk interface state, flags and allocation interface.
* [list.h](list.h.md) - Doubly linked list node type for atalkd routing tables.
* [main.c](main.c.md) - The atalkd daemon: startup, interface setup, timers, and signals.
* [main.h](main.h.md) - Interface configuration helpers and daemon state for atalkd.
* [multicast.c](multicast.c.md) - Zone multicast addresses of AppleTalk interfaces.
* [multicast.h](multicast.h.md) - AppleTalk multicast membership and zone broadcast interface.
* [nbp.c](nbp.c.md) - NBP packet processing of atalkd.
* [nbp.h](nbp.h.md) - NBP registration table and packet handler interface for atalkd.
* [route.c](route.c.md) - Kernel route changes of atalkd through the route socket.
* [route.h](route.h.md) - Kernel AppleTalk route update interface for atalkd.
* [rtmp.c](rtmp.c.md) - RTMP routing table maintenance of atalkd.
* [rtmp.h](rtmp.h.md) - RTMP routing table types, flags and packet handling interface.
* [zip.c](zip.c.md) - ZIP zone table maintenance of atalkd.
* [zip.h](zip.h.md) - ZIP zone table and zone discovery interface for atalkd.
