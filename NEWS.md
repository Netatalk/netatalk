Netatalk Changelog
==================

Changes in 4.4.0
----------------

* NEW: afpd: Added probabilistic dicache validation for significant I/O reduction, GitHub #2447, #2577
* NEW: afpd: Added a runtime option for controlling shell validity checks, GitHub #2439
* FIX: afpd: Hardened parameter validation and buffer overflow protection, GitHub #2436, GitHub #2442
* FIX: afpd: Improved afpstats init error handling; afpstats is no longer built on macOS, GitHub #2448
* FIX: atalkd: Added NULL checks to prevent a crash when accessing freed memory, GitHub #2426
* UPD: cnid: The SQLite CNID backend is now considered stable, GitHub #2432
* NEW: cnid: Implemented CNID find in both the SQLite and MySQL CNID backends, GitHub #2464, #2508, #2515
* NEW: distrib: Introduced a SELinux policy for Netatalk daemons, GitHub #2248
* UPD: distrib: Containerization files have been restructured under distrib/docker, GitHub #2482
* UPD: docker: The production container now builds with the SQLite CNID backend, GitHub #2458
* NEW: docs: Generate developer documentation using Doxygen, GitHub #2467, #2470, #2484, #2486
* NEW: docs: Added a Netatalk Code of Conduct and Contributing Guidelines, GitHub #2476, #2483
* FIX: docs: Enhanced code documentation and removed dead code throughout the codebase, GitHub #2485, #2492
* FIX: docs: Improved documentation for UAM ciphers and configuration options, GitHub #2477, #2459
* FIX: docs: Improved AppleTalk network layer diagrams and developer documentation, GitHub #2469, #2473
* FIX: docs: Improved and clarified testsuite man pages and documentation, GitHub #2460, #2591
* UPD: docs: Adapt libatalk DSI readme to a page in the developer docs, GitHub #2570
* UPD: docs: Restore historical changelogs from the ASUN fork to v1.5, GitHub #2579
* FIX: getzones: Fixed potential memory leaks and improved defensive memory management, GitHub #2421, #2441
* UPD: libatalk: Refactored the vfs module for improved memory safety, GitHub #2491
* FIX: meson: Error out if no writable CNID backend is selected for compilation, GitHub #2450
* FIX: meson: Improved Solaris compatibility for iconv parameter checks, GitHub #2415
* FIX: meson: Rename with-manual-install-dir option to with-docs-install-dir, now applied to all docs, GitHub #2471
* NEW: nad: The 'ad' binary has been renamed to 'nad' for disambiguation, GitHub #2440
* FIX: nad: Operate on shared volumes using any CNID backend with improved OS compatibility, GitHub #2461, #2506
* FIX: nad: protect against potential memory leak in xgetcwd(), GitHub #2563
* NEW: testsuite: Added argument '-E' to delete all files on test volumes between test executions, GitHub #2437, #2424
* UPD: testsuite: Enhanced dircache statistics log output and added log support to afp_lantest, GitHub #2433
* NEW: webmin: Added support for dircache tuning options, GitHub #2462
* FIX: webmin: Corrected description of fce ignore names format, GitHub #2465
* FIX: webmin: fallback to section name when volume name not set, GitHub #2586
* UPD: Updated the bstring library subproject to version 1.0.3, GitHub #2501

Changes in 4.3.2
----------------

* FIX: afpd: restore functionality of authentication over PAM with AD, GitHub #2402
* FIX: meson: check for init bins only if installing service, GitHub #2398
* FIX: meson: prefer mDNS over Avahi on Darwin systems, GitHub #2400
* UPD: subprojects: update bstring library to 1.0.2, GitHub #2406

Changes in 4.3.1
----------------

* FIX: afpd: Refactor the afpd version helptext for readability and brevity, GitHub #2367
* FIX: libatalk: remove macros, define interface for netddp_{close, recvfrom, sendto}, GitHub #2383
* FIX: rtmpqry: Fix code quality issues in rtmpqry and disambiguate comments, GitHub #2379
* NEW: testsuite: Added Linux IO monitoring to afp_lantest and refactored results display, GitHub #2354
* FIX: test: Consistently initialize buffers before use in afpd component tests, GitHub #2380
* FIX: docker: Bring back the RandNum password creation by default, GitHub #2338
* NEW: docker: Expose Classic Mac OS login message option, GitHub #2339
* FIX: distrib: substitute lockfile path in netatalkd init scripts for macOS, GitHub #2357
* NEW: docs: Create separate nbplkup and nbprgstr man pages and improve them, GitHub #2336
* UPD: docs: update logtypes list in afp.conf man page, GitHub #2364
* UPD: docs: Flesh out the afpstats.1 man page, GitHub #2389
* NEW: Introduce yaml code style guide and automatic formatting, GitHub #2351
* NEW: contrib: Add support for beautifying markdown to codefmt.sh, GitHub #2356

Changes in 4.3.0
----------------

* NEW: cnid: Experimental SQLite CNID backend, GitHub #1570
* UPD: cnid: Automatically create volume directory when starting up mysql CNID backend, GitHub #2267
* NEW: libatalk: Introduce bstring as subproject, link with shared library, and remove vendored bstring code, GitHub #2268
* UPD: afpd: Properly support LocalSearch and remove Tracker < v3 support, GitHub #2275
* REM: docker: Disable support for Spotlight compatible search, GitHub #2276
* REM: docker: Build production container without ACL support, GitHub #2309
* NEW: Add rtmpqry utility to allow querying of routing information from neighbour routers, GitHub #2178
* NEW: nbplkup: provide for script-friendly output, GitHub #2157
* NEW: nbplkup: allow specification of NBP operation and of destination address, GitHub #2162
* NEW: getzones: allow all ZIP query types to be generated (and sundry other enhancements), GitHub #2169
* UPD: macusers: Add support for macOS in the macusers script, GitHub #2206
* UPD: meson: Use single array option to control CNID backends to build, GitHub #2210
* UPD: meson: Build and run netatalk without Berkeley DB, GitHub #2265
* NEW: webmin: Create page for scanning and rebuilding CNID database, GitHub #2259
* NEW: testsuite: Test ProDOS Info Bit in FPGetFileDirParams, GitHub #2256
* NEW: testsuite: Add caching performance tests to afp_lantest, GitHub #2322
* FIX: testsuite: The ability to run test batches in afp_speedtest, GitHub #2332
* NEW: docs: Create separate man pages for each afptest tool, GitHub #2331
* FIX: Beautify C and meson code with astyle / muon, GitHub #2152
* FIX: Introduce Markdown style guide and reformat all sources, GitHub #2180
* FIX: Reformat Perl source code with perltidy, GitHub #2201
* FIX: Beautify shell scripts with shfmt, GitHub #2217

Changes in 4.2.4
----------------

* FIX: uams: Check for const pam_message member of pam_conv, GitHub #2196
       Makes it possible to build on Solaris 11.4.81 CBE
* FIX: meson: Avoid build error in incomplete Homebrew env, GitHub #2190
* UPD: meson: Build with Homebrew libraries is now opt-in, GitHub #2194
       To opt in to build against Homebrew, use -Dwith-homebrew=true
* UPD: docs: Improve afpd and macipgw man pages, GitHub #2155

Changes in 4.2.3
----------------

* FIX: Properly read from afp.conf file passed with -F parameter, GitHub #2150
* FIX: Read the appletalk option only when built with DDP, GitHub #2149
* UPD: Consistently return exit code 0 after daemon version info, GitHub #2151
* UPD: libatalk: MySQL query error log level is dropped to debug, GitHub #2143
* UPD: initscripts: Improvements to netatalk OpenRC init script, GitHub #2148
* FIX: meson: enhance iconv detection when cross compiling, GitHub #1921
* UPD: docs: Cross-platform friendly docs for CNID statedir, GitHub #2146

Changes in 4.2.2
----------------

* NEW: cnid: Create MySQL database automatically if needed, GitHub #2119
* UPD: meson: Use pandoc to build documentation when available, GitHub #2127
* UPD: meson: Generate the html manual with plain cmark, GitHub #2134
* NEW: docker: Support for the mysql CNID backend in container, GitHub #2116
* NEW: docker: Containerized netatalk webmin module, GitHub #1463
* NEW: docker: Introduce option to enable extension mapping, GitHub #2125
* NEW: docker: Introduce option for disabling Spotlight, GitHub #2128
* NEW: webmin: UI for editing of the extmap.conf file, GitHub #2129
* NEW: webmin: Introduce option for hiding service controls, GitHub #2133
* FIX: webmin: Correct handling of volume and preset names, GitHub #2130
* FIX: webmin: Treat uams_randnum.so as a standard UAM, GitHub #2131
* FIX: docs: More portable man page markdown source syntax, GitHub #2114
* FIX: docs: Properly build the localized html manual, GitHub #2136
* FIX: docs: Overhauled markdown styles of whole manual, GitHub #2138

Changes in 4.2.1
----------------

* NEW: meson: Introduce option to control state dir creation, GitHub #2070
       Introduces the with-statedir-creation boolean option, true by default
* NEW: meson: Option for controlling CUPS backend installation, GitHub #2071
       Introduces with-cups-pap-backend (boolean, default false)
       and with-cups-libdir-path (string)
* FIX: meson: Generate Unicode lookup table sources before use, GitHub #2072
* FIX: libatalk: Work around DSIWrite() bug in AppleShare Client 3.7.x,
       GitHub #2085
* FIX: libatalk: Restore cnid mysql pw option that had fallen off
       which makes the mysql backend usable again, GitHub #2112
* FIX: afpd: Don't lose extension mapping on macOS hosts, GitHub #2092
* FIX: afpd: Fall back to ea = none rather than ea = ad when
       the filesystem EA support check fails, GitHub #2103
* UPD: webmin: Print volume name + section name in volumes list, GitHub #2073
* FIX: webmin: Sort lists of index page items in alphabetical order,
       GitHub #2074
* FIX: webmin: Return to the correct index tab from other actions, GitHub #2075
* UPD: testsuite: Print a detailed test summary after spectest run, GitHub #2095
* UPD: testsuite: Break out separate FPGetExtAttr test module, GitHub #2104
* UPD: testsuite: Print usage helptext when running test binaries
       without params, GitHub #2111
* UPD: docs: Major additions to the afptest man page, GitHub #2100
* NEW: docs: bstring README with redistribution notes and LICENSE,
       GitHub #2077
* FIX: docs: Improve verbiage in signature and UUID man pages, GitHub #2084
* UPD: docs: Transition Compilation from manual chapter to readme, GitHub #2106
* UPD: docs: Reduce overlap between install chapter and install readme,
       GitHub #2107

Changes in 4.2.0
----------------

* NEW: Link with shared iniparser library instead of vendored one, GitHub #1948
       - Makes iniparser a mandatory dependency
       - Our own hacked iniparser is now removed, which has a few side effects
       - Volume section names are now case insensitive, forced to lower case
       - The include directive is no longer supported (for now)
* NEW: afpd: Introduce apf.conf 'volume name' Volume option, GitHub #1976
* NEW: afpd: Introduce 'server name' Global option in afp.conf, GitHub #1974
* NEW: docs: Convert documentation from XML to Markdown format,
       introducing cmark dependency instead of docbook-xsl, GitHub #1905
* NEW: docs: Generate local html manual with only core pages, GitHub #1969
* NEW: docker: Introduce dropbox mode option for guest access, GitHub #1981
* NEW: docker: New and improved env variable options including debug mode,
       GitHub #1977, #1979
* UPD: Control metadata settings with 'ea' solely,
       removing 'appledouble' option, GitHub #1983
* UPD: afpd: Use servername for ASP connections with hostname fallback,
       GitHub #1978
* UPD: afpd: Refactor FCE file skip logic, make comma the standard delineator,
       GitHub #1997
* UPD: libatalk: Use getaddrinfo() instead of deprecated gethostbyname(),
       GitHub #1934
* UPD: meson: Introduce with-unicode-data option to build case tables,
       GitHub #1928
* UPD: meson: Clean up obsoleted compatibility macros, GitHub #2035
* UPD: meson: Cross-platform crypt library detection, GitHub #2036
* UPD: Improve and harden the FCE listener app,
       rename it to fce_listen and install with Meson, GitHub #2063
* FIX: afpd: Register FCE file creation event when copying files, GitHub #2027
* FIX: afpd: Use getpwnam_shadow() for basic auth on OpenBSD, GitHub #2040
* FIX: libatalk: Use unspecified network stack by default on OpenBSD,
       GitHub #2044
* FIX: uams: Support for OpenBSD flavor crypt_checkpass()
       for password validation, GitHub #2037
* FIX: Fix ad cp loss of FinderInfo, GitHub #2058
* FIX: Fix for CNID error with ad mv utility, GitHub #2060
* FIX: Apply additional hardening to the Netatalk Metadata EA handling,
       GitHub #2059
* FIX: Avoid TOCTOU race conditions in libatalk code, GitHub #1938, #1936
* FIX: Fix high severity memory safety bugs, GitHub #1966
* FIX: Protect against memory leaks and out of bounds array access,
       GitHub #1989
* FIX: bstrlib: Protect against buffer overflow, null pointer dereference,
       GitHub #1987
* FIX: libatalk: Refactor vfs write_ea() to avoid TOCTOU race condition,
       GitHub #1965
* FIX: libatalk: Refactor vfs ea_open() to avoid TOCTOU race condition,
       GitHub #1964
* FIX: uams: Check account validity after calling pam_authenticate(),
       GitHub #1935
* FIX: uams: Validate PAM account after root auth in DHX2 UAM, GitHub #1937
* FIX: uams: Return properly when ClearTxt shadow password has expired,
       GitHub #2041
* FIX: getzones: do not attempt to bind to the address we're also sending to,
       GitHub #2051
* FIX: libatalk: Improved logging when charset conversion fails,
       GitHub #1952
* FIX: webmin: Add RandNum UAM option to Global config, GitHub #2047
* REM: Remove traces of unsupported LDAP SASL auth, GitHub #1925
* REM: Remove standards.h with macros that are defined by the build system,
       GitHub #1988
* REM: Eliminate obsoleted NO_REAL_USER_NAME capability flag macro,
       GitHub #2018
* REM: meson: Remove legacy IRIX XFS extended attributes API, GitHub #2052

Changes in 4.1.2
----------------

* UPD: meson: Look for shared Berkeley DB library in versioned subdir too,
       to detect the library in the MacPorts build system, GitHub #1909
* FIX: webmin: Redirect back to the originating module index tab
       when returning from actions, GitHub #1915
* FIX: webmin: Fix '-router' switch in Webmin atalkd module, GitHub #1943
* FIX: webmin: Fix a default value helptext string, GitHub #1946
* UPD: Add GPL v2 license grant to mysql CNID backend code, GitHub #1874

Changes in 4.1.1
----------------

* NEW: meson: Introduce with-bdb-include-path override option, GitHub #1908
* FIX: meson: Restore prioritized Berkeley DB detection, GitHub #1877
       Fixes a regression when building on Arch Linux.
* FIX: meson: Detect file command dynamically for NixOS, GitHub #1907
* FIX: meson: Remove libquota check that breaks NetBSD, GitHub #1900
* FIX: docs: Consolidate redundant CNID and encoding info, GitHub #1880
* FIX: afpd: Log an error when directory has invalid did, GitHub #1893
* FIX: macipgw: Don't crash when config file is missing, GitHub #1891
* FIX: macipgw: Disable default options in macipgw.conf, GitHub #1876
* UPD: macipgw: Print usage notes for the -f option, GitHub #1898
* FIX: Prevent a number of illegal null pointer calls, GitHub #1894

Changes in 4.1.0
----------------

* NEW: afpd: Add native metadata storage for macOS hosts, GitHub #1813
* FIX: afpd: Do not report old AFP versions when AppleTalk support
       is disabled, GitHub #1846
* REM: Remove 'start tracker' and 'start dbus' afp.conf options, GitHub #1848
* REM: Remove the running of AFP commands with root privileges, GitHub #1849
* FIX: libatalk: Loosen AppleDouble checks for macOS, GitHub #1829
* FIX: libatalk: Protect Netatalk metadata EA from tampering, GitHub #1855
* FIX: Refactor retreival of native FinderInfo EA on macOS hosts, GitHub #1858
* NEW: macipgw: Introduce a configuration file, GitHub #1852
* UPD: macipgw: Default port value for zip/ddp service, GitHub #1836
       This should get the gateway working on musl systems (OpenWrt)
* FIX: afppasswd: Safe password string handling, GitHub #1845
* NEW: meson: Introduce with-kerberos-path option for custom dependency path,
       which can be used for Heimdal compatibility, GitHub #1822
* UPD: meson: Define lockfiles through the Meson build system, GitHub #1850
       Meson's with-lockfile=PAth now points to the lockfile root
* UPD: meson: Detect lib paths within Homebrew build system, GitHub #1833
* FIX: meson: Correctly detect bundled iconv on OpenWrt, GitHub #1857
* UPD: meson: Link papd with cups only when cups is enabled, GitHub #1862
* UPD: initscripts: Disable fork safety workaround for macOS, GitHub #1810
* UPD: initscripts: Start in non-forking mode with launchd, GitHub #1859
* UPD: docs: Correct atalkd.conf documentation, GitHub #1818
* FIX: docs: Fixes for spelling and grammar, GitHub #1856
* UPD: docs: Clarify the behavior of the -d option for daemons, GitHub #1861
* NEW: testsuite: Introduce -X option for running on big-endian systems,
       specifically s309x, GitHub #1817
* FIX: testsuite: Cross-platform compatible file ID tests, GitHub #1826
* FIX: testsuite: Don't attempt unauthorized file renaming in Error tests,
       GitHub #1828
* FIX: testsuite: Clean up after execution of encoding test, GitHub #1832
* FIX: testsuite: Free memory after running tests, GitHub #1866
* FIX: testsuite: Improve memory management in lantest, GitHub #1868
* UPD: Rename apple_dump script to addump, GitHub #1811
* UPD: webmin: Restructure index page into three tabs, GitHub #1785
* UPD: docker: Bump base image to Alpine 3.21, GitHub #1842

Changes in 4.0.8
----------------

* UPD: Set resource max limit to 10240 on macOS, GitHub #1793
       Compatibility with older macOS hosts such as 10.15 Catalina.
* UPD: meson: Allow building papd without CUPS, GitHub #1774
       Activate the override with: -Dwith-cups=false
* UPD: meson: Favor openldap when building on macOS, GitHub #1792
       Avoids linking with macOS LDAP.Framework by default.
* UPD: meson: Improved libquota detection on FreeBSD and NetBSD, GitHub #1805
* FIX: meson: DocBook detection stops at first hit, GitHub #1800
       Detect xsl-stylesheets-nons with higher priority than xsl-stylesheets;
       -Dwith-docbook-path is now a hard override
* UPD: docs: Clarify D-Bus and GLib dependencies in the Install chapter,
       GitHub #1798 GitHub #1799
* FIX: docs: Document that DocBook XSL has to be non-namespaced, GitHub #1800
* FIX: testsuite: Retry logic for final cleanup step in test358, GitHub #1795

Changes in 4.0.7
----------------

* FIX: Remove bitrotted code in the bstring library, GitHub #1769
       This was a regression between netatalk 3.2 and 4.0.
* FIX: meson: Check for SunRPC function quota_open(), GitHub #1225
       This should enable build with quota on *BSDs.
* FIX: meson: *BSD compatible libwrap check, GitHub #1770
* NEW: meson: Add option with-manual=man_only
       which compiles and installs only troff pages, GitHub #1766
* NEW: meson: Option to specify path to perl runtime, GitHub #1776
* UPD: meson: Flip order of Berkeley DB version detection, GitHub #1771
       A more recent version of dbd is now prioritized over older ones.
* FIX: meson: Don't attempt to detect shadow passwords
       on *BSD and macOS, GitHub #1777
* FIX: meson: Configure dbus paths and config files only if dbus exists,
       GitHub #1773
* FIX: meson: Don't define spooldir when building without papd, GitHub #1786
* UPD: meson: Generate appendix XML sources via with-manual=www
       and allow custom manual install path with with-manual-install-path,
       GitHub #1781
       (This is useful primarily for project maintainers.)
* UPD: docs: Only compile and install appletalk documentation when
       with-appletalk=true, GitHub #1753
* UPD: docs: Overhaul of man page Synopsis sections, GitHub #1765
* UPD: docs: Refer to CONTRIBUTORS hosted on netatalk.io in man pages,
       GitHub #1767

Changes in 4.0.6
----------------

* FIX: Workaround for bug in AppleShare Client 3.7.4, GitHub #1749
       Only report support of AFP 2.2 and later to DSI (TCP) clients
       which shaves several bytes off the server response
       and lowers the chance of >512 byte FPGetSrvrInfo response.
* UPD: All AppleTalk daemons now take -v to print version info, GitHub #1745
* FIX: 'ad find' can take any kind of string, not just lowercase, GitHub #1751
* UPD: meson: Default to no init scripts if service management command
       not found, GitHub #1743
* FIX: Include config.h by relative path consistently (cleanup) GitHub #1746
* FIX: Remove duplicate header includes in MySQL CNID backend, GitHub #1748
* FIX: docs: Fix formatting of afppasswd man page, GitHub #1750
* FIX: webmin: Properly install netatalk-lib.pl, GitHub #1752

Changes in 4.0.5
----------------

* UPD: Distribute pre-generated Unicode table sources, GitHub #1724
       This reverts the previous change in v4.0.0 removing these sources.
       We retain the ability to regenerate them on the fly,
       if Unicode character database is found by the build system.
       Built with UnicodeData.txt version 16.0.
       This also removes hard Perl and Unicode dependencies.
* NEW: afpd: Fallback to new DSI icon when no icon defined, GitHub #1729
* FIX: atalkd: Don't send NBP Reply packets from the loopback interface,
       addressing side effect in Linux kernel 6.9+ GitHub #1734
* FIX: docs: Strip out linebreak escapes in Compile appendix, GitHub #1733
* FIX: docs: Remove straggler afp_encodingtest.1 man page alias, GitHub #1728
* FIX: macipgw: On MACIP_ASSIGN, prepopulate the newly-assigned IP address
       into the arp cache to avoid warning on Linux, GitHub #1727
* NEW: macipgw: Add command-line option to drop root privileges
       after the server has been started, GitHub #1727
* FIX: macipgw: Fix argument handling in main() for aarch64 compatibility,
       GitHub #1735
* FIX: webmin: Revert default dir detection to address
       critical regression bug, GitHub #1736
* FIX: testsuite: Exit tests with the Exclude flag early, GitHub #1737
* FIX: testsuite: Longer sleep time after file operation in test358,
       GitHub #1739
* FIX: testsuite: Make Utf8 tests big-endian safe, GitHub #1740

Changes in 4.0.4
----------------

* FIX: Fix loss of FinderInfo on resource fork creation with
       AppleDouble EA backend, GitHub #1702
* FIX: Remove remnants of obsoleted DEBUG compile time flag, GitHub #1696
       - Fixes compile time error on MUSL systems when building with AppleTalk
       - When building debug builds, the EBUG flag is now activated
       - Print build type in the Meson summary
* FIX: meson: Detect rresvport() function in system libraries, GitHub #1697
       - Local rresvport() code was previoulsy behind a broken MUSL flag
       - Enables building with AppleTalk on OpenWrt
* FIX: meson: Fix build fail with -Dwith-spotlight=false, GitHub #1715
* FIX: docker: Explicitly launch the cupsd daemon on startup, GitHub #1707
* NEW: docs: Create manual page for 'afptest' (testsuite) tools, GitHub #1695
* UPD: docs: Bring CONTRIBUTORS up to date, GitHub #1722
* UPD: testsuite: Consolidate afp_ls as a command in afparg, GitHub #1705
       - Add 'FPEnumerate dir' as an afparg command
       - Remove 'afp_ls' as a separate executable
* UPD: testsuite: Merge encoding test into spectest, GitHub #1716
       - Add 'Encoding' as a testset in the spectest
       - Rewrite the 'western' test to use Unicode for the same characters
       - Remove 'afp_encodingtest' as a separate executable
* UPD: testsuite: Collapse spectest into a single suite, GitHub #1713
       The testsuite grouping have been removed, and all spectests
       are in a single suite. The tier 2 tests are enabled with
       the -c option. The sleep and readonly tests can be run with
       the -f option.
* UPD: testsuite: Enable Color terminal output by default,
       and flip the -C option, GitHub #1708
* UPD: testsuite: Print a test summary for the spectest, GitHub #1708
* UPD: testsuite: Treat 'Not Tested' as a failure again, GitHub #1709
* FIX: testsuite: Use AFPopenLogin() for FPopenLoginExt() as bug workaround
       to enable testing of AFP 3.x connections, GitHub #1709
* UPD: testsuite: Install test data for test431 into the datadir, GitHub #1712
* FIX: testsuite: Workarounds for MUSL system calls default permissions,
       which enables the testsuite to run on Alpine Linux, GitHub #1682
* UPD: testsuite: Break down login testsuite into atomic tests, GitHub #1717
* UPD: testsuite: Use AFP 3.4 by default (previously: AFP 2.1), GitHub #1718
* UPD: testsuite: Use the Exclude flag to skip test that require setup,
       previously used to skip known buggy tests, GitHub #1720
* FIX: testsuite: Improvements to test setup, cleanup, and early failure

Changes in 4.0.3
----------------

* FIX: afpd: Limit FPGetSrvrInfo packet for AppleTalk clients, GitHub #1661
       This prevents errors with very old clients
       when many AFP options are enabled.
* FIX: Fix EOF error reporting in dsi_stream_read(), GitHub #1631
       This should prevent warnings such as:
       'dsi_stream_read: len:0, unexpected EOF'
* FIX: Fix regression when accessing the afpd UUID, GitHub #1679
       Resolves an error when running the 'ad' utilities.
* FIX: meson: Fix indexer path detection on meson 1.6, GitHub #1672
* FIX: meson: Fix PAM config directory detection, GitHub #1678
* FIX: meson: Shore up Unicode char table script error handling and detection,
       GitHub #1692
* FIX: initscripts: Remove redundant nbpunrgstr cleanup
       in atalkd systemd config, GitHub #1660
* NEW: docker: Containerized testsuite, GitHub #1649
* UPD: docker: Register the conventional NBP entities when starting up,
       GitHub #1653
* UPD: docker: Remove file/dir perm settings that were causing problems
* FIX: testsuite: Treat NOT TESTED spectest result as non-failure,
       GitHub #1663
* FIX: testsuite: Don't treat initial spectest.sh run as a failure,
       GitHub #1664
* UPD: testsuite: Reduce default log verbosity for better test reports,
       introducing two verbosity levels (-v, -V), GitHub #1665
* UPD: testsuite: Reposition the Exclude option (-x)
       to flag known failures with Netatalk 4.0
* UPD: testsuite: Install all test runners and utils, GitHub #1675
* FIX: testsuite: Link test executables with -rdynamic
       to allow sole test case runs with -f, GitHub #1690
* UPD: testsuite: Consolidate spectest into a single binary, GitHub #1693

Changes in 4.0.2
----------------

* NEW: Bring back Classic Mac OS 'legacy icon' option, GitHub #1622
* UPD: Spotlight: Support TinySPARQL/LocalSearch, GitHub #1078
* FIX: ad: Fix volume check for the AppleDouble toolsuite, GitHub #1605
       Check was failing if the 'ea = ad' option was set.
* FIX: meson: Refactor Berkley DB detection for robustness, GitHub #1604
* UPD: meson: Add localstatedir override option, GitHub #1608
* UPD: meson: Make the print spool dir FHS compliant, GitHub #1608
* UPD: docs: Improve Upgrade chapter, GitHub #1609
* UPD: docker: Use multistage build to optimize image size, GitHub #1620
* FIX: afpd: Cleanup unused, broken AFP over ASP code #1612
* FIX: papd: Correct PAPStatus string copy buffer length, GitHub #1576
* UPD: Make last CNID backend writable when built for tests, GitHub #1623
       This unblocks the integration tests that concern writing.
* NEW: Bundle and improve the afptest test suite, GitHub #1633
       Build with the new '-Dwith-testsuite' option.
* FIX: webmin: Make AppleTalk service control functional, GitHub #1636

Changes in 4.0.1
----------------

* UPD: Update license grant to reflect the retroactive rescission
       of U.C Berkeley clause 3, GitHub #1567
* FIX: meson: Don't always build AppleTalk utils with RPATH, GitHub #1568
* FIX: docs: Build the macipgw html manual page, GitHub #1569
* FIX: Explicitly import headers to appease gcc on Debian Sid, GitHub #1571
* UPD: docs: Install static redirect man pages for nbp tools, GitHub #1575
* FIX: meson: Missing xsltproc and docbook-xsl treated
       as non-fatal error, GitHub #1581
* UPD: docker: Build with optimizations, without debug symbols, GitHub #1584
* UPD: meson: In summary, list Webmin module under
       a new Add-ons section, GitHub #1586
* UPD: initscripts: Use launchctl bootstrap and
       enable directives for installing on macOS, GitHub #1583
* REM: Remove obsoleted netatalk-config script, GitHub #1587
* FIX: Change u_char data types to the portable uint8_t, GitHub #1590
* FIX: meson: Detect native Avahi before mDNS, GitHub #1591
* UPD: initscripts: Remove the redundant systemd Also directive, GitHub #1593
* UPD: docs: Flesh out the compile appendix
       and break down start steps, GitHub #1595
* FIX: Fix seg fault in ad set utility
       when not in a netatalk volume, GitHub #1597
* UPD: Update ad manual page to cover 'ad set' utility, GitHub #1599

Changes in 4.0.0
----------------

* NEW: Reintroduce AppleTalk / DDP support, GitHub #220
       Controlled with the new build system option '-Dwith-appletalk'.
       Revived daemons: atalkd, papd, timelord, a2boot
       Revived config files: atalkd.conf, papd.conf
       Revived utilities: aecho, getzones, nbplkup, nbprgstr, nbpunrgstr, pap,
       papstatus
* NEW: Bundle macipgw, the MacIP Gateway daemon by Stefan Bethke, GitHub #1204
* UPD: uams: All encrypted UAMs depend on Libgcrypt now, GitHub #1488, #1506
       This means we remove the bundled wolfSSL library.
       A big thanks to the wolfSSL team for all their support!
* FIX: uams: Remove unhelpful Libgcrypt version check, GitHub #1550
* REM: Remove the obsoleted PGP UAM, GitHub #1507
* NEW: Bundle, configure and install the Webmin module, GitHub #518
       Controlled with the new build system option '-Dwith-webmin'.
* UPD: Migrate afpstats from dbus-glib to GDBus, GitHub #666
       Special thanks to Simon McVittie for his help!
* BREAKING: Remove canned troff man pages from distribution, GitHub #460
       The build system now generates them on the fly.
       Introduces a build time dependency on DocBook XSL and xsltproc.
* BREAKING: Remove generated Unicode conversion tables, GitHub #1220
       Introduces a build time dependency on the UnicodeData.txt database.
* UPD: Detect host OS home dir and configure afp.conf on the fly, GitHub #1274
* UPD: meson: Autodetect init style for host OS, #1124
* UPD: meson: Allow building with multiple init styles, GitHub #1291
* NEW: meson: Introduce '-Dwith-readmes' option for installing additional docs.
       GitHub #1310
* REM: Remove the Autotools build system. Meson is now the only choice.
       GitHub #1213

Changes in 3.2.10
-----------------

* BREAKING: Install netatalk-dbus.conf into datadir by default, GitHub #1533
       Previously: sysconfdir. This can be overridden by the build system.
* FIX: uams: Correct shadow password length check for ClearTxt, GitHub #1528
* FIX: cnid_dbd: Set explicit max length of db_params to prevent potential
       buffer overflow, GitHub #694
* FIX: meson: Debugging was enabled by default causing tickles
       to not be sent out, GitHub #1514
* FIX: meson: Format afpd help text output to match autotools, GitHub #1499
* FIX: meson: Throw missing cracklib dictionary warning, GitHub #1495
* FIX: meson: Use a valid code sample for the TCP Wrappers check, GitHub #1491

Changes in 3.2.9
----------------

* UPD: Use the recommended command to import Solaris init manifest,
       GitHub #1451
* FIX: uams: Make sure the DHX2 client nonce is aligned appropriately,
       GitHub #1456
* FIX: uams: Fix DHCAST128 key alignment problem, GitHub #1464
* FIX: wolfssl: OpenSSL coexistence tweaks, GitHub #1469
* FIX: docs: Remove straggler path substitution in afp.conf, GitHub #1480

Changes in 3.2.8
----------------

* UPD: Bump bundled WolfSSL library to stable version 5.7.2, GitHub #1433
       Resolves CVE-2024-1544, CVE-2024-5288, CVE-2024-5991, CVE-2024-5814
* UPD: Revert local modifications to the bundled WolfSSL library, GitHub #1432
* FIX: Enable building against a shared WolfSSL 5.7.2 library, GitHub #1421
* FIX: meson: Do not define rpath with a linker argument, GitHub #1443

Changes in 3.2.7
----------------

* NEW: meson: Ability to control the run-time linker path config file,
       GitHub #1396
       New boolean Meson option: '-Dwith-ldsoconf'
       When set to false, do not create /etc/ld.so.conf.d/libatalk.conf
* BREAKING: meson: Enable rpath by default, while disabling ldsoconf
       by default, GitHub #1417
* FIX: meson: Allow ldconfig to run unprivileged during setup, GitHub #1407
* FIX: docker: Add entry script step to clean up any residual lock file,
       GitHub #1412
* NEW: docker: Ship a docker-compose.yml sample file, GitHub #1414
* NEW: docker: Check for AFP_USER and AFP_PASS when launching container,
       GitHub #1415

Changes in 3.2.6
----------------

* BREAKING: meson: Refresh the dynamic linker cache when installing on Linux,
       GitHub #1386
       This fixes the issue of the libatalk.so shared library not being found
       when configuring with a non-standard library path, e.g. /usr/local/lib .
       New Meson option '-Dwith-install-hooks' controls this behavior,
       allowing you to disable the install hook in non-privileged environments.
       On Linux systems with glibc, we now install the following config file:
       /etc/ld.so.conf.d/libatalk.conf
* BREAKING: meson: Introduce option to control which manual l10n to build,
       GitHub #1390
       New Meson option '-Dwith-manual-l10n' default to empty, can be set to
       'ja' to build the Japanese localization of the html manual.
       This changes the default behavior of the build system
       to not build the Japanese html manual by default.
* BREAKING: meson: Install htmldocs into htmldocs subdir, GitHub #1391
       Previously, the html manual files were installed into the root
       of the netatalk doc directory. Now they are put under netatalk/htmldocs .
* BREAKING: meson: Use modern linker flag for rpath, remove dtags override,
       GitHub #1384
       When configuring with '-Dwith-rpath=true' the linker flags
       '-Wl,-rpath,' will be prepended instead of the old '-R' flag.
       On Linux platforms, we no longer prepend '-Wl,\-\-enable-new-dtags',
       either.

Changes in 3.2.5
----------------

* BREAKING: meson: Allow choosing shared or static libraries to build,
       GitHub #1321
       In practice, only shared libraries are built by default now.
       Use the 'default_library' option to control what is built.
* FIX: meson: Control the MySQL CNID backend, and support MariaDB, GitHub #1341
       Introduces a new boolean 'with-cnid-mysql-backend' option.
* FIX: meson: Implement with-init-dir option, GitHub #1346
* FIX: autotools/meson: Install FreeBSD init script into correct location,
       GitHub #1345
* FIX: meson: Fix syntax error with libiconv path, GitHub #1279
* FIX: meson: Correct description for with-manual option, GitHub #1282
* FIX: meson: Correct prefix lookup for tracker-control, GitHub #1284
* FIX: meson: default OPEN_NOFOLLOW_ERRNO overwrites platform customization,
       GitHub #1286
* FIX: meson: Don't make dtags depend on rpath, GitHub #1293
* FIX: meson: Remove duplicate dependency check for posix threads, GitHub #1297
* FIX: meson: Better output when cryptographic UAMs aren't built, GitHub #1302
* FIX: meson: Prioritize tests and run single-threaded to avoid race condition,
       GitHub #1312
* FIX: meson: Better way to handle rpath executable targets, GitHub #1315
* FIX: meson: Refactor libcrypto check and print better status messages,
       GitHub #1299
* FIX: meson: Look for libmariadb dependency to appease Fedora, GitHub #1348
* FIX: meson: Declare have_atfuncs globally to avoid failure later, GitHub #1357
* FIX: meson: Do a compiler sanity check before header checks, GitHub #1356
* FIX: Avoid using reserved keyword to build the tests on NetBSD, GitHub #1328

Changes in 3.2.4
----------------

* UPD: autotools: Restore ABI versioning of libatalk,
       and set it to 18.0.0, GitHub #1261
* UPD: meson: Define long-form soversion as 18.0.0, GitHub #1256
       Previously, only '18' was defined.
* NEW: meson: Introduce pkgconfdir override option, GitHub #1241
       The new option is called 'with-pkgconfdir-path'
       and is analogous to the 'with-pkgconfdir' Autotools option.
       Additionally, the hard-coded "netatalk" path suffix has been removed.
* NEW: meson: Introduce 'debian' init style option
       that installs both sysv and systemd, GitHub #1239
* FIX: meson: Add have_atfuncs check,
       and make dtags dependent on rpath flag, GitHub #1236
* FIX: meson: Correct overwrite install logic for config files, GitHub #1253
* FIX: Fix typo in netatalk_conf.c log message

Changes in 3.2.3
----------------

* UPD: Record note of permission to upgrade CNID code
       to a later GPL, GitHub #1194
* REM: Remove long-obsoleted cnid2_create script, GitHub #1203
* UPD: docker: Add option to enable ClearText and Guest UAMs, GitHub #1202
* FIX: docs: Standardize reference entry naming
       for netatalk-config man page, GitHub #1208
* FIX: meson: Generate afppasswd manual html page, GitHub #1210
* REM: meson: Remove obsolete 64 bit library check, GitHub #1207
* FIX: meson: Enable rpath for binaries
       only when with-rpath is enabled, GitHub #1214
* FIX: meson: Require kerberos before enabling krb5 UAM,
       not just GSSAPI, GitHub #1218
* FIX: meson: Restore linking with 64-bit libdb on Solaris, GitHub #1222
* FIX: meson: Fixing linking when building with
       the 'with-ssl-override' option, GitHub #1227

Changes in 3.2.2
----------------

* UPD: meson: Use external SSL dependency to provide cast header, GitHub #1186
       This reintroduces OpenSSL/LibreSSL as a dependency for the DHX UAM,
       while removing all source files with the SSLeay copyright notice.
* UPD: meson: Add option to override system WolfSSL
       with embedded WolfSSL: 'with-ssl-override', GitHub #1176
* REM: Remove obsolete Red Hat Upstart and SuSE SysV init scripts, GitHub #1163
* FIX: meson: Fix errors in PAM support macro, GitHub #1178
* FIX: meson: Fix perl shebang substitution in cnid2_create script, GitHub #1183
* FIX: meson: Fix operation of D-Bus path macros, GitHub #1182
* FIX: meson: Fix errors in shadow password macro, GitHub #1192
* FIX: autotools: gcc 8.5 expects explicit library flags
       for libgcrypt, GitHub #1188
* NEW: Create a security policy, GitHub #1166

Changes in 3.2.1
----------------

* FIX: CVE-2024-38439,CVE-2024-38440,CVE-2024-38441: Harden user login,
       GitHub #1158
* BREAKING: meson: Rework option semantics and feature macros, GitHub #1099
       - Consistent syntax of the build options to make them user-friendly
       - Standardises the syntax of the feature macros
       - Fixes the logic of the largefile support macro
       - Disables gssapi support if the Kerberos V UAM is not required
       - All options are now defined either as 'with-\*' or 'with-\*-path'
       - Please see the Release Notes for a full list of changed options
* UPD: meson: Enable building with system WolfSSL library, GitHub #1160
       - Build system will attempt to detect
       that all required headers and symbols are supported
       - Falls back to the bundled WolfSSL library
* FIX: meson: Fix -Doption paths on systems
       where rpath is enabled by default, GitHub #1053
* FIX: meson: Fix library search macro on OmniOS hosts, GitHub #1056
* FIX: meson: Fix rules for installing scripts, GitHub #1070
       - Install afpstats only when Perl is detected
       - Don't install scripts only used by netatalk developers
* FIX: meson: set setuid bit to allow user afppasswd changing, GitHub #1071
* FIX: meson: Fix logic of libiconv detection macro, GitHub #1075
* FIX: meson: Address various issues with the meson build system, GitHub #1082
       - Enables quota support on all flavours of linux and BSD, plus macOS
       - Adds the quota provider to the configuration summary
       - Adds a user option to disable LDAP support
       - Sets dependencies according to user configuration
       - Improves the syntax of the ACL macro
* FIX: meson: Further refinements to meson build system, GitHub #1086
       - Adds user options to disable cracklib and GSSAPI support
       - Automates Berkeley DB library detection on macOS
* FIX: meson: Fix issues with quota support on linux and macOS, GitHub #1092
       - Enables quota support on macOS hosts
       - Restores missing configuration option for linux hosts
       - Removes obsolete quota configuration data for linux and macOS hosts
* FIX: meson: Set executable flags when installing scripts, GitHub #1117
* UPD: autotools and meson: Use pkg-config to find libgcrypt, GitHub #1132
       - This removes dependency on the now-obsolete libgcrypt-config
* FIX: Use portable linux macro in etc/afpd header, GitHub #1083
* UPD: Debian Trixie expects systemd scripts in /usr/lib, GitHub #1135
* UPD: Add copyright for mac_roman.h, GitHub #1137
* FIX: Cleanup of copyright headers to make them scanner friendly, GitHub #1142
* REM: Remove unused atalk/talloc.h header, GitHub #1154
* FIX: docker: Don't bail out when password is longer than 8 chars, GitHub #1067
* UPD: docker: Bump to Alpine 3.20 base image, GitHub #1111
* FIX: docker: Rework AFP user's GROUP and GID settings, GitHub #1116
       - GID now requires GROUP to be set, and applies to that group
         rather than that of the user.
* UPD: docs: Indicate license for software package,
       and add SSLeay notice, GitHub #1125
* FIX: docs: Rephrase tarball section of manual, GitHub #1164

Changes in 3.2.0
----------------

* NEW: Introduce the Meson build system, GitHub #707
       GNU Autotools is still supported, but will be removed
       in a future release. See the newly added INSTALL file.
* NEW: Bundle WolfSSL for DHX/RandNum UAM encryption, GitHub #358
       This is enabled by default, controlled by option "-Dwith-embedded-ssl"
       Requires the Meson build system.
       External OpenSSL 1.1 and LibreSSL are still supported.
* BREAKING: LDAP API bump, OpenLDAP v2.3 or later required, GitHub #762
       afp.conf option "ldap server" has been replaced with "ldap uri"
       and has a new syntax. See the manual for details.
* REM: Remove legacy cdb and tdb CNID backends, GitHub #508
* REM: Remove Andrew File System (AFS) support, GitHub #554
* REM: Remove bundled talloc, GitHub #479
       For Spotlight support, use the talloc library supplied by your OS,
       or get the source code from the Samba project and build it yourself.
* BREAKING: Remove generated SPARQL code, GitHub #337
       This introduces a compile time dependency on
       a yacc parser and a lexer to build with Spotlight support.
* BREAKING: Rename macOS launchd plist to io.netatalk.*, GitHub #778
       Note: Only the Meson build system will clean up the old plist.
* BREAKING: Renamed Gentoo init script to openrc, GitHub #868
       OpenRC is cross platform; confirmed working on Alpine Linux.
* NEW: FreeBSD init script, borrowed from FreeBSD ports, GitHub #876
       Special thanks to the author, Joe Marcus Clarke.
* NEW: OpenBSD init script, GitHub #870
* NEW: Introduce an official Dockerfile and entry script, GitHub #713
* NEW: Option to log to file with second (not us) accuracy, GitHub #580
       Enable with afp.conf option: "log microseconds = no"
* NEW: Option to add delay to FCE event emission, GitHub #849
       Set a ms delay with afp.conf option: "fce sendwait"
* NEW: afppasswd: Add -w option to set password from the CLI, GitHub #936
* NEW: docs: Distribute a manual appendix with the GNU GPL v2, GitHub #745
* NEW: docs: Distribute the Japanese localization of the manual, GitHub #806
* NEW: docs: Generate a manual appendix with build instructions, GitHub #791
       The appendix is generated from the GitHub CI workflow yaml file.
* UPD: docs: Document libraries, init scripts in manual, GitHub #808
* UPD: docs: Remove substituted file system paths from manual, GitHub #514
* FIX: afpd: Prevent theoretical crash in FPSetACL, GitHub #364
* FIX: libatalk: Fix parsing of macOS-created AppleDouble files, GitHub #270
* FIX: libatalk: Restore invalid EA metadata cleanup, GitHub #400
* FIX: quota: Use the NetBSD 6 quota API, GitHub #1028
* FIX: quota: Workaround for rquota.h symbol name on Fedora 40, GitHub #1040
* FIX: uams: Allow linking of the PGP UAM, GitHub #548
* FIX: Shore up error handling and type safety, GitHub #952
* UPD: Rewrite the afpstats script in Perl, GitHub #893
       And, improve the formatting of the standard output.
       Requires the Net::DBus Perl extension.
       This removes the effective dependency on a Python runtime.
* UPD: Make Perl and grep optional requirements, GitHub #886
       When either is missing, do not install the optional Perl scripts.
* NEW: Build system option "disable-init-hooks", GitHub #796
       Will skip init script enablement commands that require
       elevated privileges on the system.
* FIX: Make cracklib macro properly detect dictionary, GitHub #940
* FIX: Build with PAM support on FreeBSD 14, GitHub #560
* FIX: Allow libevent2 linking on OpenIndiana, GitHub #512
* FIX: Control all Spotlight dependencies at compile time, GitHub #571
* REM: Remove redundant AUTHORS file, GitHub #538

Changes in 3.1.18
-----------------

* FIX: CVE-2022-22995: Harden create_appledesktop_folder(), GitHub #480
* FIX: Disable dtrace support on aarch64 FreeBSD hosts, Github #498
* FIX: Correct syntax for libwrap check in tcp-wrappers.m4, GitHub #500
* FIX: Correct syntax for libiconv check in iconv.m4, GitHub #491
* FIX: quota is not supported on macOS, GitHub #492

Changes in 3.1.17
-----------------

* FIX: CVE-2023-42464: Validate data type in dalloc_value_for_key(), GitHub #486
* FIX: Declare a variable before using it in a loop,
       which was throwing off the default compiler on RHEL7, GitHub #481
* UPD: Distribute tarballs with xz compression by default, not gzip, GitHub #478
* UPD: Add AUTHOR sections to all man pages with a reference to CONTRIBUTORS,
       and standardize headers and footers, GitHub #462

Changes in 3.1.16
-----------------

* FIX: libatalk: Fix CVE-2022-23121, CVE-2022-23123 regression
       - Added guard check before access ad_entry(), GitHub#357
       - Allow zero length entry, for AppleDouble specification, GitHub#368
       - Remove special handling for COMMENT entries, GitHub#236
       - The assertion for invalid entires is still enabled,
         so please report any future "Invalid metadata EA" errors!
* FIX: build system: Fix autoconf warnings and modernize bootstrap
       and configure.ac, GitHub#331
* FIX: build system: Correct syntax in libevent search macro,
       summary macro and netatalk executable makefile, GitHub#342
* FIX: build system: Fix native libiconv detection on macOS, GitHub#343
* FIX: build system: Use non-interactive PAM session when available, GitHub#361
* FIX: build system: Fix detection of Berkeley DB installed
       in multiarch location, GitHub#380
* FIX: build system: Fix support for cross-compilation
       with mysql_config and dtrace, GitHub#384
* FIX: build system: Support building quota against libtirpc, GitHub#385
* FIX: build system: Fix variable substitution in configure summary, GitHub#443
* REM: build system: Remove ABI checks and the \-\-enable-developer option, GitHub#262
* FIX: initscript: Improvements to Debian SysV init script
       - Source init-functions, GitHub#386
       - Add a Description and Short-Description, GitHub#428
* FIX: docs: Clarify localstate dir configurability in manual, GitHub#401
* UPD: docs: Make BerkeleyDB 5.3.x the recommended version, GitHub#8
* FIX: docs: Update SourceForge URLs to fix CSS styles and download links
* FIX: docs: Remove obsoleted bug reporting sections, GitHub#455
* FIX: Sundry typo fixes in user visible strings and docs, GitHub#381, GitHub#382
* UPD: Rename asip-status.pl as asip-status
       to make naming implementation-agnostic, GitHub#379
* UPD: Remove redundant uid.c|h files in etc/afpd
* UPD: Don't build and distribute deprecated cnid2_create tool, GitHub#412
* REM: Remove deprecated megatron code and man page, GitHub#456
* REM: Remove deprecated uniconv code and man page, GitHub#457
* UPD: Improvements to the GitHub CI workflow

Changes in 3.1.15
-----------------

* FIX: CVE-2022-43634
* FIX: CVE-2022-45188
* NEW: Support for macOS hosts, Intel and Apple silicon, GitHub#281
* FIX: configure.ac: update deprecated autoconf syntax
* UPD: configure.ac: Support linking with system shared libraries
       Introduces the \-\-with-talloc option
* FIX: macros: largefile-check macro for largefile (clang 16)
* UPD: macros: Update pthread macro to the latest from gnu.org
* FIX: initscripts: Modernize Systemd service file.
* FIX: libatalk/conf: include sys/file.h for LOCK_EX
* FIX: libatalk: Change log level for realpath() error, SF bug#666
* FIX: libatalk: Change log level for real_name error, SF bug#596
* FIX: libatalk: The my_bool type is deprecated as of MySQL 8.0.1, GitHub#129
* UPD: libatalk: allow afpd to read read-protected afp.conf, SF bug#546
* UPD: libatalk: Make the "valid users" option work in the Homes section, SF bug#449
* UPD: libatalk: Check that FPDisconnectOldSession is successful, SF bug#634
* UPD: libatalk: Bring iniparser library codebase in line with current version 4.1
* FIX: afpd: Provide MNTTYPE_NFS on OmniOS to make quota work, GitHub#117
* FIX: afpd: Avoid triggering realpath() lookups with empty path, GitHub#277
* FIX: spotlight: Spotlight searches can cause afpd to segfault, GitHub#56
* UPD: spotlight: add support for tracker3, SF patch#147
* FIX: macusers: Fix output for long usernames
* FIX: macusers: account for usernames with non-word characters
* FIX: macusers: Support NetBSD
* FIX: Fix all function declarations without a prototype
* FIX: Fix C99 compliance issues
* FIX: Fix gcc10 compiler warnings
* REM: Remove acsiidocs sources and release notes script
* FIX: manpages: afp.conf: Parameters are not quoted, SF bug#617
* FIX: manpages: afp.conf: Document $u in home name, GitHub#123
* FIX: manpages: afp.conf: Document the usage of guest user, GitHub#298
* FIX: Document how the mysql cnid backend is configured, GitHub#69
* FIX: Fix user-visible typos in log output and man pages.
* FIX: Fix spelling, syntax, and dead URLs in html manual.
* NEW: Create README.md
* NEW: Set up GitHub workflow and static analysis with Sonarcloud

Changes in 3.1.14
-----------------

* FIX: fix build with libressl >= 2.7.0, GitHub#105
* NEW: Added Ignore Directories Feature
* UPD: Generate Unicode source code based on Unicode 14.0, GitHub#114
* FIX: Protect against removing AFP metadata xattr
* FIX: avoid setting adouble entries on symlinks
* FIX: add handling for cases where ad_entry() returns NULL, GitHub#175
* FIX: Fix setting of LD_LIBRARY_FLAGS ($shlibpath_var).
* FIX: afpstats: Fedora migrating away from IO::Socket::INET6, GitHub#130
* FIX: afpd: check return values from setXXid() functions, GitHub#115
* FIX: afpd: drop groups in become_user_permanently(), GitHub#126
* FIX: Fix use after free in get_tm_used()
* FIX: Fix sign extension problem in bsd_attr_list()
* FIX: Fix garbage read in bsd_attr_list
* FIX: make afpstats python 3 compatible
* UPD: docs: manual: Remove wrong TCP-over-TCP info; minor copy editing
* FIX: configure.ac: fix macro ordering for CentOS 6
* FIX: configure.ac: fix typo
* FIX: configure.ac: remove some trailing whitespace
* FIX: configure.ac: fix deprecated macro invocation
* FIX: configure.ac: replace obsolete macro
* FIX: libatalk/dsi/Makefile.am: fix deprecation warning
* FIX: Store AutoMake helper script in build-aux/
* FIX: configure.ac: define a dir for macros
* FIX: configure.ac: AM_CONFIG_HEADER is deprecated
* FIX: autotools: Fix another deprecation warning
* FIX: libgcrypt typo in configuration error message
* UPD: Various CI improvements
* FIX: libatalk/conf: re-generation of afp_voluuid.conf
* UPD: libatalk/conf: code cleanup and add locking to get_vol_uuid()
* UPD: add documentation for the lv_flags_t
* FIX: No need to check for attropen on Solaris, GitHub#44

Changes in 3.1.13
-----------------

* FIX: CVE-2021-31439
* FIX: CVE-2022-23121
* FIX: CVE-2022-23123
* FIX: CVE-2022-23122
* FIX: CVE-2022-23125
* FIX: CVE-2022-23124
* FIX: CVE-2022-0194
* FIX: afpd: make a variable declaration a definition
* REM: Remove bundled libevent

Changes in 3.1.12
-----------------

* FIX: dhx uams: build with LibreSSL, GitHub#91
* FIX: various spelling errors
* FIX: CVE-2018-1160

Changes in 3.1.11
-----------------

* NEW: Global option "zeroconf name", FR#99
* NEW: show Zeroconf support by "netatalk -V", FR#100
* UPD: gentoo: Switch openrc init script to openrc-run, GitHub#77
* FIX: log message: name of function doese not match, GitHub#78
* UPD: volume capacity reporting to match Samba behavior, GitHub#83
* FIX: debian: sysv init status command exits with proper exit code, GitHub#84
* FIX: dsi_stream_read: len:0, unexpected EOF, GitHub#82
* UPD: dhx uams: OpenSSL 1.1 support, GitHub#87

Changes in 3.1.10
-----------------

* FIX: cannot build when ldap is not defined, bug #630
* FIX: SIGHUP can cause core dump when mdns is enabled, bug #72
* FIX: Solaris: stale pid file puts netatalk into maintenance mode, bug #73
* FIX: dsi_stream_read: len:0, unexpected EOF, bug #633

Changes in 3.1.9
----------------

* FIX: afpd: fix "admin group" option
* NEW: afpd: new options "force user" and "force group"
* FIX: listening on IPv6 wildcard address may fail if IPv6 is
       disabled, bug #606
* NEW: LibreSSL support, FR #98
* FIX: cannot build when acl is not defined, bug #574
* UPD: configure option "\-\-with-init-style=" for Gentoo.
       "gentoo" is renamed to "gentoo-openrc".
       "gentoo-openrc" is same as "openrc".
       "gentoo-systemd" is same as "systemd".
* NEW: configure option "\-\-with-dbus-daemon=PATH" for Spotlight feature
* UPD: use "tracker daemon" command instead of "tracker-control" command
       if Gnome Tracker is the recent version.
* NEW: configure options "\-\-enable-rpath" and "\-\-disable-rpath" which
       can be used to force setting of RPATH (default on Solaris/NetBSD)
       or disable it.
* NEW: configure option "\-\-with-tracker-install-prefix" allows setting
       an alternate install prefix for tracker when cross-compiling.
* UPD: asip-status.pl: IPv6 support
* UPD: asip-status.pl: show GSS-UAM SPNEGO blob
* FIX: afpd: don't use network IDs without LDAP, bug #621
* FIX: afpd: reading from file may fail, bug #619
* NEW: AFP clients should not be able to copy or manipulate special
       extended attributes set by NFS and SMB servers on Solaris, issue #36
* FIX: ad: ad cp may crash, bug #622
* UPD: Update Unicode support to version 9.0.0

Changes in 3.1.8
----------------

* FIX: CNID/MySQL: Quote UUID table names.
       <https://sourceforge.net/p/netatalk/bugs/585/>
* FIX: Crash in cnid_metad, bug #593
* UPD: Update Unicode support to version 8.0.0
* FIX: larger server side copyfile buffer for improved IO performance,
       bug #599
* NEW: afpd: new option "ea = samba". Use Samba vfs_streams_xattr
       compatible xattrs which means adding a 0 byte at the end of
       xattrs.
* FIX: remove #541 workaround patch. There was this problem with only early
       Fedora 20.
* FIX: rpmbuild fails on Fedora x86_64, bug #598
* FIX: Listen on IPv6 wildcard address by default, bug #602
* FIX: FCE protocol version 1 packets, bug #603
* UPD: Update list of BerkeleyDB versions searched at configure time

Changes in 3.1.7
----------------

* UPD: Spotlight: enhance behaviour for long running queries, client
       will now show "progress wheel" while waiting for first results.
* FIX: netatalk: fix a crash on Solaris when registering with mDNS
* FIX: netatalk: SIGHUP would kill the process instead of being resent
       to the other Netatalk processes, bug #579
* FIX: afpd: Solaris locking problem, bug #559
* FIX: Handling of malformed UTF8 strings, bug #524
* FIX: afpd: umask handling, bug #576
* FIX: Spotlight: Limiting searches to subfolders, bug #581
* FIX: afpd: reloading logging config may result in privilege
       escalation in afpd processes
* FIX: afpd: ACL related error messages, now logged with loglevel
       debug instead of error
* FIX: cnid_metad: fix tsockfd_create() return value on error
* FIX: CNID/MySQL: volume table name generation, bug #566.

Changes in 3.1.6
----------------

* FIX: Spotlight: fix for long running queries
* UPD: afpd: distribute SIGHUP from parent afpd to children and force
       reload shares
* FIX: netatalk: refresh Zeroconf registration when receiving SIGHUP
* NEW: configure option "\-\-with-init-style=debian-systemd" for Debian 8 jessie
       and later.
       "\-\-with-init-style=debian" is renamed "\-\-with-init-style=debian-sysv".

Changes in 3.1.5
----------------

* FIX: Spotlight: several important fixes

Changes in 3.1.4
----------------

* FIX: afpd: Hangs in Netatalk which causes it to stop responding to
       connections, bug #572.
* NEW: afpd: new option "force xattr with sticky bit = yes|no"
       (default: no), FR #94
* UPD: afpd: FCE version 2 with new event types and new config options
       "fce ignore names" and "fce notify script"
* UPD: afpd: check for modified included config file, FR #95.
* UPD: libatalk: logger: remove flood protection and allocate messages
* UPD: Spotlight: use async Tracker SPARQL API
* NEW: afpd: new option "case sensitive = yes|no" (default: yes)
       In spite of being case sensitive as a matter of fact, netatalk
       3.1.3 and earlier did not notify kCaseSensitive flag to the client.
       Now, it is notified correctly by default, FR #62.

Changes in 3.1.3
----------------

* UPD: Spotlight: more SPARQL query optimisations
* UPD: Spotlight: new options "sparql results limit", "spotlight
       attributes" and "spotlight expr"
* FIX: afpd: Unarchiving certain ZIP archives fails, bug #569
* UPD: Update Unicode support to version 7.0.0
* FIX: Memory overflow caused by 'basedir regex', bug #567
* NEW: afpd: delete empty resource forks, from FR #92
* FIX: afpd: fix a crash when accessing ._ AppleDouble files created
       by OS X via SMB, bug #564
* FIX: afpd and dbd: Converting from AppleDouble v2 to ea may corrupt
       the resource fork. In some circumstances an offset calculation
       is wrong resulting in corrupt resource forks after the
       conversion. Bug #568.
* FIX: ad: fix for bug #563 broke ad file utilities, bug #570.
* NEW: afpd: new advanced option controlling permissions and ACLs,
       from FR #93

Changes in 3.1.2
----------------

* FIX: Option "vol dbpath" was broken in 3.1.1
* FIX: Spotlight: file modification date, bug #545
* FIX: Improve reliability of afpd child handler
* FIX: debian initscript: add 0 and 6 to Default-Stop. debian-bug#745520
* FIX: put the Solaris share reservation after our locking stuff, bug #560.
* UPD: Improve Linux quota behaviour
* FIX: xattrs on *BSD, bug #562
* NEW: afpd: support for using $u username variable in AFP volume
       definitions. FR#90.
* FIX: getvolbypath returns incorrect volume, bug #563
* FIX: fd leak when using appledouble = v2, bug #554
* UPD: New options that control whether dbus and Tracker are started:
       'start dbus' and 'start tracker', both default to yes, FR#91
* UPD: Spotlight: SPARQL query optimisations

Changes in 3.1.1
----------------

* FIX: Add asprint() compatibility function for systems lacking it
* FIX: Fix resource fork name conversion. Bug #534.
* FIX: Fix a bug where only the first configured UAM was loaded.
       Bug #537.
* UPD: Add support for AFP 3.4. From FR #85.
* FIX: Registering with mDNS crashed. Bug #540
* FIX: Saving from applications like Photoshop may fail, because
       removing the resource fork AppleDouble file failed. Bug #542.
* FIX: dbd: remove orphaned ._ AppleDouble files. Bug #549.
* NEW: afpd: Automatic conversion of ._ AppleDouble files
       created by OS X. Bug #550.
* FIX: afpd: Fix a crash in of_closefork(). Bug #551.
* FIX: dbd: Don't print message "Ignoring .\_file" for every .\_ file.
       Bug #552.
* FIX: afpd: Don't flood log with failed sys_set_ea() messages.

Changes in 3.1.0
----------------

* NEW: AFP Spotlight support with Gnome Tracker
* NEW: New option "spotlight" (G/V)
* NEW: Configure option \-\-with-tracker-pkgconfig-version
* NEW: Configure option \-\-with-tracker-prefix
* NEW: If Spotlight is enabled, launch our own dbus instance
* NEW: New option "dbus daemon" (G)
* UPD: Add configure option \-\-with-afpstats for overriding the
       result of autodetecting dbus-glib presence
* NEW: Add recvfile support with splice() on Linux. New global options
       "recvfile" (default: no) and "splice size" (default 64k).
* NEW: CNID backend "mysql" for use with a MySQL server

Changes in 3.0.7
----------------

* FIX: Build fixes for the Kerberos UAM
* UPD: Use dedicated exit code for AFP connections that were dropped
       by the client right after the TCP handshake
* FIX: Workaround for a problem which cannot be advertized by Avahi. Bug #541.
* FIX: Registering with mDNS crashed. Bug #540
* FIX: Saving from applications like Photoshop may fail, because
       removing the resource fork AppleDouble file failed. Bug #542.
* FIX: macusers showed root user. Bug #495.
* UPD: Add file pathname to logmessage parse_entries: bogus eid. FR#87.

Changes in 3.0.6
----------------

* FIX: charset conversion failed when copying from Mac OS 9. Bug #523.
* UPD: Don't force S_ISGID for directories on FreeBSD. Bug #525.
* NEW: Add support for ZFS ACLs on FreeBSD with libsunacl. From FR#83.
* FIX: Active Directory LDAP queries for ACL support with new options
       "ldap user filter" and "ldap group filter". Bug #526.
* NEW: Option "vol dbnest", when set to true, the CNID database for
       a volume is stored in the volume root of a share in a directory
       .AppleDB like in Netatalk 2. Defaults to false. From FR#84.
* FIX: Small fix in the DSI tickle handling. Bug #528.
* UPD: Enhance handling of connection attempts when hitting the
       connection limit. Bug #529.
* FIX: Saving from Word to a folder that is a symlink to a folder on
       another filesystem results in a crash of the afpd process and
       the save to fail. This happens only if the option
       "follow symlinks" is enabled. Bug #532.
* FIX: Disable Kerberos UAM if AFP service principal name can't be
       evaluated. Fixes bug #531.
* FIX: Fix handling of large number of volumes. Bug #527.
* NEW: Configure option \-\-with-tbd which can be used to disable the
       use of the bundled tdb and use a system installed version.

Changes in 3.0.5
----------------

* FIX: Fix a crash when using pam_winbind. Fixes bug #516.
* NEW: New global/volume option "ignored attributes"
* FIX: "afp listen" option failed to take IPv6 addresses. Bug #515.
* FIX: Fix a possible crash in set_groups. Bug #518.
* NEW: Send optional AFP messages for vetoed files, new option
       "veto message" can be used to enable sending messages.
       Then whenever a client tries to access any file or directory
       with a vetoed name, it will be sent an AFP message indicating
       the name and the directory. From FR #81.
* NEW: New boolean volume option "delete veto files". If this option is
       set to yes, then Netatalk will attempt to recursively delete any
       vetoed files and directories. FR #82.
* UPD: systemd unit dir is /usr/lib/systemd/system .
* FIX: Saving files from application like MS Word may result in the file
       loosing metadata like the Finder label. Bug #521.

Changes in 3.0.4
----------------

* FIX: Opening files without metadata EA may result in an invalid
       metadata EA. Check for malformed metadata EAs and delete them.
       Fixes bug #510.
* FIX: Fix an issue with filenames containing non-ASCII characters that
       lead to a failure setting the size of a files resource fork.
       This affected application like Adobe Photoshop where saving
       files may fail. Fixes bug #511.
* UPD: Enhance ACL mapping, change global ACL option 'map acls' to take
       the following options: "none", "rights" (default), "mode".
       none   = no mapping, this resembles the previous false/no setting
       rights = map ACLs to Finder UARights, this resembles the previous
                true/yes setting. This is the default.
       mode   = map ACLs to Finder UARights and UNIX mode
       From FR #73.
* FIX: Fix a possible crash in cname() where cname_mtouname calls
       dirlookup() where the curdir is freed because the dircache
       detected a dev/inode cache difference and evicted the object
       from the cache. Fixes bug #498.
* FIX: Add missing include, fixes bug #512.
* FIX: Change default FinderInfo for directories to be all 0, fixes
       bug 514.
* NEW: New option "afp interfaces" which allows specifying where
       Netatalk listens for AFP connections by interface names.
       From FR #79.

Changes in 3.0.3
----------------

* UPD: afpd: Increase default DSI server quantum to 1 MB
* UPD: bundled libevent2 is now static
* NEW: \-\-with-lockfile=PATH configure option for specifying an
       alternative path for the netatalk lockfile.
* UPD: systemd service file use PIDFile and ExecReload.
       From FR #70.
* UPD: RedHat sysvinit: rm graceful, reimplement reload, add condrestart
* FIX: Couldn't create folders on FreeBSD 9.1 ZFS fileystems.
       Fixed bug #491.
* FIX: Fix an issue with user homes when user home directory has not the
       same name as the username.
       Fixes bug #497.
* UPD: Fix PAM config install, new default installation dir is
       $sysconfdir/pam.d/. Add configure option \-\-with-pam-confdir
       to specify alternative path.
* NEW: AFP stats about active session via dbus IPC. Client side python
       program 'afpstats'. Requires dbus, dbus-glib any python-dbus.
       configure option \-\-dbus-sysconf-dir for specifying dbus
       system security configuration files.
       New option 'afpstats' (default: no) which determines whether
       to enable the feature or not.
* NEW: configure option \-\-with-init-dir
* NEW: dtrace probes, cf include/atalk/afp_dtrace.d for available
       probes.
* UPD: Reload groups when reloading volumes. FR #71.
* FIX: Attempt to read read-only ._ rfork results in disconnect.
       Fixes bug #502.
* FIX: File's resource fork can't be read if metadata EA is missing.
       Fixes bug #501.
* FIX: Conversion from adouble v2 to ea for directories.
       Fixes bug #500.
* FIX: Error messages when mounting read-only filesystems.
       Fixes bug #504.
* FIX: Permissions of ._ AppleDouble resource fork after conversion
       from v2 to ea.
       Fixes bug #505.
* UPD: Use FreeBSD sendfile() capability to send protocol header.
       From FR #75.
* UPD: Increase IO size when sendfile() is not used.
       From FR #76.
* FIX: Can't set Finder label on symlinked folder with "follow symlinks = yes".
       Fixes bug #508.
* FIX: Setting POSIX ACLs on Linux
       Fixes bug #506.
* FIX: "ad ls" segfault if requested object is not in an AFP volume.
       Fixes bug #496.

Changes in 3.0.2
----------------

* NEW: afpd: Put file extension type/creator mapping back in which had
       been removed in 3.0.
* NEW: afpd: new option 'ad domain'. From FR #66.
* FIX: volumes and home share with symlinks in the path
* FIX: Copying packages to a Netatalk share could fail, bug #469
* FIX: Reloading volumes from config file was broken.  Fixes bug #474.
* FIX: Fix _device-info service type registered with dns-sd API
* FIX: Fix pathname bug for FCE modified event.
* FIX: Remove length limitation of options like "valid users".
       Fixes bug #473.
* FIX: Dont copy our metadata EA in copyfile(). Fixes bug #452.
* FIX: Fix an error where catalog search gave incomplete results.
       Fixes bug #479.
* REM: Remove TimeMachine volume used size FCE event.
* UPD: Add quoting support to '[in]valid users' option. Fixes bug #472.
* FIX: Install working PAM config on Solaris 11. Fixes bug #481.
* FIX: Fix a race condition between dbd and the cnid_dbd daemon
       which could result in users being disconnected from volumes
       when dbd was scanning their volumes. Fixes bug #477.
* FIX: Netatalk didn't start when the last line of the config file
       afp.conf wasn't terminated by a newline. Fixes bug #476.
* NEW: Add a new volumes option 'follow symlinks'. The default setting is
       false, symlinks are not followed on the server. This is the same
       behaviour as OS X's AFP server.
       Setting the option to true causes afpd to follow symlinks on the
       server. symlinks may point outside of the AFP volume, currently
       afpd doesn't do any checks for "wide symlinks".
* FIX: Automatic AppleDouble conversion to EAs failing for directories.
       Fixes bug #486.
* FIX: dbd failed to convert appledouble files of symlinks.
       Fixes bug #490.

Changes in 3.0.1
----------------

* NEW: afpd: Optional "ldap uuid encoding = string | ms-guid" parameter to
       afp.conf, allowing for usage of the binary objectGUID fields from
       Active Directory.
* FIX: afpd: Fix a Solaris 10 SPARC sendfilev bug
* FIX: afpd: Fix a crash on FreeBSD
* FIX: afpd: Fixes open file handle refcounting bug which was reported as
       being unable to play movies off a Netatalk AFP share.
       Bug ID 3559783.
* FIX: afpd: Fix a possible data corruption when reading from and writing
       to the server simultaniously under load
* FIX: Fix possible alignment violations due to bad casts
* FIX: dbd: Fix logging
* FIX: apple_dump: Extended Attributes AppleDouble support for *BSD
* FIX: handling of '/' and ':' in volume name
* UPD: Install relevant includes necessary for building programs with
       installed headers and shared lib libatalk
* UPD: libevent configure args to pick up installed version. Removed
       configure arg \-\-disable-libevent, added configure args
       \-\-with-libevent-header|lib.
* UPD: gentoo initscript: merge from portage netatalk.init,v 1.1
* REM: Remove \-\-with-smbsharemodes configure option, it was an
       empty stub not yet implemented

Changes in 3.0
--------------

* UPD: afpd: force read only mode if cnid scheme is last
* REM: afpd: removed global option "icon"
* FIX: CNID path for user homes

Changes in 3.0 beta2
--------------------

* UPD: Solaris and friends: Replace initscript with SMF manifest
* FIX: Solaris and friends: resource fork handling

Changes in 3.0 beta1
--------------------

* UPD: afpd: Performance tuning of read/write AFP operations. New option
       "afp read locks" (default: no) which disables that the server
       applies UNIX byte range locks to regions of files in AFP read and
       write calls.
* UPD: apple_dump: Extended Attributes AppleDouble support.
       (*BSD is not supported yet)

Changes in 3.0 alpha3
---------------------

* NEW: afpd: Per volume "login message", NetAFP bug ID #18
* NEW: afpd: Cross-platform locking (share modes) on Solaris and derivates
       with Solaris CIFS/SMB server. Uses new Solaris fcntl F_SHARE share
       reservation locking primitives. Enabled by default, set global
       "solaris share reservations" option to false to disable it.
* NEW: ad: ad set subcommand for changing Mac metadata on the server
* UPD: unix charset is UTF8 by default.
       vol charset is same value as unix charset by default.
* UPD: .AppleDesktop/ are stored in $localstatedir/netatalk/CNID
       (default: /var/netatalk/CNID), databases found in AFP volumes are
       automatically moved
* FIX: afpd: Server info packet was malformed resulting in broken
       server names being displayed on clients
* FIX: afpd: Byte order detection. Fixes an error where Netatalk on
       OpenIndiana returned wrong volume size information.

Changes in 3.0 alpha2
---------------------

* UPD: afpd: Store '.' as is and '/' as ':' on the server, don't
       CAP hexencode as "2e" and "2f" respectively
* UPD: afdp: Automatic name conversion, renaming files and directories
       containing CAP sequences to their not enscaped forms
* UPD: afpd: Correct handling of user homes and users without homes
* UPD: afpd: Perform complete automatic adouble:v2 to adouble:ea conversion
       as root. Previously only unlinking the adouble:v2 file was done as root
* UPD: dbd: -C option removes CAP encoding
* UPD: Add graceful option to RedHat init script
* UPD: Add \-\-disable-bundled-libevent configure options When set to yes,
       we rely on a properly installed version on libevent CPPFLAGS and LDFLAGS
       should be set properly to pick that up
* UPD: Run ldconfig on Linux at the end of make install
* FIX: afpd: ad cp on appledouble = ea volumes
* FIX: dbd: ignore ._ appledouble files
* REM: Volumes options "use dots" and "hex encoding"

Changes in 3.0 alpha1
---------------------

* NEW: Central configuration file afp.conf which replaces all previous files
* NEW: netatalk: service controller starting and restarting afpd and cnid_metad
       as necessary
* NEW: afpd: Extended Attributes AppleDouble backend (default)
* UPD: CNID databases are stored in $localstatedir/netatalk/CNID
       (default: /var/netatalk/CNID), databases found in AFP volumes are
       automatically moved
* UPD: Start scripts and service manifests have been changed to only start
       the new netatalk service controller process
* UPD: afpd: UNIX privileges and use dots enabled by default
* UPD: afpd: Support for arbitrary AFP volumes using variable expansion has been
       removed
* UPD: afpd: afp_voluuid.conf and afp_signature.conf location has been
       changed to $localstatedir/netatalk/ (default: /var/netatalk/)
* UPD: afpd: default server messages dir changed to $localstatedir/netatalk/msg/
* UPD: dbd: new option -C for conversion from AppleDouble v2 to ea
* REM: AppleTalk support has been removed
* REM: afpd: SLP and AFP proxy support have been removed
* REM: afpd: legacy file extension to type/creator mapping has been removed
* REM: afpd: AppleDouble backends v1, osx and sfm have been removed

Changes in 2.2.4
----------------

* FIX: Missing UAM links
* FIX: Lockup in AFP logout on Fedora 17
* FIX: Reset signal handlers and alarm timer after successfull PAM
       authentication. Fixes a problem with AFP disconnects caused
       by pam_smbpass.so messing with our handlers and timer.
* FIX: afpd: Fix a possible problem with sendfile on Solaris derived
       platforms

Changes in 2.2.3
----------------

* NEW: afpd: support for mdnsresponder
* NEW: afpd: new LDAP config option ldap_uuid_string
* UPD: based on Unicode 6.1.0
* UPD: experimental systemd service files: always run both afpd and cnid_metad
* UPD: afpd: Ensure our umask is not altered by e.g. pam_umask
* UPD: afpd: Use GSS_C_NO_NAME as server principal when Kerberos options -fqdn
       and -krb5service are not set, from Jamie Gilbertson
* UPD: afpd: Changed behaviour for TimeMachine volumes in case there's a problem
       talking to the CNID daemons. Previously the volume was flagged read-only
       and an AFP message was sent to the client. As this might result in
       TimeMachine assuming the backup sparse bundle is damaged, we now just
       switch the CNID database to an in-memory tdb without the additional stuff.
* FIX: afpd: sendfile() on FreeBSD was broken, courtesy of Denis Ahrens
* FIX: afpd: Dont use searchdb when doing partial name search
* FIX: afpd: Fix a possible bug handling disconnected sessions,
       NetAFP Bug ID #16
* FIX: afpd: Close IPC fds in afpd session child inherited from the afpd
       master process
* FIX: dbd: Don't remove BerkeleyDB if it's still in use by e.g. cnid_dbd, fixes
       bug introduced in 2.2.2
* FIX: debian initscript: start avahi-daemon (if available) before atalkd
* FIX: Zeroconf could not advertise non-ASCII time machine volume name

Changes in 2.2.2
----------------

* NEW: afpd: New option "adminauthuser". Specifying e.g. "-adminauthuser root"
       whenever a normal user login fails, afpd tries to authenticate as
       the specified adminauthuser. If this succeeds, a normal session is
       created for the original connecting user. Said differently: if you
       know the password of adminauthuser, you can authenticate as any other
       user.
* NEW: configure option "\-\-enable-suse-systemd" for openSUSE12.1 and later.
       "\-\-enable-redhat-systemd" and "\-\-enable-suse-systemd" are same as
       "\-\-enable-systemd".
       "\-\-enable-suse" is renamed "\-\-enable-suse-sysv".
* NEW: experimental systemd service files in distrib/systemd/
* UPD: afpd: Enhanced POSIX ACL mapping semantics, from Laura Mueller
* UPD: afpd: Reset options every time a :DEFAULT: line is found in a
       AppleVolumes file
* UPD: afpd: Convert passwords from legacy encoding (wire format) to host
       encoding, NetAFP Bug ID #14
* UPD: afpd: Don't set ATTRBIT_SHARED flag for directories
* UPD: afpd: Use sendfile() on Solaris and FreeBSD for sending data
* UPD: afpd: Faster volume used size calculation for "volsizelimit" option,
       cf man AppleVolume.default for details
* FIX: afpd: ACL access checking
* FIX: afpd: Fix an error when duplicating files that lacked an AppleDouble
       file which lead to a possible Finder crash
* FIX: afpd: Read-only filesystems lead to afpd processes running as root
* FIX: afpd: Fix for filesystem without NFSv4 ACL support on Solaris
* FIX: afpd: Fix catsearch bug, NetAFP Bug ID #12
* FIX: afpd: Fix dircache bug, NetAFP Bug ID #13
* FIX: dbd: Better checking for duplicated or bogus CNIDs from AppleDouble
       files
* FIX: dbd: Remove BerkeleyDB database environment after running 'dbd'. This
       is crucial for the automatic BerkeleyDB database upgrade feature which
       is built into cnid_dbd and dbd.
* FIX: Fix compilation error when AppleTalk support is disabled
* FIX: Portability fixes
* FIX: search of surrogate pair

Changes in 2.2.1
----------------

* NEW: afpd: disable continous service feature by default, new option
       -keepsessions to enable it
* NEW: configure option "\-\-enable-redhat-systemd" for Fedora15 and later.
       "\-\-enable-redhat" is renamed "\-\-enable-redhat-sysv".
* UPD: afpd: Enhance ACL support detection for volumes: enable them per volume
       if
       1) ACL support compiled in, 2) the volume supports ACLs, 3) the new
       volume option "noacls" is not set for the volume.
       The previous behaviour was to enable ACL support for a volume if
       1) it was compiled in and 2) the volume supported ACLs. There was no way
       to disable ACLs for a volume.
* UPD: afpd: add a configurable hold time option to FCE file modification event
       generation, default is 60 s, new option "fceholdfmod" to change it
* UPD: afpd: add support for new NetBSD quota subsystem, Bug ID 3249879
* FIX: afpd: increase BerkeleyDB locks and lockobjs
* FIX: afpd: create special folder as root
* FIX: afpd: fix compilation error if \-\-enable-ddp is used
* FIX: afpd: More robust IPC reconnect error handling
* FIX: afpd: ACL access checking
* FIX: afpd: fix a possible race condition between SIGCHLD handler and
       new connection attempts
* FIX: afpd: fix undefined behaviour when more then ~510 connetions where
       established
* FIX: afpd: fix a crash when searching for a UUID that is not a special
       local UUID and LDAP support is not compiled in
* FIX: afpd: .volinfo file not created on first volume access if user in rolist
* FIX: afpd: possible crash at startup when registering with Avahi
       when Avahi is not running
* FIX: afpd: return correct user/group type when mapping UUIDs to names
* FIX: afpd: for directories add DARWIN_ACE_DELETE ACE
       if DARWIN_ACE_ADD_SUBDIRECTORY is set
* FIX: afpd: afpd crashed when it failed to register with Avahi because e.g.
       user service registration is disabled in the Avahi config
* FIX: dbd: function checking and removing malformed ad:ea header files failed
       to chdir back to the original working directory
* FIX: cnid_dbd: increase BerkeleyDB locks and lockobjs
* FIX: cnid_dbd: implement -d option, deletes CNID db
* FIX: dbd: better detection of local (or SMB/NFS) modifications on AFP volumes
* FIX: suse: initscript return better status
* FIX: Sourcecode distribution: add missing headers
* FIX: Solaris 10: missing dirfd replacement function
* FIX: case-conversion of surrogate pair
* FIX: Compilation error on GNU/kFreeBSD, fixes Bug ID 3392794 and
       Debian #630349
* FIX: Fix Debian Bug#637025
* FIX: Multiple *BSD compilation compatibility fixes, Bug ID 3380785
* FIX: precompose_w() failed if tail character is decomposed surrogate pair

Changes in 2.2.0
----------------

* NEW: afpd: new volume option "nonetids"
* NEW: afpd: ACL access check caching
* NEW: afpd: FCE event notifications
* NEW: afpd: new option "-mimicmodel" for specifying Bonjour model registration
* UPD: Support for Berkeley DB 5.1
* UPD: case-conversion is based on Unicode 6.0.0
* UPD: cnid_metad: allow up to 4096 volumes
* UPD: afpd: only forward SIGTERM and SIGUSR1 from parent to childs
* UPD: afpd: use internal function instead of popening du -sh in order to
       calculate the used size of a volume for option "volsizelimit"
* UPD: afpd: Add negative UUID caching, enhance local UUID handling
* FIX: afpd: configuration reload with SIGHUP
* FIX: afpd: crashes in the dircache
* FIX: afpd: Correct afp logout vs dsi eof behaviour
* FIX: afpd: new catsearch was broken
* FIX: afpd: only use volume UUIDs in master afpd
* FIX: dbd: Multiple fixes, reliable locking
* FIX: ad file suite: fix an error that resulted in CNID database
       inconsistencies

Changes in 2.2beta4
-------------------

* NEW: afpd: new afpd.conf options "tcprcvbuf" and "tcpsndbuf" to customize
       the corresponding TCP socket options.
* NEW: afpd: new afpd.conf option "nozeroconf" which disabled automatic
       Zeroconf service registration.
* FIX: afpd: generate mersenne primes for DHX2 UAM once at startup,
       not for every login
* FIX: afpd: DSI streaming deadlock
* FIX: afpd: extended sleep
* FIX: afpd: directory cache
* FIX: Support for platforms that do not have the *at functions
* UPD: afpd: put POSIX write lock on volume files while reading them

Changes in 2.2beta3
-------------------

* FIX: afpd: fix option volsizelimit to return a usefull value for the
       volume free space using 'du -sh' with popen
* FIX: afpd: fix idle connection disconnects
* FIX: afpd: don't disconnect sessions for clients if boottimes don't match
* FIX: afpd: better handling of very long filenames that contain many
       multibyte UTF-8 glyphs

Changes in 2.2beta2
-------------------

* NEW: afpd: AFP 3.3
* UPD: afpd: AFP 3.x can't be disabled

Changes in 2.2beta1
-------------------

* FIX: composition of Surrogate Pair
* UPD: gentoo,suse,cobalt,tru64: inistscript name is "netatalk", not "atalk"
* UPD: gentoo: rc-update install don't hook in the Makefile

Changes in 2.2alpha5
--------------------

* UPD: afpd: new option "searchdb" which enables fast catalog searches
       using the CNID db.
* UPD: Case-insensitive fast search with the CNID db
* UPD: cnid_dbd: afpd now passes the volume path, not the db path when
       connecting for a volume. cnid_dbd will read the
       ".AppleDesktop/.volinfo" file of the volume in order to figure
       out the CNID db path and the volume charset encoding.

Changes in 2.2alpha4
--------------------

* NEW: Enhanced CNID "dbd" database for fast name search support.
       Important: this makes cnidscheme "cdb" incompatible with "dbd".
* NEW: afpd: support for fast catalog searches
* NEW: ad utility: ad find
* UPD: afpd: CNID database versioning check for "cdb" scheme
* UPD: cnid_dbd: CNID database versioning and upgrading. Additional
       CNID database index for fast name searches.

Changes in 2.2alpha3
--------------------

* FIX: afpd: various fixes
* FIX: Any daemon did not run if atalkd doesn't exist (redhat/debian)

Changes in 2.2alpha2
--------------------

* FIX: afpd: fix compilation error when ACL support is not available
* FIX: Ensure Appletalk manpages and config files are distributed

Changes in 2.2alpha1
--------------------

* NEW: ad utility: ad cp
* NEW: ad utility: ad rm
* NEW: ad utility: ad mv
* NEW: afpd: dynamic directoy and CNID cache (new config option -dircachesize)
* NEW: afpd: POSIX 1e ACL support
* NEW: afpd: automagic Zeroconf registration with avahi, registering both
       the service _afpovertcp._tcp and TimeMachine volumes with _adisk._tcp.
* UPD: afpd: ACLs usable (though not visible on the client side) without common
       directory service, by mapping ACLs to UARight
* UPD: afpd: performance improvements for ACL access calculations
* UPD: AppleTalk is disabled by default at configuration time. If needed
       use configure switch \-\-enable-ddp.
* FIX: afpd: Solaris 10 compatibilty fix: don't use SO_SNDTIMEO/SO_RCVTIMEO,
       use non-blocking IO and select instead.
* FIX: cnid_dbd: Solaris 10 compatibilty fix: don't use SO_SNDTIMEO/SO_RCVTIMEO,
       use non-blocking IO and select instead.
* REM: afile/achfile/apple_cp/apple_mv/apple_rm: use ad

Changes in 2.1.6
----------------

* FIX: afpd: Fix for LDAP user cache corruption
* FIX: afpd: Fix for not shown ACLs for when filesyem uid or gid
       couldn't be resolved because (e.g. deleted users/groups)
* FIX: gentoo: cannot set $CNID_CONFIG
* FIX: ubuntu: servername was empty
* FIX: Solaris: configure script failed to enable DDP module
* FIX: AppleDouble buffer overrun by extremely long filename
* UPD: afpd: return version info with machine type in DSIGetStatus
* UPD: dbd: use on-disk temporary rebuild db instead of in-memory db
* UPD: suse: initscript update

Changes in 2.1.5
----------------

* UPD: afpd: support newlines in -loginmesg with \n escaping syntax
* UPD: afpd: support for changed chmod semantics on ZFS with ACLs
       in onnv145+
* FIX: afpd: fix leaking resource when moving objects on the server
* FIX: afpd: backport Solaris 10 compatibilty fix from 2.2: don't use
       SO_SNDTIMEO/SO_RCVTIMEO, use non-blocking IO and select instead.
* FIX: afpd: misaligned memory access on Sparc in ad_setattr, fixes
       bug 3110004.
* FIX: cnid_dbd: backport Solaris 10 compatibilty fix from 2.2: don't
       use SO_SNDTIMEO/SO_RCVTIMEO, use non-blocking IO and select instead.

Changes in 2.1.4
----------------

* FIX: afpd: Downstream fix for FreeBSD PR 148022
* FIX: afpd: Fixes for bugs 3074077 and 3074078
* FIX: afpd: Better handling of symlinks in combination with ACLs and EAs.
       Fixes bug 3074076.
* FIX: dbd: Adding a file with the CNID from it's adouble file did
       not work in case that CNID was already occupied in the database
* FIX: macusers: add support for Solaris
* NEW: cnid_metad: use a PID lockfile
* NEW: afpd: prevent log flooding
* UPD: dbd: ignore ".zfs" snapshot directories
* UPD: dbd: support interrupting -re mode

Changes in 2.1.3
----------------

* FIX: afpd: fix a serious error in networking IO code
* FIX: afpd: Solaris 10 compatibilty fix: don't use SO_SNDTIMEO, use
       non-blocking IO and select instead for writing/sending data.
* UPD: Support for BerkeleyDB 5.0.

Changes in 2.1.2
----------------

* FIX: afpd: fix for possible crash in case more then one server is
       configured in afpd.conf.
* FIX: afpd: ExtendedAttributes in FreeBSD
* FIX: afpd: sharing home folders corrupted the per volume umask.
* UPD: afpd: umask for home folders is no longer taken from startup umask.
* UPD: afpd: don't and permissions with parent folder when creating new
       directories on "upriv" volumes.
* UPD: afpd: use 'afpserver@fqdn' instead of 'afpserver/fqdn@realm'.
       Prevents a crash in older GNU GSSAPI libs on e.g. CentOS 5.x.

Changes in 2.1.1
----------------

* UPD: fallback to a temporary in memory tdb CNID database if the volume
       database can't be opened now works with the default backend "dbd" too.
* FIX: afpd: afp_ldap.conf was missing from tarball. This only effected
       [Open]Solaris.
* FIX: afpd: Check if options->server is set in set_signature, preventing
       SIGSEGV.
* FIX: afpd: server signature wasn't initialized in some cases
* FIX: DESTDIR support: DESTDIR was expanded twice
* FIX: Fix for compilation error if header files of an older Netatalk
       version are installed.

Changes in 2.1-release
----------------------

* NEW: afpd: new volume option "volsizelimit" for limitting reported volume
       size. Useful for limitting TM backup size.
* UPD: dbd: -c option for rebuilding volumes which prevents the creation
       of .AppleDouble stuff, only removes orphaned files.

Changes in 2.1-beta2
--------------------

* NEW: afpd: static generated AFP signature stored in afp_signature.conf,
       cf man 5 afp_signature.conf
* NEW: afpd: clustering support: new per volume option "cnidserver".
* UPD: afpd: set volume defaults options "upriv" and "usedots" in the
       volume config file AppleVolumes.default. This will only affect
       new installations, but not upgrades.
* FIX: afpd: prevent security attack guessing valid server accounts. afpd
       now returns error -5023 for unknown users, as does AppleFileServer.

Changes in 2.1-beta1
--------------------

* NEW: afpd: AFP 3.2 support
* NEW: afpd: Extended Attributes support using native attributes or
       using files inside .AppleDouble directories.
* NEW: afpd: ACL support with ZFS
* NEW: cnid_metad: options -l and -f to configure logging
* NEW: IPv6 support
* NEW: AppleDouble compatible UNIX files utility suite 'ad ...'.
       With 2.1 only 'ad ls'.
* NEW: CNID database maintanance utility dbd
* NEW: support BerkeleyDB upgrade. Starting with the next release
       after 2.1 in case of BerkeleyDB library updates, Netatalk
       will be able to upgrade the CNID databases.
* NEW: afpd: store and read CNIDs to/from AppleDouble files by default.
       This is used as a cache and as a backup in case the database
       is deleted or corrupted. It can be disabled with a new volume
       option "nocnidcache".
* NEW: afpd: sending SIGINT to a child afpd process enables debug logging
       to /tmp/afpd.PID.XXXXXX.
* NEW: configure args to download and install a "private" Webmin instance
       including only basic Webmin modules plus our netatalk.wbm.
* NEW: fallback to a temporary in memory tdb CNID database if the volume
       database can't be opened.
* NEW: support for Unicode characters in the range above U+010000 using
       internal surrogate pairs
* NEW: apple_dump: utility to dump AppleSingle and AppleDouble files
* NEW: afpldaptest: utility to check afp_ldap.conf.
* UPD: atalkd and papd are now disabled by default. AppleTalk is legacy.
* UPD: slp advertisement is now disabled by default. server option -slp
       SRVLOC is legacy.
* UPD: cdb/dbd CNID backend requires BerkeleyDB >= 4.6
* UPD: afpd: default CNID backend is "dbd"
* UPD: afpd: try to install PAM config that pulls in system|common auth
* UPD: afpd: symlink handling: never followed server side, client resolves
       them, so it's safe to use them now.
* UPD: afpd: Comment out all extension->type/creator mappings in
       AppleVolumes.system. They're unmaintained, possibly wrong and
       do not fit for OS X.
* FIX: rewritten logger
* FIX: afpd: UNIX permissions handling
* FIX: cnid_dbd: always use BerkeleyDB transactions
* FIX: initscripts installation now correctly uses autoconf paths,
       i.e. they're installed to \-\-sysconfdir.
* FIX: UTF-8 volume name length
* FIX: atalkd: workaround for broken Linux 2.6 AT kernel module:
       Linux 2.6 sends broadcast queries to the first available socket
       which is in our case the last configured one. atalkd now tries to
       find the right one.
       Note: now a misconfigured or plugged router can broadcast a wrong route !
* REM: afpd: removed CNID backends "db3", "hash" and "mtab"
* REM: cnid_maint: use dbd
* REM: cleanappledouble.pl: use dbd
* REM: nu: use 'macusers' instead

Changes in 2.0.5
----------------

* NEW: afpd: Time Machine support with new volume option "tm".
* FIX: papd: Remove variable expansion for BSD printers. Fixes CVE-2008-5718.
* FIX: afpd: .AppleDxxx folders were user accessible if option 'usedots'
       was set
* FIX: afpd: vetoed files/dirs where still accessible
* FIX: afpd: cnid_resolve: don't return '..' as a valid name.
* FIX: uniconv: -d option wasn't working

Changes in 2.0.4
----------------

* REM: remove timeout
* NEW: afpd: DHX2 uams using GNU libgcrypt.
* NEW: afpd: volume options 'illegalseq', 'perm' and 'invisibledots'
       'ilegalseq'  encode illegal sequence in filename asis, ex "\217-", which is not
       a valid SHIFT-JIS char, is encoded  as U\217 -.
       'perm' value OR with the client requested permissions. (help with OSX 10.5
       strange permissions).
       Make dot files visible by default with 'usedots', use 'invisibledots'
       for keeping the old behavior, i.e. for OS9 (OSX hide dot files on its
       own).
* NEW: afpd: volume options allow_hosts/denied hosts
* NEW: afpd: volume options dperm/fperm default directory and file
       permissions or with server requests.
* NEW: afpd: afpd.conf, allow line continuation with \
* NEW: afpd: AppleVolumes.default allow line continuation with \
* NEW: afpd: Mac greek encoding.
* NEW: afpd: CJK encoding.
* UPD: afpd: Default UAMs: DHX + DHX2
* FIX: afpd: return the right error in createfile and copyfile if the disk
       is full.
* FIX: afpd: resolveid return the same error code than OSX if it's a directory
* FIX: afpd: server name check, test for the whole loopback subnet
       not only 127.0.0.1.
* UPD: afpd: limit comments size to 128 bytes, (workaround for Adobe CS2 bug).
* UPD: afpd: no more daemon icon.
* UPD: usedots, return an invalide name only for .Applexxx files used by netatalk not
       all files starting with .apple.
* UPD: cnid: increase the number of cnid_dbd slots to 512.
* FIX: cnid: dbd detach the daemon from the control terminal.
* UPD: cnid: never ending Berkeley API changes...
* UPD: cnid: dbd add a timeout when reading data from afpd client.
* UPD: cnid: Don't wait five second after the first error when speaking to the dbd
       backend.
* FIX: papd: vars use % not $
* FIX: papd: quote chars in popen variables expansion. security fix.
* FIX: papd: papd -d didn't write to stderr.
* FIX: papd: ps comments don't always use ()
* FIX: many compilation errors (solaris, AFS, Tru64, xfs quota...).

Changes in 2.0.3
----------------

* NEW: afpd: add a cachecnid option that controls if afpd should
       use the IDs stored in the AD2 files as cache. Defaults
       to off.
* UPD: afpd: deal with more than 32 groups.
* FIX: afpd: several catsearch fixes, based on patch from
       TSUBAKIMOTO Hiroya.
* FIX: afpd: fix a race when a client very quickly reconnects and
       tries to kill its old session.
* FIX: afpd: OSX style symlink caused problems with Panther clients.
* FIX: afpd: old files with default type didn't show the right icon
       in finder, from Shlomi Yaakobovich, slightly modified.
* FIX: cnid_check: disable cnid_check if CNID db was configured with
       transactions and really bail out after the first error.
* FIX: admin-group configure option was broken.
* FIX: several problems with IDs cached in AD2 files.
* FIX: Ignore BIDI in UTF8 hints from OSX.
* FIX: Lots of gcc warning fixes.
* FIX: small configure script changes.

Changes in 2.0.2
----------------

* NEW: cnid: Add an indexes check and rebuild, optional for dbd
       (parameter check default no), standalone program cnid_index for
       cdb.
* UPD: Enhanced afpd's -v command line switch and added -V for more
       verbose information
* UPD: uams_gss: build the principal used by uams_gss.so from afpd's
       configuration, don't use GSS_C_NT_HOSTBASED_SERVICE
* UPD: cnid_dbd: add process id in syslog and small clean up
* REM: remove netatalkshorternamelinks.pl cf. SF bug [ 1061396 ]
       netatalkshorternamelinks.pl broken
* FIX: afpd: check for DenyRead on FPCopyFile
* FIX: afpd: add missing flush for AD2 Metadata on FPCopyFile, SF bug
       [ 1055691 ] Word 98 OS 9 Saving an existing file
* FIX: afpd: Deal with AFP3 connection and type 2 (non-UTF8) names.
       reported by Gair Heaton, HI RESOLUTION SYSTEMS
* FIX: afpd: Broken 'crlf' option
* FIX: afpd: fix SF bug [ 1079622 ] afpd/dhx memory bug,
       by Ralf Schuchardt
* FIX: afpd: Return an error if we cannot get the db stamp in
       afp_openvol.
* FIX: afpd: Fix slp registration with Solaris9 slpd, from
       hat at fa2.so-net.ne.jp

Changes in 2.0.1
----------------

* NEW: \-\-enable-debian configure option. Will install /etc/init.d/atalk
       to get not in conflict with standard debian /etc/init.d/netatalk.
       Reads netatalk.conf from $ETCDIR and not from /etc/default/
* UPD: Disable logger code by default. Log to syslog instead
* UPD: changed netatalk.conf default settings to prevent problems with
       AppleTalk zone names containing spaces
* FIX: insecure tempfile handling bug in etc2ps.sh,
       found by Trustix, CAN-2004-0974.
* REM: remove add_netatalk_printer and netatalk.template from stable
       branch until fixed. (possible symlink vulnerabilities)
* FIX: afpd: set hasBeenInited in default finder info. This bug caused
       endless finder refreshes with OS9 finder if the noadouble option
       was used. From TSUBAKIMOTO Hiroya.
* FIX: afpd: fix a bug in default CREATOR/TYPE handling. Due to this bug
       the type/creator mappings in AppleVolumes.system were ignored,
       causing problems i.e. with OS9 clients.
* FIX: AppleVolumes.system: By default don't define a CREATOR/TYPE for a
       file of unknown type.
* FIX: fix two Tru64 UNIX compilation errors,
       from Burkhard Schmidt bs AT cpfs.mpg.de
* FIX: afpd: FPMapId wasn't using UTF8 for groups if requested by client.

Changes in 2.0.0
----------------

* UPD: afpd: add an error message if -ipaddr parameter cannot be parsed
* UPD: updated documentation
* FIX: afpd: fix a file descriptor and memory leak with OSX ._ resource fork
* FIX: afpd: Prevent overwriting a file by renaming a file in the same
       directory to the same name. Won't work with OSX, the dest file gets
       deleted by OSX first.
* FIX: sometimes '0' was used instead of 0 for creator/type
* FIX: removed setpgrp check from configure, we don't use it anymore and
       it doesn't work with cross compile.
* FIX: fix for Solaris "make maintainer-clean", from Alexander Barton
* FIX: fix username matching bug in afppasswd. from kanai at nadmin dot org
* FIX: reworked username check a little. Depending on the UAM, the wrong
       username *could* have been selected.

Changes in 2.0-rc2
------------------

* UPD: use 0 0 for default creator/type rather than UNIX TEXT, from
       Shlomi Yaakobovich.
* UPD: updated documentation
* UPD: change machine type from Macintosh to Netatalk in status reply
* FIX: afpd: CopyFile only create a resource fork for destination if source
       has one.
* FIX: afpd: mangling: for utf8 --> max filename length is 255 bytes, else 31.
* FIX: cnid_dbd: fix a signed/unsigned, 16/32 bits mismatch. from Burkhard
       Schmidt, bs at cpfs.mpg.de.
* FIX: afpd: After ad_setid don't flush resource fork if it has not been
       modified.
* FIX: NEWS: Fixed ancient NEWS entries. Removed umlauts
* FIX: fix macname cache, SF bug 1021642
* FIX: revert Makefile change from 2.0-rc1. We have to include BDB_CFLAGS
       after CFLAGS

Changes in 2.0-rc1
------------------

* NEW: new manual page for asip-status.pl
* UPD: updated documentation
* UPD: uams: link uam_dhx_passwd.so to lcrypt before lcrypto. might help with
       MD5 passwords
* UPD: Improved BerkeleyDB detection
* UPD: sys/solaris/Makefile.in: enable 'make check', from Alexander Barton
* UPD: tcp wrappers detection should work on OpenBSD as well now
* UPD: macbin: increase the maximum size of macbinary forks, as suggested by
       Sourceforge bug ID 829221
* UPD: ASP: rework getstatus. use several ASP packets if the client allows
       it, otherwise just send as much as we can
* FIX: FreeBSD 5 build, from Alex Barton (alex at barton.de)
* FIX: OSX 10.3 build
* FIX: papd: workaround a problem with PJL before Postscript
* FIX: afpd: make sure we only disconnect on old session if the users match
* FIX: apfd: Quark6 mangled long filenames should work better now
* FIX: enhance ADv1 to ADv2 conversion. Fixed a SIGSEGV reported by Mark Baker
* FIX: better detection of invalid resource forks
* FIX: fix some linking problems on OpenBSD
* FIX: afpd: catsearch.c, filedir.c: fix bogus casts, from Olaf Hering
       (olh at suse.de)
* FIX: afpd: don't try to create special folders and .volinfo on read-only
       volumes
* FIX: iconv/unicode enhancements. fixed a sigsegv on conversion error
* FIX: configure.in: fix a typo, reported by Joerg Rossdeutscher
* FIX: uniconv: enhanced uniconv behaviour
* FIX: fixed some Solaris compilation problems
* FIX: papd/Makefile.am: add a missing $DESTDIR, from Vlad Agranovsky
* FIX: afpd: quota.c: remove a c99 declaration, from Yann Rouillard
* FIX: configure.in: Solaris/gcc 3.0 fix, from Yann Rouillard
* FIX: afpd: fix a SIGSEGV when sharing home dirs without any options in
       AppleVolumes.
* FIX: numerous small bugfixes

Changes in 2.0-beta2
--------------------

* NEW: atalkd, papd and npb tools now support nbpnames with extended
       characters
* NEW: integrated CUPS support for papd
* NEW: optionally advertise SSH tunneling capabilties
* NEW: automatic logfile removal for cnid_metad
* NEW: asip-status.pl has been added to netatalk
* UPD: updated documentation
* UPD: we now require Berkeley DB >= 4.1
* UPD: 64bit Linux fixes from Stew Benedict, Mandrakesoft
* UPD: remove \-\-enable-sendfile
* UPD: more verbose error messages
* FIX: better handling for resource forks without read access
* FIX: Tru64 build, by Burkhard Schmidt
* FIX: MIT Kerberos detection
* FIX: varios *BSD compile problems
* FIX: compile problem with libiconv, reported by Joe Marcus Clarke
* FIX: adv1tov2: make it work with the new structure
* FIX: afpd: filenames longer than 127 bytes were not enumerated correctly,
       reported by Thies C. Arntzen
* FIX: afpd: return IP before FQDN in status reply.
* FIX: afpd: Mac chooser could crash on a codepage conversion error
* FIX: afpd: KerberosV auth with Panther clients, make long AD tickets work,
       reported by Andrew Smith
* FIX: atalkd: could send invalid NBPLKUP replies, e.g with more than 15
       printers. Reported by Almacha
* FIX: papd: fix papd.conf parsing problems with consecutive ':' and missing
       newline. Reported by Craig White.
* FIX: megatron: make megatron work with UTF-8 volumes
* FIX: timeout: running timeout with commands which accept arguments,
       from Yuval Yeret.
* FIX: uniconv: fix a SEGFAULT, reported by Matthew Geier
* FIX: pam detection: PAM_C/LDFLAGS were always empty, from Alexander Barton
* FIX: numerous small bugfixes.

Changes in 2.0-beta1
--------------------

* NEW: OSX style adouble scheme
* NEW: japanese SHIFT_JIS codepage (iconv supplied)
* NEW: Solaris kernel module build integrated with configure
* NEW: Gentoo start scripts
* NEW: cnid_dbd doesn't use transactions by default
* FIX: afpd: the volume casefold option was broken
* FIX: afpd: update AD2 headers and keep owner on file exchange
* FIX: Solaris 9 and FreeBSD 4.9/5.2 compilation
* FIX: free space reported with groups quotas on Linux
* FIX: OS9/OS X didn't update free space
* FIX: finder crash if folder opened got deleted by another process
* FIX: randnum UAM wasn't AFP3 ready
* FIX: numerous small bugfixes.

Changes in 2.0-alpha2
---------------------

* NEW: uniconv tool for converting volume encoding.
* NEW: afpd: Make sure getstatus doesn't return loopback address as server IP.
* NEW: afpd: Specify USEDOTS with MSWINDOWS implicitely.
* NEW: afpd: SRVLOC register with IP address instead of hostname by default,
       if -fqdn is specified register with FQDN.  Added extended character
       support for SLP, non ASCII characters are escaped Added ZONE to registration.
* NEW: atalkd: Make atalkd set interfaces to ALLMULTI on linux by default.
* NEW: cnid_metad: Use DB_PRIVATE attribute for dbd backend without transaction.
* FIX: afpd: Partial workaround for an OSX client deadlock.
* FIX: afpd: Reenumerate folder if db is out of sync in resolveID.
* FIX: afpd: Don't modify modification date in copyfile.
* FIX: afpd: Variable $v substitution: always use name defined in config files.
* FIX: libatalk: Speed optimisation for byte locking was broken on resource fork.
* FIX: Solaris 9 compilation.
* FIX: Tru64 compilation fixes.
* FIX: AFS compilation fixes.
* FIX: strncpy bugfix.
* FIX: configure, man pages update and small bugfixes.

Changes in 2.0-alpha1
---------------------

* NEW: afpd: We now support AFP 3.x, which features long filenames and
       large file support
* NEW: CNID handling completely reworked. We now use per Volume configurable
       CNID backends.
* NEW: Two new daemons, cnid_metad and cnid_dbd to implement the dbd CNID backend.
       Using Berkeley DB transactions with the CNID database should be safe now.
* NEW: The on disk format of the CNID database has changed. We now support 8 byte
       device and inode numbers and an additinal type field that should make
       detection of file/directory changes outside of afpd more robust.
       Changed from HASH to BTREE access which speeds things up in certain cases
       and reduces database file size.
* NEW: Long file name mangling is now implemented using the CNID ID, so no database is
       required to map names to mangled names. This is the same method Mac OS X uses.
* NEW: New format (version 2) for Metadata in AppleDouble files. We record device and
       inode of the underlying file as well as the CNID. This can be used for recovery
       and speeds up access to the CNID.
* NEW: The old NLS codepage system has been removed. It was replaced by an iconv
       based conversion system, which provides much more flexibility.
* NEW: You can safely use extended characters in volume names and for SIGUSR2 messaging.
* NEW: The default volume encoding is UTF-8.
* NEW: All documentation is now generated using DocBook. New or completely reworked
       sections about installing, setting up and upgrading Netatalk replace various
       README files in the doc directory.
* FIX: Protect afpd better against unexpected signals during updates of the CNID
       database with the cdb backend. This makes database corruption less likely.
* FIX: All manpages have been reviewed and should now be up to date.
* FIX: Tons of bug fixes since 1.6.4. Please consults the CVS change log for details.

Changes in 1.6.4
----------------

* NEW: afpd: Using the mswindows option now implicitly specifies usedots.
  [Sam Noble]
* UPD: afpd.8: Updated the option documentation.
  [Thomas Kaiser, Sebastian Rittau]
* FIX: configure: Removed broken \-\-with-flock-locks option. [Bjoern Fernhomberg]
* FIX: libatalk: Do not log network probe (OSX 10.3). [Didier Gautheron]
* FIX: libatalk: Deadlock in signal handlers. [Didier]
* FIX: libatalk: Compilation with Linux kernel 2.6 fixed. [Sebastian, Bjoern]
* FIX: afpd: Solaris compile issues. [Bjoern]
* FIX: afpd: If connection broke in dsi_tickle the child did never die. [Bjoern]
* FIX: afpd: Catsearch, fixes a possible segmentation fault. [Bjoern]
* FIX: afpd: Compilation issues. [Olaf Hering, Sebastian]
* FIX: cnid: Fix compile problems on Tru64. [Burkhard Schmidt]
* FIX: megatron: Fixed an uninitialized variable. [Olaf]
* FIX: installation: Don't overwrite PAM file if \-\-enable-overwrite configure
       option is not set. [Sam, Ryan Cleary]
* FIX: installation: Fixed BSD installation. [Toru TAKAMIZU]
* FIX: docs: Removed ssl-dir/ssl-dirs confusion from doc/INSTALL. [Bjoern]

Changes in 1.6.3
----------------

* UPD: afpd: Infrastructural support for an upcoming Kerberos 5 UAM.
       [Sam Noble]
* UPD: uams_dhx_passwd: Better random seed in Tru64. [Burkhard Schmidt]
* FIX: afpd: Bug in AFP connection negotiation stage. [Sam]
* FIX: afpd: Catsearch, when Mac and unix name differ, search on attributes.
* FIX: afpd: Files could be opened for writing on read-only filesystems.
* FIX: afpd: Debugging using SIGUSR1 was broken. [Stefan Muenkner]
* FIX: afpd: Segfault after login. [Robby Griffin, Sean Bolton]
* FIX: psf: Correct path to etc2ps.sh.
* FIX: shell_utils: Don't distribute generated files.
* FIX: aecho: -A option didn't work. [Chris Shiels]
* FIX: configure: Berkeley DB path detection could be wrong. [Stefan]
* FIX: Automake build fixes.

Changes in 1.6.2
----------------

* FIX: atalkd: Fixes for reading long configuration file lines. [Dan Wilga]
* FIX: afpd: CNID id allocation in a multiuser environment.
* FIX: papd: Fix PSSP support when PAM is enabled.
* FIX: macusers: Can now cope with IP addresses as well as host names.
* FIX: etc2ps.sh: Install correctly.

Changes in 1.6.1
----------------

* UPD: Improve \-\-enable-fhs. [Olaf Hering]
* UPD: Add BDB 4.1.x support.
* UPD: Add more documentation on CNID, as well as list which versions
       of Berkeley DB are supported.
* FIX: Codepage file maccode.koi8-r is now correctly cleaned.
* FIX: Fix init script location on SuSE. [Olaf]
* FIX: Build fixes. [Olaf, Steven N. Hirsch]
* FIX: Various bugs when a user doesn't have access permission to a folder.
       (Fixes an OSX kernel panic.)
* FIX: CNID, folders' DID handling with concurrent access or symlinks.
       (Fixes an endless loop in afpd.)

Changes in 1.6.0
----------------

* FIX: pap looks at the STDOUT file descriptor to see if it's connected to
       a tty, instead of STDIN.

Changes in 1.6pre2
------------------

* UPD: Removed \-\-with-catsearch option from configure and enable FPCatSearch
       by default.
* UPD: The dbpath argument does now support variable substitution.
* FIX: Build fix for non-GNU-C compilers in libatalk/util/logger.c.
* FIX: Two directories with the same name but different case confused the
       Mac Finder.
* FIX: The ROOT_PARENT directory could get invalidated.

Changes in 1.6pre1
------------------

* NEW: Long file name mangling support.
* NEW: Improved log file support.
* NEW: Server-side find capability ("FPCatSearch")
* NEW: Concurrent datastore (CDB) is now the default CNID datastore.  This
       should be heavily tested in a multiuser environment.
* NEW: Variable substitution support has been added for the dbpath AppleVolume
       option.
* UPD: CNID DID handling is now enabled by default.
* FIX: Various bug and build fixes as well as code cleanups.

Changes in 1.5.5
----------------

* NEW: Allow afpd debugging to be enabled by sending the afpd process
       a SIGUSR1.
* NEW: Allow SLP to be disabled per afpd process by adding a new -noslp flag
       to afpd.conf.
* FIX: Use server name (if not NULL) for the SLP URL.
* FIX: papd: buffer overflow during config file read (Olaf Hering)

Changes in 1.5.4
----------------

* FIX: File open read only and then read write (aka pb with Illustrator).
* FIX: Problems with unexpected EOF when reading files on a ddp connection
       Photoshop, old finder.
* FIX: \-\-with-nls-dir option does now work

Changes in 1.5.3
----------------

* UPD: Extend the \-\-disable-shell-check functionality to ignore users
       with *no* shell.  Also, send a log message if a user is refused login
       based on the fact that they have no shell.
* UPD: Autoconf updates.
* UPD: Tru64 afpd authentication updates.
* UPD: As always: lots of minor code cleanups.
* FIX: Problems with Photoshop trying to open image files has been
       corrected.
* FIX: Preserve special permission bits when creating or modifying
       directories.
* FIX: afp_deleteid() now deletes the specified file and not the parent
       directory.
* FIX: papd does now announce that it supports binary data to its clients.
* FIX: NetBSD ELF support.
* FIX: acleandir.rc is now installed in the bin directory.
* FIX: megatron does now compile even if compiled with -DDEBUG.
* FIX: Clean up some syslog messages.

Changes in 1.5.2
----------------

* NEW: NetBSD support contributed by David Rankin and NetBSD contributors.
       This includes a new configure option \-\-enable-netbsd.
* NEW: Add the -client_polling afpd.conf flag to allow for clients to poll
       the server every 10 seconds for open window updates.  Currently
       this is the only way to get asynchronous directory updates.
* UPD: Use separate macro for AFS configure check.
* UPD: Some Perl scripts are now (partially) auto-generated. This improves
       out-of-the-box usage of Netatalk.
* UPD: Solaris Kernel Makefile is now auto-generated. This fixes some path
       issues, but isn't perfect, yet. Added some Solaris compatibility
       fixes to the Kernel sources, too.
* UPD: CNID DB code sync'd with the current CVS version.  NOTE: Using this
       code requires you to delete *all* existing .AppleDB directories in
       order to avoid database corruption!
* FIX: The file AppleVolumes.system contained wrong line breaks so that
       not all file types were properly recognized.
* FIX: AFS compilation would fail due to a misnamed variable.
* FIX: SLP configure check was wrong so SLP did not compile.
* FIX: Fix the way quotas are handled under certain situations.
* FIX: Do not enable debugging code if debugging option is not set.
* FIX: Some problems with wrongly assigned DIDs were fixed.
* FIX: Various bug fixes and code cleanups.

Changes in 1.5.1
----------------

* NEW: Added a program called cnid_didname_verify that can be used to
       verify the consistency of the CNID database.
* NEW: New afpd option: -timeout. Warning: This still doesn't do what it's
       supposed to!
* UPD: Code cleanups and compatibility fixes to macusers.
* UPD: AppleVolumes.system was cleaned up.
* FIX: Really fix Tru64 compilation (see last entry).
* FIX: Hand correct error value back to AFP client when deleting files or
       directories fails.
* FIX: Leading or trailing spaces are now forbidden on volumes that have
       the AFPVOL_MSWINDOWS flag set.
* FIX: Minor code cleanups and warning fixes.
* FIX: Make quota support work on FreeBSD.

Changes in 1.5.0
----------------

* FIX: Compilation on Tru64 systems was broken, since libtool's acinclude.m4
       file on the packagers system did not contain the necessary patch.
* FIX: On some systems, atalkd refuses to start, since it couldn't detect
       any interfaces. This was caused by an overzealous validity check.

Changes in 1.5rc2
-----------------

* FIX: contrib/shell_utils/lp2pap.sh was erased when "make clean" was called.
       Now we distribute lp2pap.sh.tmpl instead, and lp2pap.sh is automatically
       generated during package build.
* FIX: Some platforms (notably Tru64) don't have the snprintf() call, which
       was used in etc/afp/afp_config.c. This call was replaced by sprintf()
       and prior bounds checking.

ASUN Changes
============

List of changes between version 1.4b2+asun and 1.5, from the historical *ChangeLog* file.

2003-01-16  Joe Marcus Clarke <marcus@marcuscom.com>

* Merge the relevant bits of README.cnid into Simon's README.ids, and
  remove README.cnid.

2003-01-15  Steven N. Hirsch <shirsch@adelphia.net>

* Fix the a2boot code on 64-bit platforms.

2003-01-11  Steven N. Hirsch <shirsch@adelphia.net>

* Add Apple II boot support.
* Fix some build nits.
* Fix issues with building RPMs on RedHat 7.3 and 8.0.
* Fix issues with ProDOS attributes on files.

2003-01-07  Rafal Marcin Lewczuk <rlewczuk@pronet.pl>

* Moving files between different directories on the same volume
  changes group of moved items if necessary (SGID bit set)
* Make FPCatSearch a bit more interactive (especially for MacOS 9)

2003-01-04  Joe Marcus Clarke <marcus@marcuscom.com>

* Add a doc/README.cnid which talks about the CNID calculation
  and persistence scheme.
* Change all references to db3/DB3 to BDB since we now support
  Berkeley DB3 and DB4.
* Add DB 4.1.x support.

2002-09-26  Andrew Morgan <morgan@orst.edu>

* Added syncmail script to CVSROOT so commits are logged to the
  <netatalk-cvs@lists.sourceforge.net> mailing list.  You can subscribe
  to this mailing list to keep track of cvs commits.

2002-09-24  Sebastian Rittau  <srittau@jroger.in-berlin.de>

* NEWS: Catted CHANGES to the end of this file. Updated from the
  stable branch.
* CHANGES: Removed.

2002-02-14  andy m <morgan@orst.edu>

* etc/papd/queries.c: Added support for "ADOIsBinaryOK?" printer query
  to papd.  We now respond with "True" instead of "unknown".

2002-02-09  joe c  <marcus@marcuscom.com>

* etc/afpd/afp_options.c: Redo the -server_notif flag.  Now, server
  notifications are enabled by default, and specifying the -client_polling
  flag will disable them.

2002-02-06  joe c  <marcus@marcuscom.com>

* etc/afpd/globals.h, etc/afpd/afp_options.c, etc/afpd/status.c
  etc/afpd/volume.c: Add a new option -server_notif to specify that
  a server supports server notifications.  If this flag is not specified
  the client will poll the server every 10 seconds for directory changes.

2002-02-03  andy m <morgan@orst.edu>

* bin/afppasswd/Makefile.am
  Added an install-exec-hook to make the afppasswd binary suid root
  after it is installed.  This lets local users change their afppasswd
  password.

2001-12-14  joe c   <marcus@marcuscom.com>

* etc/afpd/afp_options.c, etc/afpd/afp_dsi.c, etc/afpd/globals.h:
  Add a new option to afpd called -timeout to specify the number of
  server tickles to send before killing a AFPoTCP session.

2001-12-10  joe c   <marcus@marcuscom.com>

* bin/cnid/cnid_didname_verify,c: Add a utility to verify the consistency
  of didname.db.  Using the stock db_verify utility will fail as the sort
  routine is unknown.

2001-12-07  joe c   <marcus@marcuscom.com>

* libatalk/cnid/cnid_open.c: Re-enable synchronous transaction support
  to try improve performance.

2001-12-04  joe c   <marcus@marcuscom.com>

* etc/afpd/unix.c: Fix afpd sharing NFSv3 mounts (thanks to
  Pierre Beyssac <beyssac@enst.fr>)

2001-12-03  joe c   <marcus@marcuscom.com>

* etc/afpd/*.[ch]: Big commit to clean up code with astyle (readable code
  is hackable code).  Also committed a fix to give CNID DB a shot in
  production use.

2001-11-27  joe c   <marcus@marcuscom.com>

* configure.in: Removed the \-\-with-cnid-db option, and added
    \-\-with-did=cnid for consistency

2001-11-19  pooba53 <bobo@bocklabs.wisc.edu>

* Modified distrib/initscripts/Makefile.am so
  that SuSE init script ends up in the correct directory.

2001-11-19  pooba53 <bobo@bocklabs.wisc.edu>

* modified config/AppleVolumes.default to not
  have the "/Home Directory" text in it as this is not the
  proper way of allowing default Home directory access.

2001-11-16  jnewman <jnewman@mudpup.com>

* macros/db3-check.m4: Prefer specific directories before general ones

2001-11-15  pooba53 <bobo@bocklabs.wisc.edu>

* Modified SuSE initscript, distrib/initscripts/rc.atalk.suse.tmpl

2001-11-08  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/uam.c, etc/uams/uams_pam.c, etc/uams/uams_dhx_pam.c,
  include/atalk/uam.h: implemented patch #477640 for netatalk not
  passing client name properly (thanks to Patrick Bihan-Faou
  <pbf@users.sourceforge.net>)

2001-11-04  joe c <marcus@marcuscom.com>

* libatalk/cnid/cnid_open.c: Re-added code to enable on-the-fly database
  recovery

2001-10-31  Dan <bobo@bocklabs.wisc.edu>

* Fixed bug in bin/afppasswd/Makefile.am causing compile problems
  with SuSE distro.

2001-10-24  joe  c  <marcus@marcuscom.com>

* etc/afpd/fork.c: Patch to add read-only locking support
  (thanks to Miro Jurisic <meeroh@MIT.EDU>)

2001-10-23  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/{{afpd_options,filedir,main,unix}.c,
  {filedir,globals,unix}.h}: patch from Edmund Lam to allow
  perms masks

2001-10-21  joe c <marcus@marcuscom.com>

* libatalk/cnid*.c: Big patch to improve transaction throughput
  and database resiliency

2001-10-19  Lance Levsen  <l.levsen@printwest.com>

* doc/FAQ: Thanks for the Patch Karen.
* doc/INSTALL: Thanks for the Patch Karen.
* CONTRIBUTORS (Developers): Thanks for the patch Brandon.
* configure.in: Fix db3 detection for db3 3.3.x users.  Thanks to
  Jonathan Newman <jnewman@mudpup.com>

2001-10-18  joe c <marcus@marcuscom.com>

* libatalk/cnid/cnid_add.c: Fix dancing icon problem
* bin/afile/achfile.c: Fix resource fork problem on littleendian
  platforms.  Thanks to Brandon Warren <bwarren@u.washington.edu>.

2001-10-17  joe c <marcus@marcuscom.com>

* libatalk/cnid/cnid_add.c: Fix deadlock problem when copying files to
  netatalk server from multiple clients

2001-10-16  Lance Levsen  <l.levsen@printwest.com>

* man/man1/apple_mv.1.tmpl: Added apple_mv man page.
* man/man1/apple_rm.1.tmpl: Added apple_rm man page.
* contrib/shell_utils/apple_mv: Updated perl. Added error check.
* config/Makefile.am: Change autoconf variable \$\(f\) to shell
  variable \$\$f.
* man/man1/Makefile.am: Modified to allow variable subs in man pages.
* contrib/shell_utils/apple_cp: Updated. Fixed file to file
  copy.

2001-10-15  Lance Levsen  <l.levsen@printwest.com>

* CONTRIBUTORS: Now up to date.
* doc/FAQ: Added Karen A Swanberg's FAQ additions.

2001-10-14  Lance Levsen  <l.levsen@printwest.com>

* doc/INSTALL: Added some basic instructions. Filled in more of
  the ./configure options.
* doc/DEVELOPER: Added BDB3 information

2001-10-11  joe  c  <marcus@marcuscom.com>

* configure.in: More PAM fixes

2001-10-10  joe  c  <marcus@marcuscom.com>

* configure.in: More PAM fixes
* etc/uams/Makefile.am: Properly add -lpam (thanks, Sebastian)

2001-10-09  joe  c  <marcus@marcuscom.com>

* configure.in: Fix problem with forced PAM
* etc/afpd/unix.c: Fix a problem setting directory perms on FreeBSD (thanks
  to Glenn Trewitt <glenn@trewitt.org>)
* libatalk/cnid/cnid_close.c: Fix problem with .AppleDB contents showing
  up in share window
* libatalk/cnid/cnid_update.c: memset more for cleanliness sake

2001-10-04  jeff b  <jeff@univrel.pr.uconn.edu>

  Released 1.5pre8

2001-10-03  joe c   <marcus@marcuscom.com>

* configure.in: Fix bug with PAM configuration
* etc/afpd/directory.c: Fix bug with unaccessible directories causing
  afpd to erroneously return AFPERR_NOOBJ
* acinclude.m4: Fixed make problem on systems running libtool 1.3.x

2001-09-28  joe c   <marcus@marcuscom.com>

* libatalk/cnid/cnid_close.c: Add more db3 3.3.x compatibility to CNID DB

2001-09-27  joe c   <marcus@marcuscom.com>

* libatalk/cnid/cnid_open.c: Set internal deadlock detection

2001-09-23  joe c   <marcus@marcuscom.com>

* libatalk/cnid/cnid_close.c, libatalk/cnid/cnid_resolve.c,
  libatalk/cnid/cnid_open.c: More s/errno/rc fixes and some code
  cleanup

2001-09-22  joe c   <marcus@marcuscom.com>

* configure.in: Fix db3 compilation on Linux
* libatalk/cnid/cnid_get.c: Fix another potential deadlock problem by
  replacing EAGAIN with DB_LOCK_DEADLOCK

2001-09-21  joe c   <marcus@marcuscom.com>

* etc/afpd/desktop.c: Re-enable codepage translations (thanks to
  Egon Niederacher <niederacher@fh-vorarlberg.ac.at>)
* libatalk/cnid/cnid_add.c, libatalk/cnid/cnid_get.c,
  libatalk/cnid/cnid_lookup.c, libatalk/cnid/cnid_close.c,
  libatalk/cnid/cnid_open.c, libatalk/cnid/cnid_update.c: Fixed bugs
  with database contention and database corruption.

2001-09-19  joe c   <marcus@marcuscom.com>

* etc/afpd/afp_config.c: Fixed a bug where SRVLOC services wouldn't
  show up in OS 9.x
* libatalk/cnid/cnid_add.c: Fix a bug where some DBT data structures
  were not being memset to NULL correctly.

2001-09-18  joe c   <marcus@marcuscom.com>

* etc/afpd/afp_options.c: Fix a bug in the custom icon code (thanks to
  Edmund Lam <epl@unimelb.edu.au> for finding this)
* libatalk/cnid/cnid_open.c: Added db3 version checking code
* config/afpd.conf.tmpl: Removed uams_guest.so from the default UAMs
  list

2001-09-17  jeff b  <jeff@univrel.pr.uconn.edu>

* acconfig.h, configure.in, etc/afpd/afp_config.c: SLP
  support added (Joe Clarke)

2001-09-14  jeff b  <jeff@univrel.pr.uconn.edu>

* sys/netatalk/endian.h: fix from Robert Cohen
  <robert.cohen@anu.edu.au> for missing endif

2001-09-13  joe c   <marcus@marcuscom.com>

* libatalk/util/getiface.c:
  fix some malloc problems when no atalkd.conf file exists

2001-09-10  joe c   <marcus@marcuscom.com>

* libatalk/util/getiface.c: up the new interface by one
  each time instead of IFACE_NUM

2001-09-10  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/afp_options.c, etc/atalkd/main.c, etc/papd/main.c:
  added version reporting with -v switch

2001-09-06  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/atalkd/main.c, etc/papd/main.c, etc/psf/psf.c,
  libatalk/asp/asp_getsess.c, libatalk/dsi/dsi_getsess.c,
  libatalk/pap/pap_slinit.c, libatalk/util/server_child.c:
  autoconf POSIX.1 sys/wait.h check
* lots of files: AC_HEADER_STDC autoconf changes
* sys/netatalk/endian.h: used autoconf endian test instead
  of manually checking every architecture

2001-09-05  joe c <marcus@marcuscom.com>

* libatalk/cnid/cnid_open.c: comment out DB_JOINENV as this is not
  supported in db3 3.1.17
* libatalk/cnid/cnid_add.c: fix my comments to properly explain the use
  of rc over errno

2001-09-04  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/filedir.h: define veto_file() prototype (Edmund Lam)
* etc/uams/uams_dhx_pam.c: added quick Sun hack to seed openssl,
  but it *really* needs something more elegant (#458433)

2001-09-04  jeff b  <jeff@univrel.pr.uconn.edu>

* libatalk/cnid/cnid_add.c, libatalk/cnid/cnid_open.c: fixed
  duplicate DID's being generated and FreeBSD db3 fix (Joe Clarke)
* doc/README.veto, etc/afpd/directory.c, etc/afpd/enumerate.c,
  etc/afpd/file.c, etc/afpd/filedir.c, etc/afpd/volume.c,
  etc/afpd/volume.h: adds Samba-style "veto file" support
  (Edmund Lam)
* configure.in: properly checks for db3 headers (Joe Clarke)

2001-08-31  jeff b  <jeff@univrel.pr.uconn.edu>

* libatalk/cnid/cnid_*.c: compilation fixes for those who don't
  want to compile with CNID support (Edmund Lam)

2001-08-28  Lance Levsen  <l.levsen@printwest.com>

* config/Makefile.am: Added a variable substitution from
  configure.in to stop overwriting the config files.
* configure.in: Added \-\-enable-overwrite flag that enables the
  overwriting of configure files. Default is no overwrite, but does
  check for missing files.

2001-08-27  jeff b  <jeff@univrel.pr.uconn.edu>

  Released 1.5pre7

2001-08-21  jeff b  <jeff@univrel.pr.uconn.edu>

* configure.in: now does rudimentary check for DB3 library
  if CNID DB option (\-\-enable-cnid-db) is given, with
  option to specify path to DB3 (Jeff)

2001-08-16  Uwe Hees <uwe.hees@rz-online.de>

* libatalk/cnid: replaced EAGAIN in db result checking with
  DB_LOCK_DEADLOCK as appropriate for db-3.
* fixed a potential transaction problem in cnidd_add.

2001-08-14  Sam Noble <ns@shadow.org>

* etc/afpd/directory.c: in afp_mapname and afp_mapid
  convert uid/gid to/from network byte order before actually
  using.  This should hopefully fix a long-standing bug in
  the admin functionality.

2001-08-14  jeff b  <jeff@univrel.pr.uconn.edu>

* acconfig.h, configure.in, etc/afpd/directory.c,
  etc/afpd/enumerate.c, etc/afpd/file.c, etc/afpd/file.h,
  etc/afpd/filedir.c, etc/afpd/fork.c, etc/afpd/volume.c,
  etc/afpd/volume.h, libatalk/Makefile.am,
  libatalk/cnid/cnid_add.c, libatalk/cnid/cnid_close.c,
  libatalk/cnid/cnid_delete.c, libatalk/cnid/cnid_lookup.c,
  libatalk/cnid/cnid_nextid.c, libatalk/cnid/cnid_open.c,
  libatalk/cnid/cnid_private.h, libatalk/cnid/cnid_update.c:
  DID database and reincluding libatalk/cnid back into
  compiled tree (Uwe Hees)
* libatalk/cnid/.cvsignore: updated .cvsignore list for
  CNID patch (Jeff)

2001-08-09  Sam Noble <ns@shadow.org>

* configure.in, acconfig.h: Merged a patch from <meeroh@mit.edu>
  to fix the kerberos uam build process.

2001-08-08  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/papd/session.c: prevents papd client from aborting
  during the submission of a print job, therefore preventing
  the job from hanging on the Mac (Michael Boers)

2001-07-10  Lance Levsen  <lance@iworks.pwgroup.ca>

* man/man8/papd.8.tmpl: Fixed ftp URI for Adobe's PPD files.

2001-06-30  andy m  <morgan@orst.edu>

* etc/papd/ppd.c: "unquote" ppd values by removing leading
  and trailing quote character. This should fix bug #426141.

2001-06-27  jeff b  <jeff@univrel.pr.uconn.edu>

* many, many, files: more malformed ifdef correction, nicer
  comments, etc, etc, etc (Jeff)
* etc/afpd/directory.c, etc/afpd/uid.c, etc/afpd/uid.h: fixes
  for force-uidgid to compile properly. haven't tested it, but
  no more compile errors. (Jeff)

2001-06-27  uwe hees <hees@viva.de>

* etc/uams/uams_guest.c: fixed a typo.

2001-06-26  andy m  <morgan@orst.edu>

* etc/papd/file.c: modified markline() to return 1 instead
  of *linelength for successful completion. This should fix
  the remaining binary printing problems in papd.  Thanks go
  out to Dave Arnold <darn0ld@home.com> for getting me thinking
  about the markline function.

2001-06-25  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/auth.c, etc/afpd/main.c, etc/afpd/uam.c,
  etc/uams/uams_dhx_passwd.c, etc/uams/uams_passwd.c,
  include/atalk/uam.h: TRU64 authentication patch to allow
  any security scheme to be used on the TRU64 side (Burkhard
  Schmidt)
* etc/afpd/uam.c, etc/papd/uam.c: fixed DISABLE_SHELLCHECK
  support in both afpd and papd (Jason Keltz <jas@cs.yorku.ca>)
* etc/*/*.{c,h}: corrected malformed defines, nicer comments,
  CVS Id tags (Jeff)

2001-06-20  jeff b  <jeff@univrel.pr.uconn.edu>

* configure.in: check for linux/quota.h before enabling
  QUOTACTL_WRAPPER (Joe Clarke)
* acconfig.h, configure.in, include/atalk/util.h,
  libatalk/util/module.c: removed NO_DLFCN_H in favor of
  ifndef HAVE_DLFCN_H (Jeff)
* configure.in, etc/afpd/*.{c,h}, include/atalk/util.h:
  major autoconf fixes for afpd, nicer commenting, etc (Jeff)

2001-06-19  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/file.c, etc/afpd/parse_mtab.c, etc/afpd/parse_mtab.h,
  etc/atalkd/route.h, etc/atalkd/rtmp.c, etc/papd/headers.c,
  etc/papd/magics.c, libatalk/asp/asp_tickle.c: patch for
  fixed DID calculation in etc/afpd/file.c, FreeBSD errors and
  other miscellany (Joe Clarke)
* minor patches and fixes to the aforementioned files, warning
  fixes with GCC, etc (Jeff)

2001-06-18  jeff b  <jeff@univrel.pr.uconn.edu>

* configure.in, etc/uams/Makefile.am,
  etc/uams/uams_krb4/Makefile.am: patch #433952 from Sebastian
  Rittau to move UAM authentication to use libtool
* configure.in, bin/afppasswd/Makefile.am, config/Makefile.am,
  contrib/shell_utils/Makefile.am, distrib/initscripts/Makefile.am,
  etc/afpd/Makefile.am, etc/afpd/nls/Makefile.am,
  etc/atalkd/Makefile.am, etc/papd/Makefile.am,
  man/man5/Makefile.am, man/man8/Makefile.am: patch #433906
  to move to pkgconfdir for package config files (Sebastian Rittau)
* configure.in: fixed error that caused \-\-with-did not to function
  properly

2001-06-13  Sam Noble <ns@shadow.org>

* etc/papd/{printcap,ppd,lp,file,comment}.h:
  added #include <sys/cdefs.h> to these headers so that __P gets
  properly defined on platforms like TRU64

2001-06-11  jeff b  <jeff@univrel.pr.uconn.edu>

* configure.in, include/atalk/adouble.h, libatalk/compat/flock.c:
  patch #431859 to avoid ucbinclude on Solaris, with flock support,
  thanks to Russ Allbery (<rra@users.sourceforge.net>)
* acconfig.h, configure.in, libatalk/util/server_child.c,
  libatalk/util/server_lock.c, sys/netatalk/endian.h: patch #432052
  for portability to IRIX, HP-UX, and AIX (Russ Allbery)
* etc/afpd/nls/makecode.c: patch #432137 to add codepage mapping
  support for (C), (TM) and other characters to avoid losing them,
  submitted by Andre Schild (<aschild@users.sourceforge.net>)
* configure.in: set sysconfdir as /etc/netatalk by default, and
  uams path now pulls from sysconfdir instead of config_dir
  (Sam Noble)

2001-06-07  jeff b  <jeff@univrel.pr.uconn.edu>

* configure.in, bin/afppasswd/Makefile.am,
  contrib/shell_utils/Makefile.am, distrib/initscripts/Makefile.am,
  etc/afpd/Makefile.am, etc/afpd/nls/Makefile.am,
  etc/atalkd/Makefile.am, etc/papd/Makefile.am,
  man/man5/Makefile, man/man8/Makefile.am: patch #422872 from
  Sebastian Rittau to move from CONFIG_DIR to sysconfdir
* etc/psf/Makefile.am, sys/solaris/Makefile: additional removal
  of CONFIG_DIR in favor of sysconfdir, plus patch #422860 from
  Sebastian Rittau to correct other problems
* config/Makefile.am, config/netatalk.pamd: patch #422856 from
  Sebastian Rittau, moving to pam_unix.so and being more proper
* etc/afpd/Makefile.am, etc/afpd/main.c: added support for
  ${sysconfdir}/afpd.mtab to be read into memory, so that mtab
  DID support actually works...

2001-06-06  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/filedir.c, etc/afpd/unix.c: fixed dropkludge code
  so that it properly compiles again, along with minor warning
  fixen

2001-06-05 Dan L. (pooba53)

* Modified configure.in so references made to $ac_prefix_default
  listed at the beginning are correct. The previous references were
  being made to $ac_default_prefix.

2001-06-04  jeff b  <jeff@univrel.pr.uconn.edu>

* doc/README.TRU64: details about tru64 installations, from
  Edmund Lam <epl@unimelb.edu.au>
* etc/afpd/fork.c: implemented Sebastian Rittau's change to
  avoid overwriting AppleDouble headers (finally)
* configure.in, etc/afpd/enumerate.c, etc/afpd/parse_mtab.c:
  added initial support for mtab DID format. removed "lastdid"
  configure option in favor of \-\-with-did={last,mtab}

2001-06-01  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/quota.c: fix for Linux compile by Sam Noble
  <ns@shadow.org>

2001-05-25  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/uams/uams_passwd.c: another Tru64 fix from Burkhard
  Schmidt <bs@cpfs.mpg.de>
* configure.in, contrib/shell_utils/Makefile.am,
  contrib/shell_utils/afpd-mtab.pl, doc/Makefile.am,
  doc/COPYRIGHT.mtab, doc/README.mtab, doc/README.mtab.distribution,
  etc/afpd/.cvsignore, etc/afpd/Makefile.am, etc/afpd/parse_mtab.c,
  etc/afpd/parse_mtab.h, test_parse_mtab.c: experimental mtab
  code from Bob Rogers to generate more persistant DIDs

2001-05-22  jeff b  <jeff@univrel.pr.uconn.edu>

* configure.in, etc/afpd/unix.h: more portability fixes, and
  integration of Tru64 build fix from Edmund Lam <epl@unimelb.edu.au>
* configure.in, bin/megatron/Makefile.am,
  distrib/initscripts/Makefile.am, etc/afpd/main.c,
  etc/afpd/quota.c, etc/afpd/unix.h,
  etc/uams/uams_dhx_passwd.c, etc/uams/uams_passwd.c: Another
  round of Tru64 patches from Burkhard Schmidt <bs@cpfs.mpg.de>

2001-05-09  jeff b  <jeff@univrel.pr.uconn.edu>

* autogen.sh: added automake \-\-include-deps to autogen.sh to
  promote more portable Makefiles (thanks to Christian
  Weisgerber <naddy@mips.inka.de> from OpenBSD)

2001-05-08  jeff b  <jeff@univrel.pr.uconn.edu>

* bin/megatron/Makefile.am, etc/uams/Makefile.am: small Makefile fixes
  from Olaf Hering <olh@suse.de>

* etc/uams/uams_dhx_passwd.c: Tru64 fixes from Burkhard Schmidt
  <bs@cpfs.mpg.de>

2001-05-07  jeff b  <jeff@univrel.pr.uconn.edu>

* contrib/shell_utils/netatalkshorternamelinks.pl: added script to
  shorten names
* etc/afpd/quota.c, etc/uams/uams_passwd.c: patches from Burkhard
  Schmidt <bs@cpfs.mpg.de> to fix typos

2001-05-03  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/quota.c, etc/afpd/unix.h, etc/afpd/main.c,
  etc/uams/uams_passwd.c: Tru64 patch from Burkhard Schmidt <bs@cpfs.mpg.de>

* configure.in, etc/afpd/quota.c, etc/afpd/unix.h: fixes for USE\_\*\_H
  moving to autodetected HAVE\_\*\_H from autoconf script

2001-05-01  jeff b  <jeff@univrel.pr.uconn.edu>

* bin/aecho/aecho.c, bin/getzones/getzones.c, bin/megatron/asingle.c,
  bin/megatron/hqx.c, bin/megatron/macbin.c, bin/megatron/megatron.c,
  bin/megatron/nad.c, bin/megatron/updcrc.c, libatalk/atp/atp_bprint.c,
  libatalk/util/getiface.c: warnings patch from Sebastian Rittau
  <srittau@users.sourceforge.net> (#420300)
* bin/afile/*: replacement for old restrictive afile from Sebastian
  Rittau <srittau@users.sourceforge.net> (#420302)
* distrib/initscripts/rc.atalk.redhat.tmpl: daemon-specific start and
  stop messages to the redhat initscript. nbpregister and unregister
  messages are also displayed. This patch also permits spaces in zone
  and machine names to be used in the variables. From Ryan Cleary
  <tryanc@users.sourceforge.net> (#418094)
* bin/megatron/Makefile.am: patch to properly create links for
  megatron, from Sebastian Rittau <srittau@users.sourceforge.net>
  (#420446)

2001-04-25  morgan a <morgan@orst.edu>

* etc/afpd/unix.c: in setdirowner(), changed some of the syslog
  statements from LOG_ERR to LOG_DEBUG.  Some common "soft errors"
  were being logged and scaring users.  :)

2001-04-24  jeff b  <jeff@univrel.pr.uconn.edu>

* configure.in: fixed problem with tcp_wrappers support; it needed to
  check for tcpd_warn

2001-04-20  jeff b  <jeff@univrel.pr.uconn.edu>

* configure.in, etc/afpd/Makefile.am, etc/papd/Makefile.am: added
  AFPD_LIBS and PAPD_LIBS to cope with libraries that don't need to
  be used for everything

2001-04-16  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/directory.c, etc/afpd/messages.c, etc/uams/uams_dhx_pam.c:
  merged patch from Heath Kehoe <hkehoe@users.sourceforge.net> #416371,
  fixing an OSX issue, byteorder problems with uid/gid in directory.c,
  and fixing the syslog()'s in uams_dhx_pam.c to not produce useless
  errors

2001-04-12  jeff b  <jeff@univrel.pr.uconn.edu>

  Released 1.5pre6

2001-04-10  jeff b  <jeff@univrel.pr.uconn.edu>

* configure.in, acconfig.h, etc/afpd/uam.c: patch submitted by Jason
  Kelitz (jkeltz) to allow disabling of shell checking
* configure.in, contrib/Makefile.am: made timelord compilation
  optional, disabled by default

2001-04-03  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/file.c: merged patch from Soren Spies <sspies@apple.com>
  at Apple, fixing server disconnect problem upon afp_createid() call

2001-04-02  jeff b  <jeff@univrel.pr.uconn.edu>

* contrib/shell_utils/Makefile.am, contrib/shell_utils/cleanappledouble.pl:
  added cleanappledouble.pl script from Heath Kehoe <hakehoe@avalon.net>

2001-03-26  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/quota.c: fix compile dbtob problem on Linux from Sam
  Noble <ns@shadow.org>

* configure.in, etc/uams/Makefile.am, etc/uams/uams_krb4/Makefile.am:
  moved -shared into LDSHAREDFLAGS to fix Solaris build problems
  from Bob Rogers <rogers-netatalk-devel@rgrjr.dyndns.org> and
  Akop Pogosian <akopps@csua.berkeley.edu>

2001-03-22  Lance Levsen  <lance.l@dontspam.home.com>

* etc/uams/Makefile.am: Added $LDFLAGS to fix broken compile due
  to inability to find libcrypto. libcrypto is defined in LDFLAGS as
  "-L$ssldir/lib" in configure.

2001-03-22 12:57 EST  jeff b  <jeff@univrel.pr.uconn.edu>

* configure.in: patch for OpenBSD compile reported by Jean-Phillipe
  Rey <jprey@ads.ecp.fr>

2001-03-21 09:35 EST  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/ofork.c, include/atalk/adouble.h, libatalk/adouble/ad_open.c:
  patch from Jonathan Paisley (<jonp@chem.gla.ac.uk>)

2001-03-14 13:30 EST  jeff b  <jeff@univrel.pr.uconn.edu>

* configure.in: patch from Yoshinobu Ishizaki to fix problems with
  Linux 2.0.x builds (Patch #408256)
* etc/afpd/file.c: used patch at <http://www.avalon.net/~hakehoe/>
  to fix deleting/emptying trash problems (Patch #408218)

2001-03-14 11:00 EST  jeff b  <jeff@univrel.pr.uconn.edu>

* libatalk/adouble/ad_open.c: fixed O_RDWR kludge in ad_mode call
  which was causing file creation problems

2001-03-09 09:42 EST  jeff b  <jeff@univrel.pr.uconn.edu>

* sys/solaris/Makefile: fixed problems noted by Akop Pogosian in Solaris
  build, most notably paths, and reference to lp2pap.sh in the wrong
  place

2001-03-07 15:30 EST  jeff b  <jeff@univrel.pr.uconn.edu>

  Released 1.5pre5

* distrib/rpm/netatalk-redhat.spec, distrib/rpm/netatalk-mandrake.spec:
  updated for 1.5pre5 release

2001-03-07 10:34 EST  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/volume.c: changed VOLOPT_MAX to be 9 if FORCE_UIDGID is not
  defined (thanks to Axel Bringenberg <A.Bringenberg@srz-berlin.de>)

2001-03-07 10:14 EST  jeff b  <jeff@univrel.pr.uconn.edu>

* ChangeLog: started using timestamps in ChangeLog
* etc/uams/uams_krb4/Makefile.am: fixed reference to send_to_kdc.c typo
  (thanks to Sebastian Rittau)

2001-03-06 13:40  Lance Levsen <l.levsen@printwest.com>

* FAQ, README, README.ASUN, INSTALL.txt: Moved FAQ, AND READMEs to
  docs/, where they should be.
* INSTALL/INSTALL.txt: Added ./INSTALL/INSTALL.txt
* INSTALL/FAQ, README, README.ASUN: Moved README.ASUN, README,
  FAQ to ./INSTALL

2001-03-06 11:47  Andrew Morgan <morgan@orst.edu>

* TODO: A few updates to papd entry.
* README.MORGAN: Removed README.MORGAN because that information in
  now in papd's man page.
* man/man8/papd.8.tmpl: Updated papd man page to match current
  code.  Added descriptions of authenticated printing and other new
  papd options.

2001-02-28 15:43  Marc J. Millar <itlm019@mailbox.ucdavis.edu>

* libatalk/adouble/ad_open.c: AppleDouble directory creation
  debugging

2001-02-28  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/directory.c, etc/afpd/file.c, etc/afpd/filedir.c,
  etc/afpd/unix.c, etc/afpd/unix.h, etc/afpd/volume.h,
  etc/afpd/volume.c, man/man5/AppleVolumes.default.5.tmpl: added
  "dropbox" to available option if DROPKLUDGE is used during
  compile

2001-02-27  jeff b  <jeff@univrel.pr.uconn.edu>

* README: updated 1.5+ install instructions to include list of
  required and recommended packages
* etc/uams/uams_*.c: cleanups, addition of CVS Id tag to C source
* configure.in, acconfig.h: change USE_AFS to AFS to be the same as
  all of the defines in the codebase
* etc/uams/uams_dhx_pam.c: fixed DHX login using this module (last
  patch made with syslog()'s didn't include any brackets)
  (Bug #233756)
* distrib/initscripts/.cvsignore: removed pulling of atalk
* configure.in, etc/uams/Makefile.am: conditional compilation support
  for PGP UAM module using \-\-enable-pgp-uam
* configure.in, etc/uams/Makefile.am, etc/uams/uams_krb4/Makefile.am,
  etc/uams/uams_krb4/.cvsignore, etc/uams/uams_krb4/*.c: modifications
  for future Kerberos module reintegration

2001-02-26  jeff b  <jeff@univrel.pr.uconn.edu>

* configure.in: added /usr/local/ssl to list of SSL paths to check, to
  help kludge compilation on Mac OS X from Marcel <lammerse@xs4all.nl>
* distrib/initscripts/rc.atalk.redhat.tmpl: adjusted to echo warning
  instead of dumping out if appletalk module not present, from
  Steven Karen <karelsf@users.sourceforge.net> (Bug #404087)
* configure.in, contrib/timelord/timelord.c: applied patch from Wes
  Hardaker <hardaker@users.sourceforge.net> (Patch #402245), with
  suitable configure.in fixes

2001-02-23  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/desktop.c, etc/afpd/codepage.c, etc/afpd/nls/makecode.c:
  patch from Axel Barnitzek <barney@users.sourceforge.net> to fix
  broken codepage support.
* ChangeLog: started updaing ChangeLog with important patch/fix
  information, as it is *never* up to date.
* configure.in, acconfig.h: implemented AFS configuration option
  patch from Wes Hardaker <hardaker@users.sourceforge.net>
* VERSION: bumped up version to 1.5pre5, since 1.5pre4 was kind of
  paperbag-ish
* autogen.sh: make libtoolize copy instead of linking files to
  avoid problems, thanks to Wes Hardaker <hardaker@users.sourceforge.net>

2001-02-20  jeff b  <jeff@univrel.pr.uconn.edu>

  Released 1.5pre4

* Debian packaging in tree
* Numerous Makefile/build fixes
* .cvsignore implemented
* Solaris build fixes

2001-01-02  jeff b  <jeff@univrel.pr.uconn.edu>

* etc/afpd/uid.c, etc/afpd/uid.h, ...: added support for forcing
  uid/gid per volume for afpd

2000-09-22  Roland Schulz <rdschulz@abarrach.franken.de>

* etc/afpd/volume.c (setvoltime): fix for multiple clients
  writing to same volume.

2000-02-28  a sun  <asun@asun.cobalt.com>

* etc/afpd/directory.h (CNID_INODE): xor the inode a little
  differently.

2000-02-23  a sun  <asun@asun.cobalt.com>

* etc/afpd/volume.c (creatvol): / is a special case. you can't
  share it unless you give it a name.

2000-02-21  a sun  <asun@asun.cobalt.com>

* distrib/initscripts/rc.atalk.redhat/cobalt: added changes to
  make redhat 6.x happier.

2000-02-17  a sun  <asun@asun.cobalt.com>

* libatalk/adouble/ad_lock.c (adf_unlock): off-by-one error with
  lock removal. this + the log right below fix ragtime.

2000-02-16  a sun  <asun@asun.cobalt.com>

* etc/afpd/fork.c (afp_bytelock): only error on bytelocks
  positioned at 0x7FFFFFFF if there's no resource fork.

2000-02-14  a sun  <asun@asun.cobalt.com>

* libatalk/adouble/ad_lock.c: re-wrote locking bits so that
  allocations happen in blocks. added missing case that omnis
  database triggers.

2000-02-07  a sun  <asun@asun.cobalt.com>

* bin/nbp/Makefile (install): make nbprgstr/nbpunrgstr with 700
  permissions.
* include/atalk/adouble.h (sendfile): change to deal with
  <sys/sendfile.h>

2000-01-25  a sun  <asun@asun.cobalt.com>

* etc/afpd/ofork.c: keep track of oforks being used for each
  directory so that we can update them if the directory tree gets
  modified.
* etc/afpd/directory.c (deletecurdir): remove dangling symlinks on
  delete.

2000-01-24  a sun  <asun@asun.cobalt.com>

* etc/afpd/directory.h (CNID): moved cnid assignment here along
  with helpful macros.
* etc/afpd/directory.c: changed directory search to use red-black
  trees to improve balance. parent-child tree changed to circular
  doubly-linked list to speed up insert/remove times.  there's still
  one obstacle to actually freeing red-black tree entries. i need to
  add an ofork list to struct dir to minimize search times.

2000-01-18  a sun  <asun@asun.cobalt.com>

* etc/afpd/directory.c (dirinsert): detect attempts to add
  pre-existing entries as just symbolic links.
* etc/afpd/filedir.h (CNID): moved inode-cnid assignment here and
  extended to directories.

2000-01-03  a sun  <asun@asun.cobalt.com>

* etc/uams/uams_pam.c (PAM_conv): surround PAM_BINARY_PROMPT with
  an #ifdef.
* etc/afpd/status.c (status_init): fixed a bunch of problems here
  that manifested under solaris 7.
* etc/afpd/main.c (main): use FD_SETSIZE instead of FD_SETSIZE +
  1.

1999-12-27  a sun  <asun@asun.cobalt.com>

* libatalk/util/getiface.c: moved interface detection code to here
  so that i can use if_nameindex() or getifconf() depending upon
  what's available.

1999-12-13  a sun  <asun@asun.cobalt.com>

* libatalk/dsi/dsi_tcp.c (dsi_tcp_init): added if_nameindex()
  based interface code.
* etc/afpd/afp_options.c (afp_options_parseline): added
  -server_quantum as an option. using hex would be a good idea.
* libatalk/dsi/dsi_opensess.c (dsi_opensession): added bits to set
  the server quantum. by default, the server quantum is limited to
  1MB due to a bug in the os 9 appleshare client.
* distrib/initscripts/rc.atalk.{cobalt,redhat}: surround nbp stuff
  with double quotes.
* etc/uams/uams_dhx_pam.c (pam_changepw): added dhx-based password
  changing for pam.

1999-12-06  a sun  <asun@asun.cobalt.com>

* etc/afpd/directory.c (setdirparams): don't error if we can't set
  the desktop owner/permisssions.

1999-11-04  a sun  <asun@asun.cobaltnet.com>

* etc/afpd/fork.c (afp_openfork): had the ordering wrong on an
  openfork.

1999-11-02  a sun  <asun@asun.cobaltnet.com>

* etc/afpd/afp_dsi.c (afp_over_dsi): flush data for unknown dsi
  commands.

1999-10-28  a sun  <asun@asun.cobaltnet.com>

* etc/uams/*.c: return FPError_PARAM if the user is unknown.

1999-10-27  a sun  <asun@asun.cobaltnet.com>

* etc/afpd/fork.c (afp_read): if sendfile isn't supported, use the
  old looping method.

1999-10-25  a sun  <asun@asun.cobaltnet.com>

* libatalk/nbp/nbp_unrgstr.c (nbp_unrgstr): fix nbp unregisters.

1999-10-21  a sun  <asun@asun.cobaltnet.com>

* etc/afpd/Makefile (install): moved install of afpd earlier per
  suggestion by steven michaud.

1999-10-05  a sun  <asun@asun.cobaltnet.com>

* etc/uams/uams_randnum.c (afppasswd): for ~/.passwd's, turn
  ourselves into the user so that nfs is happy.

1999-09-19  a sun  <asun@adrian5>

* libatalk/netddp/netddp_open.c, nbp/*.c: only use the bcast stuff
  if it's on an os x server machine.

1999-09-15  a sun  <asun@adrian5>

* libatalk/nbp/nbp_unrgstr.c,nbp_lkup.c,nbp_rgstr.c: os x server
  wants ATADDR_BCAST. that probably means that i need to do
  multihoming appletalk a little differently. bleah.

1999-09-09    <asun@asun.cobaltnet.com>

* etc/afpd/directory.c (getdirparams), libatalk/adouble/ad_open.c
  (ad_open): mondo lameness. i forgot that directory lookups can be
  done with "." as the directory name. that was auto-hiding
  them. bleah. i also figured out which bit was the invisible bit
  for finderinfo information.

1999-09-06  Adrian Sun  <asun@glazed.cobaltnet.com>

* etc/afpd/desktop.c (mtoupath): fixed a bug in codepage support
  that accidentally crept in.

1999-08-31  Adrian Sun  <asun@glazed.cobaltnet.com>

* etc/afpd/quota.c (getfsquota): use group quotas in quota
  calculations if the user belongs to a single group. just use the
  user quotas if the user belongs to multiple groups.
* etc/afpd/volume.c (getvolspace): added an options:limitsize to
  restrict the available space to 2GB. this is for macs running
  older versions of the operating system with newer versions of the
  appleshare client. weird huh?
* etc/afpd/quota.c (uquota_getvolspace): bleah. 64-bit shifts
  don't get promoted in the same way as arithmetic operations. added
  some more casts to deal with that issue.

1999-08-24  Adrian Sun  <asun@glazed.cobaltnet.com>

* man/man?/Makefile: don't re-build .tmp files if they already
  exist. this gets the install phase to work correctly.

1999-08-13  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/directory.c, file.c, filedir.c: illegal characters get
  AFPERR_PARAM. also, reject names with /'s in them if the nohex
  option is used.

1999-08-12  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/filedir.c,file.c,directory.c: changed error for
  illegal filenames to AFPERR_EXIST.

1999-08-11  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/desktop.h (validupath): if usedots is set, .Apple* and
  .Parent files are no longer valid file names.
* etc/afpd/volume.c (volset): added usedots and nohex as
  options. usedots stops :hex translation of . files while nohex
  stops :hex translation of everything but . files. in addition,
  . files created on the unix side are by default hidden.
* libatalk/adouble/ad_open.c: initialize more bits.

1999-08-10  a sun  <asun@hecate.darksunrising.blah>

* distrib/initscripts/rc.atalk.redhat (WORKSTATION): use the
  actual name for nbp registration rather than ATALK_NAME.
* sys/solaris/Makefile (kernel): make sure osdefs and machinedefs
  get used when building the kernel module.
* sys/solaris: changed strings.h to string.h

1999-08-08  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (readvolfile): changed volume options into an
  array of structs to ease maintenance.

1999-08-05  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/status.c (status_init): change the default icon
  depending upon whether or not it's an ASIP or an AppleTalk
  connection.

1999-08-04  Adrian Sun  <asun@glazed.cobaltnet.com>

* etc/atalkd/main.c (setaddr): made a failure with setaddr a
  little more informative.

1999-08-03  Adrian Sun  <asun@glazed.cobaltnet.com>

* yippee. someone figured what was happening with the installation
  of the man pages. i got rid of a duplicate entry.

1999-08-02  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (readvolfile): added a per-file way of setting
  default options. it keys in on a :DEFAULT: label.

1999-07-30  a sun  <asun@hecate.darksunrising.blah>

* moved rc.atalk.* scripts to distrib/initscripts.

1999-07-27  a sun  <asun@hecate.darksunrising.blah>

* contrib/printing: added patch from <job@uchicago.edu>
* etc/afpd/file.c: forgot to initialize struct ad in
  some places.
* etc/afpd/nls/makecode.c: added an empty mapping.
* etc/psf/Makefile (install): well cp -d didn't work either. just
  use tar.

1999-07-26  a sun  <asun@hecate.darksunrising.blah>

* sys/solaris/tpi.c (tpi_attach): changed DDI_NT_NET to DDI_PSEUDO
  (from <denny@geekworld.com>).
* distrib/rpm/netatalk-asun.spec (Summary): incorporated new spec
  and patch files from inoue.
* sys/linux/Makefile (install-sysv): fixed up a bit.
* etc/psf/Makefile (install): use cp -d instead of cp -a to make
  *bsd happier.
* etc/afpd/afp_options.c (afp_options_parseline): reversed meaning
  of -icon. now it means to use the yucky bitmap instead of the
  apple icon.
* bin/afppasswd/Makefile (all): add -Iinclude/openssl for
  afppasswd as well.

1999-07-18  a sun  <asun@hecate.darksunrising.blah>

* create links/mangle files in the compile rather than the install
  phase so that rpm will be happier.

1999-07-17  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/file.c (afp_createfile), directory (afp_createdir),
  filedir.c (afp_rename, afp_moveandrename): don't allow the
  creation/renaming of names with certain characters if mswindows
  compatibility is enabled.

1999-07-16  a sun  <asun@hecate.darksunrising.blah>

* rc.atalk.redhat: incorporated chkconfig from inoue.

1999-07-15  a sun  <asun@hecate.darksunrising.blah>

* etc/atalkd/config.c (getifconf): wrap check against
  IFF_MULTICAST behind an #ifdef IFF_MULTICAST.
* sys/netbsd/Makefile (LDSHAREDFLAGS): key in on machine type.

1999-07-11  a sun  <asun@hecate.darksunrising.blah>

* contrib/ICDumpSuffixMap: added internet config perl script from
  inoue.
* contrib/printing: added contributed solaris printing scripts
  from <job@uchicago.edu>.

1999-07-10  a sun  <asun@hecate.darksunrising.blah>

* etc/atalkd/interface.h, rtmp.h: prototyped functions.
* etc/atalkd/zip.c: converted bcopy's to memcpy's.
* etc/atalkd/nbp.c,rtmp.c: added checks for the interface for
  dontroute cases.
* etc/atalkd/main.c: converted bzero/bcopy to memset/memcpy.

1999-07-08  a sun  <asun@hecate.darksunrising.blah>

* libatalk/nbp/nbp_rgstr.c (nbp_rgstr): return EADDRINUSE if the
  address already exists.

1999-07-06  a sun  <asun@hecate.darksunrising.blah>

* rc.atalk.redhat: changed netatalk.config to netatalk.conf

1999-07-05  a sun  <asun@hecate.darksunrising.blah>

* etc/atalkd/nbp.c (nbp_packet): add interface to nbp struct. this
  is so that we can filter by interface in the future. however, it
  doesn't seem to work that well right now. bleah.
* etc/atalkd/main.c: fixed up dontroute option so that it doesn't
  screw up atalkd.conf. also, we need to do a bootaddr if dontroute
  is set.
* libatalk/atp,nbp,netddp; bin/aecho,nbp,getzones,pap;
  etc/papd,afpd: accept -A \<ddp address\> as an option so that you
  can specify the address to be used on a multihomed server. for
  papd, you use the 'pa' option in papd.conf.

1999-07-04  a sun  <asun@hecate.darksunrising.blah>

* etc/atalkd/config.c (parseline): initialize parseline properly
  so that we don't get extraneous junk.
* etc/afpd/afp_options.c (afp_options_parseline): do
  gethostbyaddr/gethostbyname's for -ipaddr and -fqdn.
* etc/atalkd/config.c (getifconf/readconf): check to see if the
  supported device can support appletalk addresses. either continue
  or exit depending upon whether or not it's auto-configed.

1999-07-03  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/afp_options.c (afp_options_parse): -I (-[no]icon) will
  toggle the volume icon so that it uses the apple icon instead.
* etc/afpd/config.c (AFPConfigInit): added more logic for the
  -proxy option. here are the rules: -proxy will always try to
  create a DDP server instance. by default, the proxy server will
  still allow you to login with an appletalk connection. to prevent
  that, just set the uamlist to an empty string.

1999-07-02  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/status.c (status_netaddress): added support for fqdn
  (not available in the appleshare client yet).

1999-07-01  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/config.c (DSIConfigInit): application code for proxy
  setup. it's the -proxy option.
* libatalk/dsi/dsi_init/tcp.c (dsi_init/dsi_tcp_init): added
  support for proxy setup.

1999-06-30  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/filedir.c (afp_rename): fixed up some error
  codes. quark express should be happier.
* etc/afpd/uam.c (uam_afpserver_option): added
  UAM_OPTION_HOSTNAME. use this to set PAM_RHOST. i just got a
  report that setting that fixes pam on solaris machines.

1999-06-28  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/ofork.c (of_alloc): report out of forks in syslog..
* etc/afpd/enumerate.c (afp_enumerate): close an opendir leak.
* include/atalk/{dsi,asp}.h: make cmdlen and datalen ints.
* etc/afpd/fork.c (afp_write): fixed up error condition.

1999-06-26  a sun  <asun@hecate.darksunrising.blah>

* etc/uams/Makefile (install): changed install location of uams.
* sys/linux/Makefile (install-sysv): always install redhat
  script. netatalk.config script only gets installed if it's not
  there already.

1999-06-23  a sun  <asun@hecate.darksunrising.blah>

* rc.atalk.redhat: merged in redhat contrib rpm rc.atalk script.
* etc/afpd/afp_options.c (afp_options_init): changed default
  maxusers to 20.

1999-06-22  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/config.c (DSIConfigInit): truncate options->server to
  just the server name here.
* etc/afpd/volume.c (volxlate): made $s return something
  meaningful no matter what.
* libatalk/adouble/ad_sendfile.c (ad_readfile): freebsd sendfile
  wants an off_t.

1999-06-20  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (volxlate): added variable substitution. if it
  doesn't understand the variable, it just spits it back out.

  (creatvol): display truncated volume name if it's too long.
* sys/{generic,solaris}/Makefile: added NO_CRYPTLIB option to deal
  with oses that have -lcrypt but shouldn't use it.

1999-06-11  a sun  <asun@hecate.darksunrising.blah>

* include/atalk/afp.h: added comments to FPErrors.
* etc/afpd/enumerate.c (afp_enumerate): make FPEnumerate do some
  more error checking.
* include/atalk/util.h: server_lock() returns pid_t.

1999-06-10  a sun  <asun@hecate.darksunrising.blah>

* README.ASUN: added location for both ssleay and openssl.
* etc/uams: moved install to LIBDIR/uams. "uams_*" now means "uam
  server." in the future, there will be "uamc_*." changed the shared
  library names to match.
* include/atalk/atp.h,nbp.h: forgot to include <sys/cdefs.h>
* etc/uams/Makefile: openssl-0.9.3c uses <openssl/*.h> so add that
  to the include path.
* sys/{solaris,ultrix}/Makefile: just use -I../sys/generic instead
  of doing a link.
* include/atalk/uam.h, etc/uams/uam_*.c, etc/afpd/uam.c: added uam
  type field. do type check.
* etc/uams/uam_*pam.c: added a couple more error codes.

1999-06-08  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/nls/Makefile (codepage.h): make sure that a link to
  codepage.h gets made.
* libatalk/*/Makefile: make sure that the profiled directory gets
  created.
* etc/afpd/directory.c (afp_mapname): removed an extraneous line
  that was causing mapname to fail.

1999-06-07  a sun  <asun@hecate.darksunrising.blah>

* etc/atalkd/main.c (main): added a note to check the syslog if
  atalkd can't be setup.
* sys/linux/Makefile: added -DNEED_QUOTACTL_WRAPPER to the list of
  auto-detected #defines.

1999-06-06  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/fork.c (afp_write): argh. i moved things around a
  little too much and ended up with an uninitialized eid. strangely,
  the compiler didn't complain. simplified bits a little as
  well. also, FPWrite was returning the wrong error messages. on
  64-bit filesystems, the offset can wraparound. so, report a disk
  full error if that's going to happen. egcs-19990602 gets one
  memcpy right and another wrong on my udb. bleah.

  (afp_read): fixed the error messages here as well.

1999-06-05  a sun  <asun@hecate.darksunrising.blah>

* Makefile, sys/generic, sys/{ultrix,solaris}/Makefile: create
  some links on the fly if they're missing.
* etc/afpd/directory.c (copydir): fixed a leaking opendir and
  re-arranged a little.

1999-06-04  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd: prototyped everything here and moved the FP functions
  into include files.
* libatalk/util/bprint.c: moved all of the bprints to here.
* libatalk/asp, include/atalk/asp.h: prototyped asp functions.
* include/atalk/atp.h, libatalk/atp: prototyped atp functions.
* libatalk/nbp, include/atalk/nbp.h: added prototypes for nbp
  functions.
* bin/afppasswd/Makefile (afppasswd): fixed a misspelling in the
  install phase.
* bin/afppasswd/afppasswd.c: added -a option so that root can add
  new users. turned all of the options into bits. added newlines to
  each entry.

1999-06-03  a sun  <asun@hecate.darksunrising.blah>

* sys/freebsd/Makefile: turn on sendfile support if running on a
  FreeBSD 3+ machine.

1999-06-02  a sun  <asun@hecate.darksunrising.blah>

* etc/uams/uam_dhx_pam.c: fixed memory freeing part of pam
  conversation function.
* sys/*/Makefile: check at make time to see if -lrpcsvc and
  -lcrypt should be included in the appropriate places.

1999-05-28  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/file.c (deletefile): added more error checking here as
  well.
* etc/afpd/directory.c (renamedir): added a couple a few more
  error bits.
* sys/sunos/Makefile: sunos should really work now.

1999-05-27  a sun  <asun@hecate.darksunrising.blah>

* include/atalk/afp.h: added in a couple new error codes (one
  deals with password setting policy, the other with maximum logins
  by any user).
* etc/afpd/fork.c (afp_openfork): try to re-open files on
  read-only volumes as read-only.

1999-05-26  a sun  <asun@hecate.darksunrising.blah>

* sys/solaris/Makefile: fixed a few bobbles here. solaris uses
  uname -p. other oses seem to use uname -m for the same information.
* etc/uams/uam_pam.c (pam_changepw): added check for same
  password.
* etc/uams/uam_randnum.c (randnum_changepw): added in cracklib and
  same password checks.
* sys/osx/Makefile: moved the os x server stuff into its own build
  directory.
* sys/linux/Makefile, sys/solaris/Makefile: key in on OSVERSION
  and MACHINETYPE for some stuff.

1999-05-25  a sun  <asun@hecate.darksunrising.blah>

* sys/sunos/Makefile: various bits to make stuff work with sunos
  again.

1999-05-25  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/file.c (copyfile): only copy the header file if newname
  is specified.
* etc/afpd/directory.c (copydir): make sure to balk if the
  directory already exists. in addition, make sure to preserve the
  timestamps.

1999-05-24  a sun  <asun@hecate.darksunrising.blah>

* bin/afppasswd/afppasswd.c: global password updating utility for
  the randnum authentication method.

1999-05-22  a sun  <asun@hecate.darksunrising.blah>

* etc/uams/uam_randnum.c (afppasswd): added in global password
  file for the randnum authentication method. it looks for a .key
  file as well to handle encryption.
* etc/afpd/afp_options.c (afp_options_parseline): added
  -passwdfile as an option so that you can specify a global randnum
  password file if desired.
* etc/afpd/volume.c (readvolfile): we now have rwlist and rolist
  as an AppleVolumes.* option. if the user is in the rolist, the
  volume gets set as readonly. if there's a rwlist, and the user
  isn't in it, the volume also gets set as readonly.

1999-05-21  a sun  <asun@hecate.darksunrising.blah>

* libatalk/adouble/ad_lock.c (ad_fcntl_lock): plug a leak if we
  can't allocate the reference counting variable.
* etc/uams/uam_*.c: make sure that uam_setup returns an error
  code.

1999-05-19  a sun  <asun@hecate.darksunrising.blah>

* include/atalk/paths.h (_PATH_LOCKDIR): added os x server's
  /var/run as the lock file directory.
* etc/afpd/fork.c (afp_write): <kanehara@tpk.toppan.co.jp> reported
  a problem with FPWrite getting a request count of 0. that's
  fixed.
* etc/afpd/Makefile: bleah. for some reason, pam doesn't like to
  load itself from a shared library. i've compensated by linking it
  into afpd again.
* etc/uams/uam_dhx_passwd.c: okay. DHX now works. something's
  still screwy with the dhx_pam stuff though.

1999-05-18  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/uam.c (uam_getname): i forgot that getname modified the
  username to fit what's in pw->pw_name if necessary.

1999-05-16  a sun  <asun@hecate.darksunrising.blah>

* etc/uams/uam_dhx_passwd/pam.c: almost ready versions of the DHX
  authentication method. i'm still missing a little info to get it
  all right.
* bin/megatron/nad.c (nad_header_read): if there isn't a mac name,
  create it from the unix name.
* bin/megatron/megatron.c (megatron): oops. need to turn fdCreator
  and fdType into strings.

1999-05-16  a sun  <asun@pelvetia>

* etc/afpd/uam.c (uam_afpserver_option): changed the interface a
  little. now, you pass in an int * if you want to either get/set
  the size of the option. added in UAM_OPTION_RANDNUM for generic
  (4-byte granularity) random number generation.
* etc/afpd/switch.c: added afp_logout to preauth_switch.

1999-05-15  a sun  <asun@hecate.darksunrising.blah>

* bin/megatron/macbin.c (bin_open): make error message for
  macbinary files more informative.

  (test_header): added more macbinary tests. it now has a workaround
  for apple's incorrectly generated macbinary files.

1999-05-14  a sun  <asun@hecate.darksunrising.blah>

* sys/solaris/Makefile: added shared library generation bits.
* etc/uams: moved server-side uams here.
* include/netatalk/endian.h: fixed some solaris bits.
* etc/afpd/config.c (configfree): don't do an asp_close. instead,
  do an atp_close and free the asp object. oh yeah, as afpd needs
  to export symbols to its modules, make sure you don't do anything
  more exciting than strip \-\-strip-debug with it.

1999-05-12  a sun  <asun@hecate.darksunrising.blah>

* various places that use sigaction: zero out struct sigaction so
  that we don't send something confusing. also make sure that we
  don't set a timer unless we already have a sigaction set.
* etc/afpd/fork.c (afp_openfork): don't error on trying to open an
  empty resource fork read-only. also, added back in the bit of code
  that prevented locks from being attempted on non-existent resource
  forks.
* etc/afpd/afp_options.c (getoption): added a uamlist commandline
  option (-U list).
* libatalk/netddp/netddp_open.c: don't bind if nothing was passed
  in.
* libatalk/nbp/nbp_unrgstr.c (nbp_unrgstr): oops. forgot to
  convert this over to use by the netddp interface.

1999-05-12  a sun  <asun@pelvetia>

* etc/afpd/uam.c: os x server's runtime library loader is
  braindead. as a result, i've switched to using an exported struct
  with the uam's name.
* bin/aecho,getzones: changed these to use the netddp interface.
* libatalk/nbp/nbp_rgstr.c,unrgstr.c: fixed more leaky bits.
* libatalk/netddp: abstracted the ddp interface to netddp. besides
  the prior socket-driven interface, there's now an os x server
  interface. so, instead of calling socket/sendto/recvfrom, you call
  netddp_open/netddp_sendto/netddp_recvfrom.

1999-05-11  a sun  <asun@pelvetia>

* libatalk/nbp/nbp_lkup.c: oh my. nbp_lookup was fd leaky if there
  was a problem.
* etc/atalkd/main.c (main): make sure that if -dontroute is
  selected for all but one interface, that interface also gets
  -dontroute set.

1999-05-10  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/auth.c: re-wrote to deal with plug-in uams. it's much
  smaller than it used to be.

1999-05-09  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/uams/uam_guest.c, uam_pam.c, uam_passwd.c,
  uam_randnum.c: uam modules. these should probably be moved out of
  afpd (and into something like etc/uam_server) when the printing
  stuff gets uam support.

1999-05-08  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/uam.c: interface to user authentication modules.
  it should eventually be moved to libatalk, but that's not
  necessary until the printing uam stuff is done. everything is from
  the server-side perspective, but that's only because there aren't
  any client-side uses right now.
* libatalk/util/module.c: generic interface to run-time library
  loading functions. right now, the dlfcn family and os x server's
  NS-style way of doing things are the ones understood. in addition,
  there's a DLSYM_PREPEND_UNDERSCORE for those systems that need it.
* libatalk/asp/asp_write.c (asp_wrtcont): log both the read and
  write part of write continuations.

1999-05-07  a sun  <asun@hecate.darksunrising.blah>

* etc/atalkd: added the ability to turn off routing for particular
  interfaces. specify -dontroute for each interface that you don't
  want to route.

1999-05-06  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/auth.c: got rid of global clrtxtname and switched to
  using obj->username.

1999-05-04  a sun  <asun@hecate.darksunrising.blah>

* libatalk/dsi/dsi_write.c (dsi_write): dsi_write could loop
  forever if there's a problem while it's being used. that's fixed.

1999-05-01  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/directory.c (renamedir,copydir,deletedir): added bits
  so that renaming a directory works across filesystems.

1999-04-27  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/fork.c (getforkparams): report mtime if it's > than
  what's stored in the header file.
* config/afpd.conf: incorporated a patch by Stefan Bethke to make
  afpd.conf more understandable.
* sys/solaris/if.c: many of the firstnet/lastnet bits weren't
  endian converted. that's fixed.
* libatalk/adouble/ad_lock.c (adf_find(x)lock): F_RD/WRLCK aren't
  necessarily ORable, so use ADLOCK_RD/WR instead.

  (ad_fcntl_unlock): erk. fixed a typo that had the resource fork
  unlock accidentally getting the data fork locks.

1999-04-24  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/fork.c (afp_openfork): always try to create a resource
  fork if asked.

1999-04-21  a sun  <asun@hecate.darksunrising.blah>

* libatalk/adouble/ad_open.c, ad_read.c/ad_write.c, ad_flush.c:
  turned the mmapped header stuff into and #ifdef
  USE_MMAPPED_HEADERS option.
* libatalk/adouble/ad_open.c (ad_header_read): darn. i forgot that
  the hfs fs doesn't currently have mmappable header files. rather
  than implement that, i just reverted back to a modified version
  of the old way of reading headers.

1999-04-15  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/fork.c (afp_bytelock): byte locks become read locks on
  read-only files.

  (afp_openfork): deal with read-only data forks that don't have
  corresponding .AppleDouble files. we can't really do anything with
  deny locks in this case. just make sure that read locks are set.
* etc/afpd/file.c (getfilparams): oops. got the parentheses wrong
  around FILPBIT_FINFO.
* etc/afpd/fork.c (afp_read): as we share open files now, check
  for fork type against of_flags instead of just checking to see if
  the file is open. this fixes a bug that caused resource forks to
  get filled with data fork information.

1999-04-09  a sun  <asun@porifera.zoology.washington.edu>

* sys/generic/Makefile: AFP/tcp now compiles on irix with quota
  support.

1999-04-09  a sun  <asun@mead1.u.washington.edu>

* sys/generic/Makefile: AFP/tcp now compiles on aix with quota
  support.

1999-04-09  a sun  <asun@saul6.u.washington.edu>

* sys/generic/Makefile: AFP/tcp part now compiles on digital unix
  with quota support enabled.

1999-04-08  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c, fork.c, file.c, directory.c, filedir.c,
  config/AppleVolumes.default: added read-only volume option.
* etc/afpd/quota.c (uquota_getvolspace): modified for os x
  server.

1999-04-03  a sun  <asun@hecate.darksunrising.blah>

* bin/megatron/macbin.c (bin_write): only pad if we need to do so
  (from <jk@espy.org>).
  (bin_header_write/read): fixed up screwed up file date
  generation/reading with macbinary files.
* bin/megatron: changed all of the bcopy/bzero/bcmp's to
  memcpy/memset/memcmp's. added macbinary III support.
* bin/megatron/macbin.c (bin_open): added \-\-stdout as an option so
  that we can stream macbinary file creation to stdout.
* bin/megatron/megatron.c: incorporated information patch (\-\-header
  and \-\-macheader) from <fmorton@base2inc.com>.

1999-04-02  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd: whee! there are no more bcopy/bcmp's in this
  directory.
* libatalk: changed the bcopy/bzero's to memcpy/memset's. added in
  dummy ints for some of the files that can get compiled to empty
  objects. check for the type of msync() available as well.

1999-03-31  a sun  <asun@hecate.darksunrising.blah>

* INSTALL/README.GENERIC: added information for a generic
  architecture. It includes the information needed to get netatalk
  to compile on a random unix platform.
* etc/afpd/quota.c: moved the quota stuff here so that we can
  #ifdef it out on a machine without quota support.

1999-03-30  a sun  <asun@hecate.darksunrising.blah>

* libatalk/adouble/ad_lock.c: reference count the locked ranges as
  well. this prevents multiple read locks on the same byte range
  from disappearing if one user disappears.

  (ad_fcntl_lock): here are the current rules for file
  synchronization:
     1) if there's a appledouble header, we use the beginning
        of that for both data and resource forks.
     2) if there isn't, we use the end of the data fork (or past the
        end on 64-bit machines)

1999-03-28  a sun  <asun@hecate.darksunrising.blah>

* libatalk/adouble/ad_open.c (ad_refresh): okay. mmapping the
  appledouble entry parts is done.
* libatalk/cnid/cnid_add.c (cnid_add): prevent anyone from adding
  in an illegal cnid.

1999-03-27  a sun  <asun@hecate.darksunrising.blah>

* libatalk/adouble/ad_open.c (ad_refresh): started making the
  appledouble header parsing more generic so that we can read in
  arbitrary appledouble header files. i just mmap the parts that we
  need.

1999-03-22  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/file.c (afp_copyfile): return the correct error
  response on a failed copy. also, error if the file is already open
  to prevent problems with locks. we really need to ad_lock
  this during the copy

1999-03-21  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (readvolfile): switched volume options to
  using ':' as a delimiter as that's one of the characters that's
  not allowed as part of a mac filename.
  (volset): changed access to allow/deny
* etc/afpd/auth.c (noauth_login): make sure that the username gets
  set.

1999-03-17  a sun  <asun@hecate.darksunrising.blah>

* NOTE to myself: jeremy allison said that samba uses refcounts to
  prevent close() from killing all the byte locks. so, i've started
  converting afpd to using refcounting as well. luckily, we already
  have of_findname, so we know when files are open. in cases where
  files are already open, this will replace an ad_open with a lookup
  into a hash table.
* etc/afpd/directory.c (getdirparams/getfilparams): check for
  NULL names when getting directory/file names.
* etc/afpd/directory.{c,h} (DIRDID_ROOT/DIRDID_ROOT_PARENT): make
  sure these are always in network byte order.

1999-03-15  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/fork.c (afp_openfork): okay, fixed the file
  synchronization bits. now, we use two bytes to do the read/write
  stuff. when access is needed, a read lock is attempted. if a deny
  lock is needed, a write lock is attempted. we even handle the
  access None mode now by saving the access modes.
* etc/afpd/fork.h (AFPFORK_ACCMASK): started adding bits so that
  we can obey all of the file synchronization rules.
* etc/afpd/fork.c (afp_bytelock): got the meaning of the clearbit
  reversed. with helios lantest's lock/unlock 4000 times test, it
  looks like i get <1 second overhead on my machine when using byte
  locks. NOTE: this will get a little worse when serialization gets
  added. in addition, 0x80000000 only works for 64-bit machines. i
  reserve the last allowable bit for 32-bit machines.

  actually, both 64-bit machines and 32-bit machines use 0x7FFFFFFF
  now as i'm able to trigger a kernel oops in linux with the 64-bit
  code.

  (afp_read/afp_write): make sure to use the same offset when doing
  a tmplock.

1999-03-14  a sun  <asun@hecate.darksunrising.blah>

* libatalk/adouble/ad_lock.c: i went and implemented a bunch of
  stuff to get byte locks to work correctly (except for the
  serialization) only to discover that files that use byte locks
  also request a deny write mode. luckily, byte locks only cover up
  to 0x7FFFFFFF. so, i'll just use 0x80000000 for the
  synchronization locks.

1999-03-08  a sun  <asun@hecate.darksunrising.blah>

* sys/{*bsd,ultrix,solaris,linux}/Makefile (depend): surround
  DEPS with double quotes so that multiple defines work.

1999-03-06  a sun  <asun@hecate.darksunrising.blah>

* libatalk/adouble/ad_write.c, ad_read.c: make off off_t in size.
* libatalk/adouble/ad_flush.c (adf_fcntl_relock), ad_lock.c
  (adf_fcntl_locked): okay. fcntl locks now check against multiple
  programs on the same machine opening the same file. current
  problems with the mechanism that i don't want to fix unless
  necessary:
    1) there's a race during the relock phase. serialization
       would solve that.
    2) it assumes that each fd only locks a single contiguous
         range at a time. keeping a list of locked ranges would
       solve that.

  also, i changed the default to using fcntl locks. if the above two
  are really necessary, i'll probably switch to something a little
  more featureful like the berkeley db's lock manager.

  (note to myself: stuff new from asun2.1.3 from 1999-03-03)

1999-03-05  a sun  <asun@hecate.darksunrising.blah>

* libatalk/adouble/ad_lock.c: got rid of the endflag checks to
  reduce system calls a little.
* etc/afpd/auth.c (getname): do a case-insensitive compare on the
  login name as well.
* sys/solaris/Makefile: added 64-bit solaris patch from
  <jason@pattosoft.com.au>.

1999-03-03  a sun  <asun@hecate.darksunrising.blah>

* include/netatalk/endian.h: make solaris 2.5 complain less.
* bin/adv1tov2/adv1tov2.c, libatalk/adouble/ad_open.c (ad_v1tov2):
  fixed a couple problems with the adv1tov2 stuff.

1999-02-26  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (afp_openvol): erk. the volume password gets
  aligned along an even boundary.

1999-02-23  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (readvolfile): added volume password support.

1999-02-14  a sun  <asun@hecate.darksunrising.blah>

* etc/atalkd/multicast.c (addmulti): added FreeBSD's data-link
  layer multicast setting bits.
* libatalk/adouble/ad_open.c (ad_v1tov2): make sure to stick in
  prodos field info when converting.
* rc.atalk.redhat: added pidof checking in case the machine
  crashes. also added rc.atalk.wrapper to the redhat rc script
  installation.

1999-02-07  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/fork.c (afp_setforkparams): make sure to do better
  error detection here and more fully report error conditions.

  (flushfork): make sure to flush the header if necessary (rfork
  length changed or modification date needs to be set).

  (afp_write): ugh. this wasn't returning the right values for the
  last byte written if the endflag was set. in addition, it was
  setting the modification date. that should be left to FPCloseFork
  and FPFlush(Fork). this fixes a problem that shows up with
  QuarkXPress.

  NOTE: as of now, changes to the rfork info are the only things
  that aren't flushed immediately when altered.
* etc/afpd/fork.c (get/setforkparams), ofork.c: what ugliness. we
  need to report bitmap errors if we try to fiddle with the wrong
  fork. i added an of_flags field to keep things sorted.
* libatalk/adouble/ad_open.c (ad_v1tov2): oops. in all of the
  movement, i forgot to make sure that the pre-asun2.2.0 features
  still compile.

1999-02-06  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/filedir.c (afp_moveandrename): make sure to save the
  old name even when it's a directory.
* globals.h: added oldtmp and newtmp to AFPObj to reduce the
  number of buffers used. use these when needed in afp_* calls.
* etc/afpd/directory.c (deletecurdir): delete stray .AppleDouble
  files when deleting a directory.

1999-02-05  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/file.c (afp_createfile): fixed a hard create error
  check bug.
* fixed up a few bobbles in the netatalk-990130 merge.
* the noadouble option should be pretty much implemented. here's
  how it goes:
    when a directory is created, the corresponding
    .AppleDouble directory is not.

    comments, dates, and other file attributes will get
    silently ignored and not result in the creation of a new
    .AppleDouble directory.

    however, if anything possessing a resource fork is copied
    into that directory, the corresponding .AppleDouble
    directory will be created. once this happens, any
    other file in the directory can acquire an AppleDouble
    header file in the future.

1999-02-03  a sun  <asun@hecate.darksunrising.blah>

* merged in the rest of netatalk-990130.
* sys/solaris: merged in netatalk-990130 changes.
* etc/afpd/file.c,fork.c,desktop.c libatalk/adouble/ad_sendfile.c:
  tested and fixed the sendfile bits on linux. it looks like linux
  doesn't permit socket -> file sendfiles yet.
* etc/afpd/fork.c (afp_read): we can't stream FPRead's with
  newline character checking on.

1999-02-02  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/fork.c (afp_flush), ofork.c (of_flush): FPFlush
  operates on a per-volume basis.

1999-01-31  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/file.c (copyfile): sendfile bits added here also.
* etc/afpd/desktop.c (afp_geticon): added sendfile bits here as
  well.
* libatalk/adouble/ad_sendfile.c (ad_writefile/ad_readfile):
  implemented sendfile bits. currently, there's support for linux
  and freebsd. unfortunately, freebsd's implementation doesn't allow
  file->file or socket->file copies. bleah.

1999-01-30  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/file.c (setfilparams), directory (setdirparams),
  volume.c (volset): added in the beginnings of a NOADOUBLE option
  for those that don't want AppleDouble headers to be created by
  default. it doesn't really work that well right now.

1999-01-29  a sun  <asun@hecate.darksunrising.blah>

* libatalk/adouble/ad_open.c (ad_v1tov2): separated v1tov2 bits
  from ad_refresh. made broken v1 header checking the default. fixed
  broken v1 date checking. now, it just checks to see if the v1
  MDATE is > than st_mtime by 5 years.
* etc/afpd/directory.c: make date setting alter directory dates as
  well.

1999-01-24  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/afp_dsi.c (alarm_handler,afp_over_dsi): added a
  CHILD_RUNNING flag to prevent afpd from timing out on long copies.

1999-01-21  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (afp_openvol), libatalk/cnid/cnid_nextid.c:
  shift the beginning of the fake did's if necessary.
* libatalk/adouble/ad_open.c (ad_refresh): fixed screwed-up date
  detection code.
* libatalk/cnid/cnid_add.c,cnid_open.c,cnid_close.c: made some
  changes so that the CNIDs will still work even when the .AppleDB
  directory is read-only. if you're still allowed to create files on
  these volumes, you'll get a temporary id for those.

1999-01-20  a sun  <asun@hecate.darksunrising.blah>

* libatalk/cnid/{cnid_close.c,cnid_open.c}: added bits so that log
  files get cleared out on cnid_close() if it's the last user for a
  volume.

1999-01-18  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (afp_setvolparams): added FPSetVolParms. this
  allows us to set the backup date on the server.
* etc/afpd/file.c (afp_exchangefiles): whee! we now have
  FPExchangeFiles. we also have FPDeleteID, so that only leaves us
  with FPCatSearch to do.

1999-01-16  a sun  <asun@hecate.darksunrising.blah>

* sys/solaris/ddp.c (ddp_rput): added a couple htons()'s for the
  net addresses.

1999-01-11  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (volset, afp_openvol): you can now specify a
  dbpath= in AppleVolumes.* for the CNID database.
* libatalk/adouble/ad_open.c (ad_refresh): if we build in an
  appledouble v1 environment, turn on v1compat by default.

1999-01-10  a sun  <asun@hecate.darksunrising.blah>

* libatalk/adouble/ad_open.c (ad_refresh): added v1compat option
  to handle broken ad headers.
* etc/afpd/file.c (setfilparams): we need to make sure that we
  flush the file if we've created it even if there's an error.  the
  magic number/version don't get saved if we don't.
* etc/afpd/appl.c, etc/afpd/directory.c, etc/afpd/desktop.c: added
  DIRBITS to mkdirs.

1998-12-30  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (afp_openvol): got rid of unnecessary v_did.
* etc/afpd/file.c (afp_resolveid, afp_createid): added these two
  in.
* well, what do you know? the cnid stuff compiles in.

1998-12-29  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/fork.c, directory.c, file.c, filedir.c, volume.c,
  enumerate.c: added in stubs for CNID database conditional on
  AD_VERSION > AD_VERSION1.
* etc/afpd/nls/makecode.c: added iso8859-1 mapping.

1998-12-27  a sun  <asun@hecate.darksunrising.blah>

* bin/adv1tov2/adv1tov2.c: turn non-printable ascii characters
  into hex code as well.

1998-12-21  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/auth.c: fixed FPChangePW for 2-way randnums.

1998-12-15  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/fork.c (read_file/write_file): do crlf translation in
  both directions so that we can recover from problems if
  necessary.

1998-12-14  a sun  <asun@hecate.darksunrising.blah>

* bin/adv1tov2/adv1tov2.c: small utility program that recursively
  descends a directory and converts everything it sees into
  AppleDouble v2.

1998-12-13  a sun  <asun@hecate.darksunrising.blah>

* libatalk/adouble/ad_flush.c (ad_rebuild_header): moved the
  header rebuilding here so that ad_refresh can use it.
* libatalk/adouble/ad_open.c (ad_refresh): added locking to v1->v2
  conversion.
* bin/megatron/asingle.c: yuk. removed all of
  the duplicate stuff here and had it use the #defines in adouble.h.
* libatalk/adouble/ad_open.c (ad_refresh): finished v1 -> v2
  conversion routine. we still need a shortname creator and a cnid
  database for the v2 features to be useful.

1998-12-11  a sun  <asun@hecate.darksunrising.blah>

* libatalk/adouble/ad_open.c (ad_refresh): punt if we get a file
  that we don't understand.

1998-12-10  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/desktop.c (utompath,mtoupath): simplified the codepage
  stuff. also made sure to lower/upper the 8-bit characters as
  well.
* libatalk/util/strdicasecmp.c: the casemapping had a few wrong
  characters.
* etc/afpd/fork.c (getforkparams): make sure that the ROpen/DOpen
  attribute bits are in the correct byte ordering.

1998-12-09  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (volset): made prodos an option to options=
  flag. also added crlf as an option.
* libatalk/adouble/ad_open.c (ad_refresh): fix up times if
  necessary.
  (ad_open): deal correctly with 0-length files by treating them as
  newly created.
* etc/afpd/volume.c (getvolparams), file.c (get/setfilparams),
  fork.c (getforkparams), directory.c (get/setdirparams): finished
  adding appledouble version 1 and 2 date conversion. also added
  attribute setting.
* etc/afpd/volume.c (getvolparams): make sure to flush the header
  file if we needed to fiddle with it.
* libatalk/adouble/ad_date.c, ad_attr.c: date/attribute
  setting/retrieval code.
* libatalk/adouble/ad_open.c (ad_open): initialize date
  structures here instead of elsewhere.

1998-12-07  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/directory.c, fork.c, volume.c, file.c: added unix<->afp
  time conversion code.

1998-12-05  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (volset): changed prodos setting to
  prodos=true.

1998-12-04  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/volume.c (volset): okay. you now have the following
  options to casefold: lowercase,uppercase,xlatelower,xlateupper
  * tolower    -> lowercases everything in both directions
  * toupper    -> uppercases everything in both directions
  * xlatelower -> client sees lowercase, server sees uppercase
  * xlateupper -> client sees uppercase, server sees lowercase

  NOTE: you only should use this if you really need to do so. this
  and the codepage option can cause mass confusion if applied
  blindly to pre-existing directories.

1998-12-03  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/desktop.c (utompath,mtoupath), etc/afpd/volume.h: added
  multiple options to casefold. bits 0 and 1 deal with MTOU, and
  bits 2 and 3 deal with UTOM. i did it that way so that you can
  casefold in one direction only or in both directions if
  desired. needless to say, setting both bits for UTOM or MTOU
  doesn't make any sense. right now, toupper takes precedence in
  MTOU, and tolower takes precedence in UTOM.

1998-12-02  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/afp_options.c (afp_options_*): added -uampath and
  -codepagepath to the list of available options. they specify the
  directories to look for uam modules and codepages,
  respectively. currently, -uampath doesn't do anything.
* etc/afpd/volume.c (readvolfile): spruced up options to
  AppleVolumes files. now you can have mtoufile=<codepage.x>,
  utomfile=<codepage.y>, casefold=\<num\> for volumes.
* etc/afpd/desktop.c (utompath,mtoupath): added
  codepage/casefolding support. casefold is currently an int that
  could have multiple actions. right now, i just lowercase in
  mtoupath and uppercase in utompath.
* etc/afpd/ofork.c (of_alloc, of_findname, of_rename): added vol
  as an additional specifier so that we don't have problems with
  the same path names on multiple volumes.

1998-11-29  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/volume.c (getvolparams): added AFP2.1 volume attribute
  bits.

1998-11-24  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/atalkd/config.c (readconf, getifconf): added IFF_SLAVE to
  prevent atalkd from trying to use a slave channel.

1998-11-23  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/volume.c (getvolparams): we shouldn't set the custom
  icon bit by default on the root directory. that screws up pre-OS 8
  systems.

1998-11-19  a sun  <asun@purgatorius.zoology.washington.edu>

* libatalk/dsi/dsi_getsess.c (dsi_getsession): ignore SIGPIPE's
  so that we can gracefully shut down the server side.
* etc/afpd/afp_dsi.c (afp_over_dsi), etc/afpd/afp_options.c,
  libatalk/dsi/dsi_getsess.c (dsi_getsession),
  libatalk/asp/asp_getsess.c (asp_getsession): made the tickle timer
  interval an option (-tickleval \<sec\>).
* etc/afpd/afp_dsi.c (afp_dsi_timedown): added child.die so that
  we don't stomp on a shutdown timer. libatalk/dsi/dsi_read.c,
  dsi_write.c both save/restore the old timer, so they don't really
  care what the actual value is.

1998-11-18  a sun  <asun@purgatorius.zoology.washington.edu>

* due to the recent obsession with bug fixing and low-level dsi
  cleanups, i've decided that this should really be asun2.1.1
  instead of asun2.1.0a.

1998-11-17  a sun  <asun@purgatorius.zoology.washington.edu>

* libatalk/dsi/dsi_tcp.c (dsi_tcp_open): moved the afpd connection
  announcement here from etc/afpd/afp_dsi.c.
* libatalk/dsi/dsi_stream.c: moved all of the read/write functions
  into here as they're pretty generic. now, the protocol specific
  stuff only handles open and close.
* etc/afpd/fork.c (afp_read/write), desktop.c (afp_geticon),
  file.c (copyfile), include/atalk/dsi.h (dsi_writefile/readfile):
  added initial stubs for sendfile support. i still need to go
  through and calculate the appropriate offsets to use.
* libatalk/dsi/dsi_read.c, dsi_write.c: disable the interval timer
  instead of just ignoring it.
* etc/afpd/desktop.c (afp_geticon), etc/afpd/fork.c (afp_read),
  libatalk/dsi/dsi_read.c (dsi_readinit, dsi_readinit): modified the
  dsi_read interface to return errors so that i can kill things
  gracefully.

1998-11-16  a sun  <asun@purgatorius.zoology.washington.edu>

* libatalk/dsi/dsi_tcp.c (dsi_tcp_send/dsi_tcp_write): erk. read()
  and write() treat a return of 0 differently.

1998-11-16  a sun  <asun@hecate.darksunrising.blah>

* libatalk/dsi/dsi_read.c (dsi_readinit): make sure to stick in
  the error code.

1998-11-15  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/fork.c (afp_read): re-ordered some of the checks here
  to return earlier on 0-sized files.

1998-11-13  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/afp_dsi.c (afp_over_dsi): moved the dsi->noreply toggle
  check to here from dsi_cmdreply.

1998-11-11  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/atalkd/zip.c (zip_packet): make sure to copy multicast zone
  back out. (reported by Michael Zuelsdorff <micha@dolbyco.han.de>)

1998-11-09  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/directory.c (getdirparams): changed unknown bit reply
  code to AFPERR_BITMAP instead of AFPERR_PARAM.

1998-11-06  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/enumerate.c (afp_enumerate), directory.c (renamedir):
  fixed a couple of failed realloc leaks.
* etc/afpd/filedir.c (afp_moveandrename, afp_rename): added bits
  to deal with case-insensitive, case-preserving filesystems.

1998-10-30  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/auth.c: fixed randnum password changing check.

1998-10-27  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/atalkd/main.c: add a check for SIOCATALKDIFADDR if
  SIOCDIFADDR fails.
* etc/afpd/volume.c (getvolparams): ad_open had the wrong
  parameters.
* etc/afpd/unix.c (setdeskowner): added a little extra space to
  prevent buffer overflows here.

1998-10-26  a sun  <asun@purgatorius.zoology.washington.edu>

* sys/linux/Makefile: fixed PAM message.
* sys/solaris/Makefile: make failure to ln -s a non-fatal error.
* etc/papd/session.c, bin/pap/pap.c: changed sequence count to
  wrap from 0 -> 1 instead of from 0xFFFF -> 1.
* etc/afpd/filedir.c (afp_rename, afp_moveandrename): actually, we
  should check against the entire unix name.

1998-10-21  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/filedir.c (afp_rename, afp_moveandrename): make sure
  to check against mac name.

1998-10-19  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/auth.c (afp_changepw): make password changing deal
  correctly with "real" user names. also, moved seteuid() to before
  the pam_authenticate() bit as shadow passwords need that.
* etc/afpd/enumerate.c (afp_enumerate): make sure to check the mac
  name against MACFILELEN.

1998-10-16  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/file.c (renamefile), filedir.c (afp_rename),
  directory.c (renamedir): use strndiacasecmp() for AFPERR_SAMEOBJ
  checks. also make sure test occurs before checking to see if the
  destination exists.

1998-10-15  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/auth.c (afp_changepw): fixed a bit of brain damage. i
  forgot that password changing needs root privileges to work.
* etc/afpd/auth.c (PAM_conversation): the supplied code was
  incorrect. i cleaned it up a bit.
* sys/linux/Makefile: fixed the installation bits.

1998-10-14  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/auth.c (afp_changepw): don't kill the connection here
  if there's a problem.

1998-10-10  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/enumerate.c, fork.c, ofork.c, file.c,
  globals.h, directory.c, auth.c: #defined MACFILELEN and used
  that. also made sure that files > MACFILELEN never show up.

1998-09-25  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/{afpd,papd,atalkd}/bprint.c (bprint): got rid of the
  spurious pointer dereference.
* etc/afpd/ofork.c (of_alloc/of_rename): allocate the max mac file
  length so that we don't need to realloc.
* etc/afpd/filedir.c (afp_rename, afp_moveandrename): make sure to
  return AFPERR_BUSY if the dest has an ofork open.
* etc/afpd/file.c (renamefile), directory.c (renamedir), filedir.c
  (afp_rename): return AFPERR_SAMEOBJ if source == dest

1998-09-21  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd: went through and looked for places that needed to use
  curdir instead of dir. i think i have them all right now.
* etc/afpd/filedir.c (afp_moveandrename): wasn't keeping track of
  curdir correctly. what this really means is that cname should be
  fixed to return everything it changes as opposed to changing a
  global variable.

1998-09-19  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/config.c (configinit): do the right thing if
  AFPConfigInit fails.

1998-09-18  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/config.c (ASP/DSIConfigInit, configfree): how
  embarrassing. i wasn't doing refcounts correctly.

1998-09-17  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/nfsquota.c (getnfsquota): ultrix uses dqb_bwarn instead
  of dqb_btimelimit.
* sys/ultrix/Makefile: ultrix understands the old rquota format.
* etc/afpd/ofork.c (of_findname): erk. forgot to only search in
  the current directory.
  (of_rename): erk. changed it to handle renaming a file that has
  been opened multiple times.
* etc/atalkd: made sure that if you don't specify -router, things
  are as they were before.

1998-09-13  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/status.c (status_flags): forgot to turn on password
  setting if randnum passwords are being used.

1998-09-11  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/unix.c (setdirmode): erk. make sure only to setgid on
  directories.
* bin/aecho/aecho.c (main): incorporated -c \<num\> (ala ping) patch
  from "Fred Lindberg" <lindberg@id.wustl.edu>.

1998-09-03  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/directory.c (afp_closedir, afp_opendir): added these in
  for more AFP 2.0 compliance. unfortunately, apple's appleshare
  client doesn't like non-fixed directory ids.

1998-08-31  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/volume.c (accessvol): the accessible volume list can
  now be controlled by groups and usernames. just use something of
  the following form: @group,name,name2,@group2,name3

  NOTE: you can't have any spaces, and the parser forces you to
  include all options. in this case, there are some apple II options
  that need to be entered. they need to go away soon anyway.
* etc/afpd/auth.c (getname): oops. i forgot to copy the gecos
  field into a temporary buffer before calling strtok.

1998-08-29  a sun  <asun@hecate.darksunrising.blah>

* etc/atalkd/main.c (as_timer), rtmp.c (rtmp_delzones): when the
  last router on an interface goes down, we need to delete the
  interface's zone table.

1998-08-28  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/status.c (afp_getsrvrinfo): although it's never used,
  i've added this in to increase AFP compliance.
* etc/afpd/auth.c (afp_getuserinfo): added FPGetUserInfo -- this
  should make afpd compatible with OS 8.5's Nav Services.
* etc/atalkd/config.c,main.c: -router now specifies router mode
  with any number of interfaces.

1998-08-27  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/atalkd/main.c (as_timer): well, i figured out how to set up
  atalkd as a single-interface router. now, you can get zones with
  only single interfaces! i'm only waiting on wes for the approved
  configuration toggle.

1998-08-26  a sun  <asun@purgatorius.zoology.washington.edu>

* libatalk/adouble/ad_lock.c, include/atalk/adouble.h: turned the
  ADLOCK_* into real #defines and added translations in the
  lock-type specific functions. this should make it easier to
  recompile things without screwing up.

1998-08-26  a sun  <asun@hecate.darksunrising.blah>

* etc/atalkd/nbp.c (nbp_packet): forgot to handle another local
  zone case.

1998-08-25  a sun  <asun@hecate.darksunrising.blah>

* etc/afpd/status.c (status_server): changed status_server to
  use only the obj part of obj:type@zone-style names.
* etc/atalkd/nbp.c (nbp_packet): unregistering wasn't handling
  zones properly. it was matching on anything but the actual zone.

1998-08-18  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/auth.c (clrtxt_login): added pam_set_time(PAM_TTY) so
  that solaris' pam session setup doesn't crap out.

1998-08-17  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/atalkd/multicast.c (zone_bcast): fixed to do the right thing
  with zip multicast info.

1998-08-15  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/nfsquota.c: made the old-style rquota fields dependent
  upon -DUSE_OLD_RQUOTA and defined that for sunos. also included
  <sys/time.h> for ultrix breakage.

1998-08-13  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/filedir.c (afp_rename), etc/afpd/ofork.c (of_rename): i
  knew that speeding up of_findname would be useful. in any case, i
  discovered the source of yet another small AFP non-compliance that
  was confusing WordPerfect. on an afp_rename, we also need to
  rename the corresponding ofork. i've added an of_rename() to do
  this.

1998-08-13  a sun  <asun@hecate>

* etc/afpd/ofork.c (of_dealloc,of_alloc): sped up dealloc by
  sticking refnum in ofork.

1998-08-12  a sun  <asun@hecate>

* etc/afpd/fork.c (afp_openfork): added already open attribute
  bits.
* etc/afpd/ofork.c: added a hashed of_findname.

1998-08-06  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/fork.c (afp_openfork): fixed a problem with opening
  forks from read-only non-appledouble media.

1998-07-23  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/afs.c (afs_getvolspace), etc/afpd/volume.c
  (getvolspace): modified them to treak afs like the other
  getvolspaces w.r.t. VolSpace.

1998-07-21  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/unix.c (mountp): erk. i forgot that symlinks are often
  used for filesystems. nfs quotas sometimes failed as a
  result. that's fixed now.

1998-07-20  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/auth.c (login): added a -DRUN_AS_USER #define so that
  it's simple to run the server process as a non-root user.

1998-07-17  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/nfsquota.c (callaurpc, getnfsquota), volume.h: it turns
  out that i was opening lots of sockets with callaurpc. now, the
  socket gets saved and reused.

  NOTE: quota-1.55-9 from redhat 5.x doesn't return the correct size
  for rquota's bsize. unless fixed, rquota will report incorrect
  values.

1998-07-08  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/uam/README: added some preliminary ideas on a
  plug-in uam architecture. in addition, this should allow arbitrary
  afp function replacement. eventually, auth.c should get a
  bit cleaner.

1998-07-07  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/nfsquota.c: added headers and redefined a couple
  structure fields so that sunos4 compiles.
* libatalk/compat/rquota_xdr.c: compile if we're using glibc <
  2. this should get redhat 4.2 machines. NOTE: they're still
  missing librpcsvc.a, so they'll need to remove that from the
  etc/afpd/Makefile.

1998-07-06  a sun  <asun@purgatorius.zoology.washington.edu>

* libatalk/compat/rquota_xdr.c: it turns out that solaris is
  missing a couple functions needed for rquota support. here they
  are.
* etc/afpd/unix.c (mountp): fixed the nfs detection for
  solaris. we still need bsd and ultrix.

1998-07-05  a sun  <asun@hecate>

* include/atalk/adouble.h: marked out space for appledouble v2.

1998-07-04  a sun  <asun@hecate>

* etc/afpd: plugged up some ad_open leaks. made sure that we don't
  get negative numbers for length fields and such.

1998-07-04  a sun  <asun@hecate>

* etc/afpd/nfsquota.c (getnfsquota): added nfs rquota
  support. Robert J. Marinchick <rjm8m@majink1.itc.virginia.edu>
  provided the initial bits from the bsd quota program.
* etc/afpd/unix.c (getquota): made getquota call getfsquota or
  getnfsquota depending upon the type of fs.
* etc/afpd/unix.c (mountp/special): munged mountp and
  special to return either the nfs mount point or the fs
  device. set the vol->v_nfs flag if it's nfs.
* etc/afpd/volume.c (getvolspace): xbfree and xbtotal will now
  honor results returned from uquota_getvolspace.

1998-06-29  a sun  <asun@purgatorius.zoology.washington.edu>

* etc/afpd/file.c (copyfile): mmapping the file copy only helps on
  multiple copies. as that's not the case here, i've reverted to
  just doing read + write.

1998-06-28  a sun  <asun@purgatorius.zoology.washington.edu>

* sys/linux/Makefile: fixed the redhat-style atalk
  installation. also, it doesn't over-write an existing
  /etc/rc.d/init.d/atalk file.
* etc/afpd, libatalk/adouble/ad_write.c: removed <sys/file.h> and
  </usr/ucbinclude/sys/file.h> so that we rely upon adouble.h.

1998-06-19  a sun  <asun@purgatorius.zoology.washington.edu>

* changed sys/linux/Makefile to install a redhat-style sysv atalk
  script instead of the bsd one.
* include/atalk/adouble.h: moved same-name list stub to struct
  ad_adf.

Thu Jun 18 18:20:28 1998  a sun  <asun@purgatorius>

* changed to asunx.y.z notation as i was getting tired of
  increasing numbers. as this version is undergoing a fairly
  substantial overhaul, i bumped it to 2.1.0. don't ask why asun1.0
  never existed. i just started at 2.0.
* ofork (etc/afpd/{ofork.c,ofork.h,fork.c}: put in skeleton code
  for hashed-by-name oforks and oforks which group by name to help
  with byte locks and of_findname.
* adouble (include/atalk/adouble.h): started implementing
  appledouble v2. mostly, i added bits to headers. v2 has wonderful
  bits that should make prodos support much less clunky, allow
  persistent dids, and allow msdos support.
* finder info: added bits to directory.c and file.c describing the
  actual contents of finder info (from IM: Toolbox
  Essentials). also, set default directory view to an allowed value
  thanks to a suggestion from the appledouble v2 specs. that should
  help with read-only media.
* etc/afpd/{directory.c,volume.c,afs.c,directory.h}: added
  DIRDID_ROOT and DIRDID_ROOT_PARENT so people know that these did's
  are reserved.

Wed Jun 17 11:54:49 1998  a sun  <asun@purgatorius>

* well, i'm starting a changelog as i keep forgetting what i've
  done.
* locks: revamped them to be more in line with what should
  happen. currently, i've reverted back to flock-style locks as i'll
  need to stick in more code to get fcntl-style locks to work
  properly. specifically, i think modifying of_alloc/of_dealloc to
  keep track of locks, fds, and names should solve the problem with
  fcntl locks being process-specific and not fd specific.

UMICH Changes
=============

List of changes between version 1.2 and 1.4b2, from the historical *CHANGES* file.

Note that the version numbers indicated in the headings are those of
the prior version from which the changes were made.

Changes from the 1.4b1 release
------------------------------

* Fixed the maximum free/total volume size in afpd.
* Made ~ the default volume in afpd.
* Fixed pid file handling and changed setpgrp() to setpgid() in afpd,
  papd, and atalkd.
* Added code to afpd to set the Unix file time stamps with utime().
* Fixed a bug in papd's printcap code which limited it to 15 or so
  printers.
* Fixed papd's handling of piped printers.
* Fixed papd's handling of bad job names.
* Fixed atalkd to send NBP LKUP packets from NBP port.
* Added "sync;sync;sync" to Solaris kinstall to help with streams
  file corruption.
* Added nlocalrts to streams ddp.conf.  Thanks Thomas Tornblom.
* Fixed signed extension infinite loop in Solaris module.
* Moved all the config files to .../config.

Changes from the 1.3.3 release
------------------------------

* Added code from Sun Microsystems, Inc (OPCOM) for Solaris support.
  See COPYRIGHT.
* Added support for FreeBSD, mostly changes by Mark Dawson and Julian
  Elischer.
* All sorts of other stuff.

Changes from the 1.3.1 release
------------------------------

* Added options to psf's filter names to support accounting on HPs.
  !!! NOTE:  The location of the filters has changed, see the man
  page for where.
* Added code from Alan Cox to support Linux.
* Rewrote papd.  Now handles dropped connections better.
  Configuration has been modernized.  !!! NOTE: The format of the
  configuration file has changed, but NOT THE NAME.
* Added Kerberos support to papd.
* atalkd now removes routes on a SIGTERM.  Still can't just restart
  it, but it's closer.
* Changed atalkd and the kernel driver to remove a hack added to
  support sending to 0.255.  Now the kernel will allow multiple open
  sockets with the same port, so long as the addess associated with
  the port is different.  atalkd now opens a socket for each port on
  each interface.
* atalkd now rewrites its configuration file.  If no configuration
  file is given, one will be generated.  Permissions on the new
  configuration file will be inherited from the old one.  If there is
  no old one, permissions default to 644.  Won't rewrite the file if
  the owner doesn't have write permission.
* Removed support for the "AFS Kerberos UAM", in favor of the
  "AuthMan UAM".  Kerberos support should now be much more
  straight-forward.
* Fixed a bug in afpd which would cause incorrect group calculations
  on ultrix machines.
* Fixed a bug in afpd which causes SimpleText and some other
  applications to silently fail to write.  There's also a bug in
  MacOS, but we can't fix that.
* Fixed a bad interaction with afpd and AFS which would cause file
  writes to not propogate between AFS clients.
* !!! CHANGED the name(s) of afpd's config files.  The new files are
  AppleVolumes.system and AppleVolumes.default.  If AppleVolumes.system
  exists, it is always read, AppleVolumes.default is only read if the
  user has no AppleVolumes file.  Included a flag "-u" to indicate
  which file has precedence.  "-u" user wins, otherwise ".system"
  wins.
* Rewrote the AppleVolumes parsing code.  Now works.
* Added a filename extension mapping to afpd.  User always takes
  precedence, regardless of the "-u" flag.  Code to change the type
  of all Unix files contributed by Kee Hinckley <nazgul@utopia.com>.
* afpd now supports both UFS and AFS volumes simultaneously.  It also
  uses access() to attempt to calculate reasonable Mac permissions
  for AFS directories.
* Changed reporting of file times.  Files that are written from Unix
  now update the Mac's idea of the files modification time.  Unix
  mtime is now reported instead of ctime.
* Added support for a new UAM to afpd.  This requires that client
  Macs have MacTCP and AuthMan installed.  Should make running afpd
  for AFS easier.
* Removed code so that otherwise valid volumes for which the mounting
  user has no permission will appear in the volume selection dialog
  on the Mac gray-ed out.
* Added code from Chris Metcalf of MIT to the AppleDouble library
  which improves permission inheritance.
* Added code from G. Paul Ziemba of Alantec, Inc to better report
  errors in psf.  Also changed psf to use syslog for errors that
  users aren't interested in.
* Added information to psf's man page to better explain the
  interaction between psf, pap, and lpd.
* Make psf/pap/psa do accounting when it's turnes on in
  /etc/printcap.
* Changed pap's error message when there is no printer specified on
  the command line and no .paprc is found.  Also heavily modified
  pap's man page to reflect changes in the "new" version of pap,
  including moving it from section 8 to section 1.
* Fixed a byte-order bug in pap's sequence numbers.  Doubt if pap has
  ever worked right on little endian machines!
* Added a flag to pap to optionally close before receiving EOF from
  the printer.  Off by default.  psf calls pap with this option on.
* Added timeouts to the nbp library calls.  This means that processes
  won't hang when atalkd dies during boot, thus hanging your
  machine.

Changes from the 1.3 release
----------------------------

* Fixed a bug in afpd which would cause APPL mappings to contain both
  mac and unix path names.  The fixed code will handle the old
  (corrupted) database.
* Fixed a *very* serious bug which would cause files to be corrupted
  when copying to afpd.
* Fixed a bug in afpd which would cause replies to icon writes to
  contain the written icon.
* Filled in the function code switch in afpd.  Previously, a hacker
  could probably have used afpd to get unauthorized access to a
  machine running afpd.
* Fixed a bug in the asp portion of libatalk.a which could cause the
  malloc()/free() database to be corrupted.
* Fixed a bug in atalkd's zip query code.  With this bug, only the
  first N % 255 nets get queried.  However, since nets bigger than
  255 are usually pretty unstable, the unqueried for nets will
  eventually get done, when N drops by one.
* Suppressed a spurious error ("route: No such process") in atalkd.

Changes from the 1.2.1 release
------------------------------

* atalkd is completely rewritten for phase 2 support.  atalkd.conf
  from previous version will not work!
* afpd now has better AFS support.  In particular, the configuration
  for AFS was made much easier; a number of Kerberos-related
  byte-ordering and time problems were found; clear-text passwords
  were added (thanks to <geeb@umich.edu>).
* afpd now handles Unix permissions much better (thanks to
  <metcalf@mit.edu>).
* There are many, many more changes, but most are small bug fixes.

Changes from the 1.2 release
----------------------------

* The Sun support now uses loadable kernel modules (a la VDDRV)
  instead of binary patches. As such, it should work on any sunos
  greater than 4.1, and is confirmed to work under 4.1.1 and 4.1.2.
* The DEC support no longer requires source. It also runs under
  ultrix 4.1 and 4.2. It still requires patching your kernel, but the
  patches are limited to those files available to binary-only sites
  -- primarily hooks for things like netatalk.
* The etc.rc script now uses changes made to nbprgstr (see below).
* aecho now takes machine names on the command line.
* nbplkup now takes a command line argument specifying the number of
  responses to accept. It also takes its defaults from the NBPLKUP
  environment variable.
* nbprgstr may be used to register a name at any requested port.
* afpd now logs if an illegal shell is used during login, instead of
  silently denying service.
* A bug in afpd which caused position information for the directory
  children of the root of a volume to be ignored has been fixed.
* Several typos in afpd which would cause include files necessary to
  ultrix to be skipped have been fixed.
* atalkd will no long propagate routes to networks whose zone
  it doesn't know.
* atalkd no longer dumps core if it receives a ZIP GetMyZone request
  from a network whose zone it doesn't know. (Since this currently
  can only happen from off net, it's not precisely a legal request.)
* pap and papd (optionally) no longer check the connection id in PAP
  DATA responses. Both also maintain the function code in non-first-packet
  PAP DATA responses.  These changes are work-arounds to deal with
  certain AppleTalk printer cards, notably the BridgePort LocalTalk
  card for HP LJIIISIs.
* pap no longer sends an EOF response to each PAP SENDDATA request,
  only the first.
* A bug in papd which would cause it to return a random value when
  printing the procset to a piped printer has been fixed.
* A bug relating to NBP on reverse-endian machines has been fixed.
* atp_rsel() from libatalk now returns a correct value even if it
  hasn't recieved anything yet.
* atalk_addr() from libatalk no longer accepts addresses in octal
  format, since AppleTalk addresses can have leading zeros. Also it
  checks that the separator character is a '.'.
* Pseudo man pages for nbplkup, nbprgstr, and nbpunrgstr, have been
  added.
* The example in the psf(8) man page is now correct.
* The man pages for changed commands have been updated.
* The README files for various machine have been updated
  appropriately.
