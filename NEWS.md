Changes in 4.2.4
================

* FIX: uams: Check for const pam_message member of pam_conv, GitHub #2196
       Makes it possible to build on Solaris 11.4.81 CBE
* FIX: meson: Avoid build error in incomplete Homebrew env, GitHub #2190
* UPD: meson: Build with Homebrew libraries is now opt-in, GitHub #2194
       To opt in to build against Homebrew, use -Dwith-homebrew=true
* UPD: docs: Improve afpd and macipgw man pages, GitHub #2155

Changes in 4.2.3
================

* FIX: Properly read from afp.conf file passed with -F parameter, GitHub #2150
* FIX: Read the appletalk option only when built with DDP, GitHub #2149
* UPD: Consistently return exit code 0 after daemon version info, GitHub #2151
* UPD: libatalk: MySQL query error log level is dropped to debug, GitHub #2143
* UPD: initscripts: Improvements to netatalk OpenRC init script, GitHub #2148
* FIX: meson: enhance iconv detection when cross compiling, GitHub #1921
* UPD: docs: Cross-platform friendly docs for CNID statedir, GitHub #2146

Changes in 4.2.2
================

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
================

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
================

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
================

* UPD: meson: Look for shared Berkeley DB library in versioned subdir too,
       to detect the library in the MacPorts build system, GitHub #1909
* FIX: webmin: Redirect back to the originating module index tab
       when returning from actions, GitHub #1915
* FIX: webmin: Fix '-router' switch in Webmin atalkd module, GitHub #1943
* FIX: webmin: Fix a default value helptext string, GitHub #1946
* UPD: Add GPL v2 license grant to mysql CNID backend code, GitHub #1874

Changes in 4.1.1
================

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
================

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
       Meson's with-lockfile-path now points to the lockfile root
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
================

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
================

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
================

* FIX: Workaround for bug in AppleShare Client 3.7.4, GitHub #1749
       Only report support of AFP 2.2 and later to DSI (TCP) clients
       which shaves several bytes off the server response
       and lowers the chance of >512 byte FPGetSrvrInfo response.
* UPD: All AppleTalk daemons now take -v to print version info, GitHub #1745
* FIX: `ad find' can take any kind of string, not just lowercase, GitHub #1751
* UPD: meson: Default to no init scripts if service management command
       not found, GitHub #1743
* FIX: Include config.h by relative path consistently (cleanup) GitHub #1746
* FIX: Remove duplicate header includes in MySQL CNID backend, GitHub #1748
* FIX: docs: Fix formatting of afppasswd man page, GitHub #1750
* FIX: webmin: Properly install netatalk-lib.pl, GitHub #1752

Changes in 4.0.5
================

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
================

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
* NEW: docs: Create manual page for `afptest' (testsuite) tools, GitHub #1695
* UPD: docs: Bring CONTRIBUTORS up to date, GitHub #1722
* UPD: testsuite: Consolidate afp_ls as a command in afparg, GitHub #1705
       - Add `FPEnumerate dir' as an afparg command
       - Remove `afp_ls' as a separate executable
* UPD: testsuite: Merge encoding test into spectest, GitHub #1716
       - Add `Encoding' as a testset in the spectest
       - Rewrite the `western' test to use Unicode for the same characters
       - Remove `afp_encodingtest' as a separate executable
* UPD: testsuite: Collapse spectest into a single suite, GitHub #1713
       The testsuite grouping have been removed, and all spectests
       are in a single suite. The tier 2 tests are enabled with
       the -c option. The sleep and readonly tests can be run with
       the -f option.
* UPD: testsuite: Enable Color terminal output by default,
       and flip the -C option, GitHub #1708
* UPD: testsuite: Print a test summary for the spectest, GitHub #1708
* UPD: testsuite: Treat `Not Tested' as a failure again, GitHub #1709
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
================

* FIX: afpd: Limit FPGetSrvrInfo packet for AppleTalk clients, GitHub #1661
       This prevents errors with very old clients
       when many AFP options are enabled.
* FIX: Fix EOF error reporting in dsi_stream_read(), GitHub #1631
       This should prevent warnings such as:
       `dsi_stream_read: len:0, unexpected EOF'
* FIX: Fix regression when accessing the afpd UUID, GitHub #1679
       Resolves an error when running the `ad' utilities.
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
================

* NEW: Bring back Classic Mac OS `legacy icon' option, GitHub #1622
* UPD: Spotlight: Support TinySPARQL/LocalSearch, GitHub #1078
* FIX: ad: Fix volume check for the AppleDouble toolsuite, GitHub #1605
       Check was failing if the `ea = ad' option was set.
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
       Build with the new `-Dwith-testsuite' option.
* FIX: webmin: Make AppleTalk service control functional, GitHub #1636

Changes in 4.0.1
================

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
================

* NEW: Reintroduce AppleTalk / DDP support, GitHub #220
       Controlled with the new build system option `-Dwith-appletalk'.
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
       Controlled with the new build system option `-Dwith-webmin'.
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
* NEW: meson: Introduce `-Dwith-readmes' option for installing additional docs.
       GitHub #1310
* REM: Remove the Autotools build system. Meson is now the only choice.
       GitHub #1213

Changes in 3.2.10
=================

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
================

* UPD: Use the recommended command to import Solaris init manifest,
       GitHub #1451
* FIX: uams: Make sure the DHX2 client nonce is aligned appropriately,
       GitHub #1456
* FIX: uams: Fix DHCAST128 key alignment problem, GitHub #1464
* FIX: wolfssl: OpenSSL coexistence tweaks, GitHub #1469
* FIX: docs: Remove straggler path substitution in afp.conf, GitHub #1480

Changes in 3.2.8
================

* UPD: Bump bundled WolfSSL library to stable version 5.7.2, GitHub #1433
       Resolves CVE-2024-1544, CVE-2024-5288, CVE-2024-5991, CVE-2024-5814
* UPD: Revert local modifications to the bundled WolfSSL library, GitHub #1432
* FIX: Enable building against a shared WolfSSL 5.7.2 library, GitHub #1421
* FIX: meson: Do not define rpath with a linker argument, GitHub #1443

Changes in 3.2.7
================

* NEW: meson: Ability to control the run-time linker path config file,
       GitHub #1396
       New boolean Meson option: `-Dwith-ldsoconf'
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
================

* BREAKING: meson: Refresh the dynamic linker cache when installing on Linux,
       GitHub #1386
       This fixes the issue of the libatalk.so shared library not being found
       when configuring with a non-standard library path, e.g. /usr/local/lib .
       New Meson option `-Dwith-install-hooks' controls this behavior,
       allowing you to disable the install hook in non-privileged environments.
       On Linux systems with glibc, we now install the following config file:
       /etc/ld.so.conf.d/libatalk.conf
* BREAKING: meson: Introduce option to control which manual l10n to build,
       GitHub #1390
       New Meson option `-Dwith-manual-l10n' default to empty, can be set to
       `ja' to build the Japanese localization of the html manual.
       This changes the default behavior of the build system
       to not build the Japanese html manual by default.
* BREAKING: meson: Install htmldocs into htmldocs subdir, GitHub #1391
       Previously, the html manual files were installed into the root
       of the netatalk doc directory. Now they are put under netatalk/htmldocs .
* BREAKING: meson: Use modern linker flag for rpath, remove dtags override,
       GitHub #1384
       When configuring with `-Dwith-rpath=true' the linker flags
       `-Wl,-rpath,' will be prepended instead of the old `-R' flag.
       On Linux platforms, we no longer prepend `-Wl,--enable-new-dtags',
       either.

Changes in 3.2.5
================

* BREAKING: meson: Allow choosing shared or static libraries to build,
       GitHub #1321
       In practice, only shared libraries are built by default now.
       Use the `default_library' option to control what is built.
* FIX: meson: Control the MySQL CNID backend, and support MariaDB, GitHub #1341
       Introduces a new boolean `with-cnid-mysql-backend' option.
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
================

* UPD: autotools: Restore ABI versioning of libatalk,
       and set it to 18.0.0, GitHub #1261
* UPD: meson: Define long-form soversion as 18.0.0, GitHub #1256
       Previously, only `18' was defined.
* NEW: meson: Introduce pkgconfdir override option, GitHub #1241
       The new option is called `with-pkgconfdir-path'
       and is analogous to the `with-pkgconfdir' Autotools option.
       Additionally, the hard-coded "netatalk" path suffix has been removed.
* NEW: meson: Introduce `debian' init style option
       that installs both sysv and systemd, GitHub #1239
* FIX: meson: Add have_atfuncs check,
       and make dtags dependent on rpath flag, GitHub #1236
* FIX: meson: Correct overwrite install logic for config files, GitHub #1253
* FIX: Fix typo in netatalk_conf.c log message

Changes in 3.2.3
================

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
       the `with-ssl-override' option, GitHub #1227

Changes in 3.2.2
================

* UPD: meson: Use external SSL dependency to provide cast header, GitHub #1186
       This reintroduces OpenSSL/LibreSSL as a dependency for the DHX UAM,
       while removing all source files with the SSLeay copyright notice.
* UPD: meson: Add option to override system WolfSSL
       with embedded WolfSSL: `with-ssl-override', GitHub #1176
* REM: Remove obsolete Red Hat Upstart and SuSE SysV init scripts, GitHub #1163
* FIX: meson: Fix errors in PAM support macro, GitHub #1178
* FIX: meson: Fix perl shebang substitution in cnid2_create script, GitHub #1183
* FIX: meson: Fix operation of D-Bus path macros, GitHub #1182
* FIX: meson: Fix errors in shadow password macro, GitHub #1192
* FIX: autotools: gcc 8.5 expects explicit library flags
       for libgcrypt, GitHub #1188
* NEW: Create a security policy, GitHub #1166

Changes in 3.2.1
================

* FIX: CVE-2024-38439,CVE-2024-38440,CVE-2024-38441: Harden user login,
       GitHub #1158
* BREAKING: meson: Rework option semantics and feature macros, GitHub #1099
       - Consistent syntax of the build options to make them user-friendly
       - Standardises the syntax of the feature macros
       - Fixes the logic of the largefile support macro
       - Disables gssapi support if the Kerberos V UAM is not required
       - All options are now defined either as `with-*' or `with-*-path'
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
================

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
=================

* FIX: CVE-2022-22995: Harden create_appledesktop_folder(), GitHub #480
* FIX: Disable dtrace support on aarch64 FreeBSD hosts, Github #498
* FIX: Correct syntax for libwrap check in tcp-wrappers.m4, GitHub #500
* FIX: Correct syntax for libiconv check in iconv.m4, GitHub #491
* FIX: quota is not supported on macOS, GitHub #492

Changes in 3.1.17
=================

* FIX: CVE-2023-42464: Validate data type in dalloc_value_for_key(), GitHub #486
* FIX: Declare a variable before using it in a loop,
       which was throwing off the default compiler on RHEL7, GitHub #481
* UPD: Distribute tarballs with xz compression by default, not gzip, GitHub #478
* UPD: Add AUTHOR sections to all man pages with a reference to CONTRIBUTORS,
       and standardize headers and footers, GitHub #462

Changes in 3.1.16
=================

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
* REM: build system: Remove ABI checks and the --enable-developer option, GitHub#262
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
=================

* FIX: CVE-2022-43634
* FIX: CVE-2022-45188
* NEW: Support for macOS hosts, Intel and Apple silicon, GitHub#281
* FIX: configure.ac: update deprecated autoconf syntax
* UPD: configure.ac: Support linking with system shared libraries
       Introduces the --with-talloc option
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
=================

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
=================

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
=================

* FIX: dhx uams: build with LibreSSL, GitHub#91
* FIX: various spelling errors
* FIX: CVE-2018-1160

Changes in 3.1.11
=================

* NEW: Global option "zeroconf name", FR#99
* NEW: show Zeroconf support by "netatalk -V", FR#100
* UPD: gentoo: Switch openrc init script to openrc-run, GitHub#77
* FIX: log message: name of function doese not match, GitHub#78
* UPD: volume capacity reporting to match Samba behavior, GitHub#83
* FIX: debian: sysv init status command exits with proper exit code, GitHub#84
* FIX: dsi_stream_read: len:0, unexpected EOF, GitHub#82
* UPD: dhx uams: OpenSSL 1.1 support, GitHub#87

Changes in 3.1.10
=================

* FIX: cannot build when ldap is not defined, bug #630
* FIX: SIGHUP can cause core dump when mdns is enabled, bug #72
* FIX: Solaris: stale pid file puts netatalk into maintenance mode, bug #73
* FIX: dsi_stream_read: len:0, unexpected EOF, bug #633

Changes in 3.1.9
================

* FIX: afpd: fix "admin group" option
* NEW: afpd: new options "force user" and "force group"
* FIX: listening on IPv6 wildcard address may fail if IPv6 is
       disabled, bug #606
* NEW: LibreSSL support, FR #98
* FIX: cannot build when acl is not defined, bug #574
* UPD: configure option "--with-init-style=" for Gentoo.
       "gentoo" is renamed to "gentoo-openrc".
       "gentoo-openrc" is same as "openrc".
       "gentoo-systemd" is same as "systemd".
* NEW: configure option "--with-dbus-daemon=PATH" for Spotlight feature
* UPD: use "tracker daemon" command instead of "tracker-control" command
       if Gnome Tracker is the recent version.
* NEW: configure options "--enable-rpath" and "--disable-rpath" which
       can be used to force setting of RPATH (default on Solaris/NetBSD)
       or disable it.
* NEW: configure option "--with-tracker-install-prefix" allows setting
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
================

* FIX: CNID/MySQL: Quote UUID table names.
       https://sourceforge.net/p/netatalk/bugs/585/
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
================

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
================

* FIX: Spotlight: fix for long running queries
* UPD: afpd: distribute SIGHUP from parent afpd to children and force
       reload shares
* FIX: netatalk: refresh Zeroconf registration when receiving SIGHUP
* NEW: configure option "--with-init-style=debian-systemd" for Debian 8 jessie
       and later.
       "--with-init-style=debian" is renamed "--with-init-style=debian-sysv".

Changes in 3.1.5
================

* FIX: Spotlight: several important fixes

Changes in 3.1.4
================

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
================

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
================

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
================

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
* FIX: dbd: Don't print message "Ignoring ._file" for every ._ file.
       Bug #552.
* FIX: afpd: Don't flood log with failed sys_set_ea() messages.

Changes in 3.1.0
================

* NEW: AFP Spotlight support with Gnome Tracker
* NEW: New option "spotlight" (G/V)
* NEW: Configure option --with-tracker-pkgconfig-version
* NEW: Configure option --with-tracker-prefix
* NEW: If Spotlight is enabled, launch our own dbus instance
* NEW: New option "dbus daemon" (G)
* UPD: Add configure option --with-afpstats for overriding the
       result of autodetecting dbus-glib presence
* NEW: Add recvfile support with splice() on Linux. New global options
       "recvfile" (default: no) and "splice size" (default 64k).
* NEW: CNID backend "mysql" for use with a MySQL server

Changes in 3.0.7
================

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
================

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
* NEW: Configure option --with-tbd which can be used to disable the
       use of the bundled tdb and use a system installed version.

Changes in 3.0.5
================

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
================

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
================

* UPD: afpd: Increase default DSI server quantum to 1 MB
* UPD: bundled libevent2 is now static
* NEW: --with-lockfile=PATH configure option for specifying an
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
       $sysconfdir/pam.d/. Add configure option --with-pam-confdir
       to specify alternative path.
* NEW: AFP stats about active session via dbus IPC. Client side python
       program `afpstats`. Requires dbus, dbus-glib any python-dbus.
       configure option --dbus-sysconf-dir for specifying dbus
       system security configuration files.
       New option 'afpstats' (default: no) which determines whether
       to enable the feature or not.
* NEW: configure option --with-init-dir
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
================

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
================

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
       configure arg --disable-libevent, added configure args
       --with-libevent-header|lib.
* UPD: gentoo initscript: merge from portage netatalk.init,v 1.1
* REM: Remove --with-smbsharemodes configure option, it was an
       empty stub not yet implemented

Changes in 3.0
==============

* UPD: afpd: force read only mode if cnid scheme is last
* REM: afpd: removed global option "icon"
* FIX: CNID path for user homes

Changes in 3.0 beta2
====================

* UPD: Solaris and friends: Replace initscript with SMF manifest
* FIX: Solaris and friends: resource fork handling

Changes in 3.0 beta1
====================

* UPD: afpd: Performance tuning of read/write AFP operations. New option
       "afp read locks" (default: no) which disables that the server
       applies UNIX byte range locks to regions of files in AFP read and
       write calls.
* UPD: apple_dump: Extended Attributes AppleDouble support.
       (*BSD is not supported yet)

Changes in 3.0 alpha3
=====================

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
=====================

* UPD: afpd: Store '.' as is and '/' as ':' on the server, don't
       CAP hexencode as "2e" and "2f" respectively
* UPD: afdp: Automatic name conversion, renaming files and directories
       containing CAP sequences to their not enscaped forms
* UPD: afpd: Correct handling of user homes and users without homes
* UPD: afpd: Perform complete automatic adouble:v2 to adouble:ea conversion
       as root. Previously only unlinking the adouble:v2 file was done as root
* UPD: dbd: -C option removes CAP encoding
* UPD: Add graceful option to RedHat init script
* UPD: Add --disable-bundled-libevent configure options When set to yes,
       we rely on a properly installed version on libevent CPPFLAGS and LDFLAGS
       should be set properly to pick that up
* UPD: Run ldconfig on Linux at the end of make install
* FIX: afpd: ad cp on appledouble = ea volumes
* FIX: dbd: ignore ._ appledouble files
* REM: Volumes options "use dots" and "hex encoding"

Changes in 3.0 alpha1
=====================

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
================

* FIX: Missing UAM links
* FIX: Lockup in AFP logout on Fedora 17
* FIX: Reset signal handlers and alarm timer after successfull PAM
       authentication. Fixes a problem with AFP disconnects caused
       by pam_smbpass.so messing with our handlers and timer.
* FIX: afpd: Fix a possible problem with sendfile on Solaris derived
       platforms

Changes in 2.2.3
================

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
================

* NEW: afpd: New option "adminauthuser". Specifying e.g. "-adminauthuser root"
       whenever a normal user login fails, afpd tries to authenticate as
       the specified adminauthuser. If this succeeds, a normal session is
       created for the original connecting user. Said differently: if you
       know the password of adminauthuser, you can authenticate as any other
       user.
* NEW: configure option "--enable-suse-systemd" for openSUSE12.1 and later.
       "--enable-redhat-systemd" and "--enable-suse-systemd" are same as
       "--enable-systemd".
       "--enable-suse" is renamed "--enable-suse-sysv".
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
* FIX: dbd: Remove BerkeleyDB database environment after running `dbd`. This
       is crucial for the automatic BerkeleyDB database upgrade feature which
       is built into cnid_dbd and dbd.
* FIX: Fix compilation error when AppleTalk support is disabled
* FIX: Portability fixes
* FIX: search of surrogate pair

Changes in 2.2.1
================

* NEW: afpd: disable continous service feature by default, new option
       -keepsessions to enable it
* NEW: configure option "--enable-redhat-systemd" for Fedora15 and later.
       "--enable-redhat" is renamed "--enable-redhat-sysv".
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
* FIX: afpd: fix compilation error if --enable-ddp is used
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
================

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
===================

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
===================

* FIX: afpd: fix option volsizelimit to return a usefull value for the
       volume free space using `du -sh` with popen
* FIX: afpd: fix idle connection disconnects
* FIX: afpd: don't disconnect sessions for clients if boottimes don't match
* FIX: afpd: better handling of very long filenames that contain many
       multibyte UTF-8 glyphs

Changes in 2.2beta2
===================

* NEW: afpd: AFP 3.3
* UPD: afpd: AFP 3.x can't be disabled

Changes in 2.2beta1
===================

* FIX: composition of Surrogate Pair
* UPD: gentoo,suse,cobalt,tru64: inistscript name is "netatalk", not "atalk"
* UPD: gentoo: rc-update install don't hook in the Makefile

Changes in 2.2alpha5
====================

* UPD: afpd: new option "searchdb" which enables fast catalog searches
       using the CNID db.
* UPD: Case-insensitive fast search with the CNID db
* UPD: cnid_dbd: afpd now passes the volume path, not the db path when
       connecting for a volume. cnid_dbd will read the
       ".AppleDesktop/.volinfo" file of the volume in order to figure
       out the CNID db path and the volume charset encoding.

Changes in 2.2alpha4
====================

* NEW: Enhanced CNID "dbd" database for fast name search support.
       Important: this makes cnidscheme "cdb" incompatible with "dbd".
* NEW: afpd: support for fast catalog searches
* NEW: ad utility: ad find
* UPD: afpd: CNID database versioning check for "cdb" scheme
* UPD: cnid_dbd: CNID database versioning and upgrading. Additional
       CNID database index for fast name searches.

Changes in 2.2alpha3
====================

* FIX: afpd: various fixes
* FIX: Any daemon did not run if atalkd doesn't exist (redhat/debian)

Changes in 2.2alpha2
====================

* FIX: afpd: fix compilation error when ACL support is not available
* FIX: Ensure Appletalk manpages and config files are distributed

Changes in 2.2alpha1
====================

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
       use configure switch --enable-ddp.
* FIX: afpd: Solaris 10 compatibilty fix: don't use SO_SNDTIMEO/SO_RCVTIMEO,
       use non-blocking IO and select instead.
* FIX: cnid_dbd: Solaris 10 compatibilty fix: don't use SO_SNDTIMEO/SO_RCVTIMEO,
       use non-blocking IO and select instead.
* REM: afile/achfile/apple_cp/apple_mv/apple_rm: use ad

Changes in 2.1.6
================

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
================

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
================

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
================

* FIX: afpd: fix a serious error in networking IO code
* FIX: afpd: Solaris 10 compatibilty fix: don't use SO_SNDTIMEO, use
       non-blocking IO and select instead for writing/sending data.
* UPD: Support for BerkeleyDB 5.0.

Changes in 2.1.2
================

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
================

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
======================

* NEW: afpd: new volume option "volsizelimit" for limitting reported volume
       size. Useful for limitting TM backup size.
* UPD: dbd: -c option for rebuilding volumes which prevents the creation
       of .AppleDouble stuff, only removes orphaned files.

Changes in 2.1-beta2
====================

* NEW: afpd: static generated AFP signature stored in afp_signature.conf,
       cf man 5 afp_signature.conf
* NEW: afpd: clustering support: new per volume option "cnidserver".
* UPD: afpd: set volume defaults options "upriv" and "usedots" in the
       volume config file AppleVolumes.default. This will only affect
       new installations, but not upgrades.
* FIX: afpd: prevent security attack guessing valid server accounts. afpd
       now returns error -5023 for unknown users, as does AppleFileServer.

Changes in 2.1-beta1
====================

* NEW: afpd: AFP 3.2 support
* NEW: afpd: Extended Attributes support using native attributes or
       using files inside .AppleDouble directories.
* NEW: afpd: ACL support with ZFS
* NEW: cnid_metad: options -l and -f to configure logging
* NEW: IPv6 support
* NEW: AppleDouble compatible UNIX files utility suite `ad ...`.
       With 2.1 only `ad ls`.
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
       i.e. they're installed to --sysconfdir.
* FIX: UTF-8 volume name length
* FIX: atalkd: workaround for broken Linux 2.6 AT kernel module:
       Linux 2.6 sends broadcast queries to the first available socket
       which is in our case the last configured one. atalkd now tries to
       find the right one.
       Note: now a misconfigured or plugged router can broadcast a wrong route !
* REM: afpd: removed CNID backends "db3", "hash" and "mtab"
* REM: cnid_maint: use dbd
* REM: cleanappledouble.pl: use dbd
* REM: nu: use `macusers` instead

Changes in 2.0.5
================

* NEW: afpd: Time Machine support with new volume option "tm".
* FIX: papd: Remove variable expansion for BSD printers. Fixes CVE-2008-5718.
* FIX: afpd: .AppleDxxx folders were user accessible if option 'usedots'
       was set
* FIX: afpd: vetoed files/dirs where still accessible
* FIX: afpd: cnid_resolve: don't return '..' as a valid name.
* FIX: uniconv: -d option wasn't working

Changes in 2.0.4
================

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
================

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
================

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
================

* NEW: --enable-debian configure option. Will install /etc/init.d/atalk
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
================

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
       username _could_ have been selected.

Changes in 2.0-rc2
==================

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
==================

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
====================

* NEW: atalkd, papd and npb tools now support nbpnames with extended
       characters
* NEW: integrated CUPS support for papd
* NEW: optionally advertise SSH tunneling capabilties
* NEW: automatic logfile removal for cnid_metad
* NEW: asip-status.pl has been added to netatalk
* UPD: updated documentation
* UPD: we now require Berkeley DB >= 4.1
* UPD: 64bit Linux fixes from Stew Benedict, Mandrakesoft
* UPD: remove --enable-sendfile
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
====================

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
=====================

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
=====================

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
================

* NEW: afpd: Using the mswindows option now implicitly specifies usedots.
  [Sam Noble]
* UPD: afpd.8: Updated the option documentation.
  [Thomas Kaiser, Sebastian Rittau]
* FIX: configure: Removed broken --with-flock-locks option. [Bjoern Fernhomberg]
* FIX: libatalk: Do not log network probe (OSX 10.3). [Didier Gautheron]
* FIX: libatalk: Deadlock in signal handlers. [Didier]
* FIX: libatalk: Compilation with Linux kernel 2.6 fixed. [Sebastian, Bjoern]
* FIX: afpd: Solaris compile issues. [Bjoern]
* FIX: afpd: If connection broke in dsi_tickle the child did never die. [Bjoern]
* FIX: afpd: Catsearch, fixes a possible segmentation fault. [Bjoern]
* FIX: afpd: Compilation issues. [Olaf Hering, Sebastian]
* FIX: cnid: Fix compile problems on Tru64. [Burkhard Schmidt]
* FIX: megatron: Fixed an uninitialized variable. [Olaf]
* FIX: installation: Don't overwrite PAM file if --enable-overwrite configure
       option is not set. [Sam, Ryan Cleary]
* FIX: installation: Fixed BSD installation. [Toru TAKAMIZU]
* FIX: docs: Removed ssl-dir/ssl-dirs confusion from doc/INSTALL. [Bjoern]

Changes in 1.6.3
================

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
================

* FIX: atalkd: Fixes for reading long configuration file lines. [Dan Wilga]
* FIX: afpd: CNID id allocation in a multiuser environment.
* FIX: papd: Fix PSSP support when PAM is enabled.
* FIX: macusers: Can now cope with IP addresses as well as host names.
* FIX: etc2ps.sh: Install correctly.

Changes in 1.6.1
================

* UPD: Improve --enable-fhs. [Olaf Hering]
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
================

* FIX: pap looks at the STDOUT file descriptor to see if it's connected to
       a tty, instead of STDIN.

Changes in 1.6pre2
==================

* UPD: Removed --with-catsearch option from configure and enable FPCatSearch
       by default.
* UPD: The dbpath argument does now support variable substitution.
* FIX: Build fix for non-GNU-C compilers in libatalk/util/logger.c.
* FIX: Two directories with the same name but different case confused the
       Mac Finder.
* FIX: The ROOT_PARENT directory could get invalidated.

Changes in 1.6pre1
====================

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
================

* NEW: Allow afpd debugging to be enabled by sending the afpd process
       a SIGUSR1.
* NEW: Allow SLP to be disabled per afpd process by adding a new -noslp flag
       to afpd.conf.
* FIX: Use server name (if not NULL) for the SLP URL.
* FIX: papd: buffer overflow during config file read (Olaf Hering)

Changes in 1.5.4
================

* FIX: File open read only and then read write (aka pb with Illustrator).
* FIX: Problems with unexpected EOF when reading files on a ddp connection
       Photoshop, old finder.
* FIX: --with-nls-dir option does now work

Changes in 1.5.3
================

* UPD: Extend the --disable-shell-check functionality to ignore users
       with _no_ shell.  Also, send a log message if a user is refused login
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
================

* NEW: NetBSD support contributed by David Rankin and NetBSD contributors.
       This includes a new configure option --enable-netbsd.
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
       code requires you to delete _all_ existing .AppleDB directories in
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
================

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
================

* FIX: Compilation on Tru64 systems was broken, since libtool's acinclude.m4
       file on the packagers system did not contain the necessary patch.
* FIX: On some systems, atalkd refuses to start, since it couldn't detect
       any interfaces. This was caused by an overzealous validity check.

Changes in 1.5rc2
=================

* FIX: contrib/shell_utils/lp2pap.sh was erased when "make clean" was called.
       Now we distribute lp2pap.sh.tmpl instead, and lp2pap.sh is automatically
       generated during package build.
* FIX: Some platforms (notably Tru64) don't have the snprintf() call, which
       was used in etc/afp/afp_config.c. This call was replaced by sprintf()
       and prior bounds checking.

Changes from the 1.4b1 release:
===============================

*   Fixed the maximum free/total volume size in afpd.

*   Made ~ the default volume in afpd.

*   Fixed pid file handling and changed setpgrp() to setpgid() in afpd,
    papd, and atalkd.

*   Added code to afpd to set the Unix file time stamps with utime().

*   Fixed a bug in papd's printcap code which limited it to 15 or so
    printers.

*   Fixed papd's handling of piped printers.

*   Fixed papd's handling of bad job names.

*   Fixed atalkd to send NBP LKUP packets from NBP port.

*   Added "sync;sync;sync" to Solaris kinstall to help with streams
    file corruption.

*   Added nlocalrts to streams ddp.conf.  Thanks Thomas Tornblom.

*   Fixed signed extension infinite loop in Solaris module.

*   Moved all the config files to .../config.

Changes from the 1.3.3 release:
===============================

*   Added code from Sun Microsystems, Inc (OPCOM) for Solaris support.
    See COPYRIGHT.

*   Added support for FreeBSD, mostly changes by Mark Dawson and Julian
    Elischer.

*   All sorts of other stuff.

Changes from the 1.3.1 release:
===============================

*   Added options to psf's filter names to support accounting on HPs.
    !!! NOTE:  The location of the filters has changed, see the man
    page for where.

*   Added code from Alan Cox to support Linux.

*   Rewrote papd.  Now handles dropped connections better.
    Configuration has been modernized.  !!! NOTE: The format of the
    configuration file has changed, but NOT THE NAME.

*   Added Kerberos support to papd.

*   atalkd now removes routes on a SIGTERM.  Still can't just restart
    it, but it's closer.

*   Changed atalkd and the kernel driver to remove a hack added to
    support sending to 0.255.  Now the kernel will allow multiple open
    sockets with the same port, so long as the addess associated with
    the port is different.  atalkd now opens a socket for each port on
    each interface.

*   atalkd now rewrites its configuration file.  If no configuration
    file is given, one will be generated.  Permissions on the new
    configuration file will be inherited from the old one.  If there is
    no old one, permissions default to 644.  Won't rewrite the file if
    the owner doesn't have write permission.

*   Removed support for the "AFS Kerberos UAM", in favor of the
    "AuthMan UAM".  Kerberos support should now be much more
    straight-forward.

*   Fixed a bug in afpd which would cause incorrect group calculations
    on ultrix machines.

*   Fixed a bug in afpd which causes SimpleText and some other
    applications to silently fail to write.  There's also a bug in
    MacOS, but we can't fix that.

*   Fixed a bad interaction with afpd and AFS which would cause file
    writes to not propogate between AFS clients.

*   !!! CHANGED the name(s) of afpd's config files.  The new files are
    AppleVolumes.system and AppleVolumes.default.  If AppleVolumes.system
    exists, it is always read, AppleVolumes.default is only read if the
    user has no AppleVolumes file.  Included a flag "-u" to indicate
    which file has precedence.  "-u" user wins, otherwise ".system"
    wins.

*   Rewrote the AppleVolumes parsing code.  Now works.

*   Added a filename extension mapping to afpd.  User always takes
    precedence, regardless of the "-u" flag.  Code to change the type
    of all Unix files contributed by Kee Hinckley <nazgul@utopia.com>.

*   afpd now supports both UFS and AFS volumes simultaneously.  It also
    uses access() to attempt to calculate reasonable Mac permissions
    for AFS directories.

*   Changed reporting of file times.  Files that are written from Unix
    now update the Mac's idea of the files modification time.  Unix
    mtime is now reported instead of ctime.

*   Added support for a new UAM to afpd.  This requires that client
    Macs have MacTCP and AuthMan installed.  Should make running afpd
    for AFS easier.

*   Removed code so that otherwise valid volumes for which the mounting
    user has no permission will appear in the volume selection dialog
    on the Mac gray-ed out.

*   Added code from Chris Metcalf of MIT to the AppleDouble library
    which improves permission inheritance.

*   Added code from G. Paul Ziemba of Alantec, Inc to better report
    errors in psf.  Also changed psf to use syslog for errors that
    users aren't interested in.

*   Added information to psf's man page to better explain the
    interaction between psf, pap, and lpd.

*   Make psf/pap/psa do accounting when it's turnes on in
    /etc/printcap.

*   Changed pap's error message when there is no printer specified on
    the command line and no .paprc is found.  Also heavily modified
    pap's man page to reflect changes in the "new" version of pap,
    including moving it from section 8 to section 1.

*   Fixed a byte-order bug in pap's sequence numbers.  Doubt if pap has
    ever worked right on little endian machines!

*   Added a flag to pap to optionally close before receiving EOF from
    the printer.  Off by default.  psf calls pap with this option on.

*   Added timeouts to the nbp library calls.  This means that processes
    won't hang when atalkd dies during boot, thus hanging your
    machine.

Changes from the 1.3 release:
=============================

*   Fixed a bug in afpd which would cause APPL mappings to contain both
    mac and unix path names.  The fixed code will handle the old
    (corrupted) database.

*   Fixed a *very* serious bug which would cause files to be corrupted
    when copying to afpd.

*   Fixed a bug in afpd which would cause replies to icon writes to
    contain the written icon.

*   Filled in the function code switch in afpd.  Previously, a hacker
    could probably have used afpd to get unauthorized access to a
    machine running afpd.

*   Fixed a bug in the asp portion of libatalk.a which could cause the
    malloc()/free() database to be corrupted.

*   Fixed a bug in atalkd's zip query code.  With this bug, only the
    first N % 255 nets get queried.  However, since nets bigger than
    255 are usually pretty unstable, the unqueried for nets will
    eventually get done, when N drops by one.

*   Suppressed a spurious error ("route: No such process") in atalkd.

Changes from the 1.2.1 release:
===============================

*   atalkd is completely rewritten for phase 2 support.  atalkd.conf
    from previous version will not work!

*   afpd now has better AFS support.  In particular, the configuration
    for AFS was made much easier; a number of Kerberos-related
    byte-ordering and time problems were found; clear-text passwords
    were added (thanks to geeb@umich.edu).

*   afpd now handles Unix permissions much better (thanks to
    metcalf@mit.edu).

*   There are many, many more changes, but most are small bug fixes.

Changes from the 1.2 release:
=============================

*   The Sun support now uses loadable kernel modules (a la VDDRV)
    instead of binary patches. As such, it should work on any sunos
    greater than 4.1, and is confirmed to work under 4.1.1 and 4.1.2.

*   The DEC support no longer requires source. It also runs under
    ultrix 4.1 and 4.2. It still requires patching your kernel, but the
    patches are limited to those files available to binary-only sites
    -- primarily hooks for things like netatalk.

*   The etc.rc script now uses changes made to nbprgstr (see below).

*   aecho now takes machine names on the command line.

*   nbplkup now takes a command line argument specifying the number of
    responses to accept. It also takes its defaults from the NBPLKUP
    environment variable.

*   nbprgstr may be used to register a name at any requested port.

*   afpd now logs if an illegal shell is used during login, instead of
    silently denying service.

*   A bug in afpd which caused position information for the directory
    children of the root of a volume to be ignored has been fixed.

*   Several typos in afpd which would cause include files necessary to
    ultrix to be skipped have been fixed.

*   atalkd will no long propagate routes to networks whose zone
    it doesn't know.

*   atalkd no longer dumps core if it receives a ZIP GetMyZone request
    from a network whose zone it doesn't know. (Since this currently
    can only happen from off net, it's not precisely a legal request.)

*   pap and papd (optionally) no longer check the connection id in PAP
    DATA responses. Both also maintain the function code in non-first-packet
    PAP DATA responses.  These changes are work-arounds to deal with
    certain AppleTalk printer cards, notably the BridgePort LocalTalk
    card for HP LJIIISIs.

*   pap no longer sends an EOF response to each PAP SENDDATA request,
    only the first.

*   A bug in papd which would cause it to return a random value when
    printing the procset to a piped printer has been fixed.

*   A bug relating to NBP on reverse-endian machines has been fixed.

*   atp_rsel() from libatalk now returns a correct value even if it
    hasn't recieved anything yet.

*   atalk_addr() from libatalk no longer accepts addresses in octal
    format, since AppleTalk addresses can have leading zeros. Also it
    checks that the separator character is a '.'.

*   Pseudo man pages for nbplkup, nbprgstr, and nbpunrgstr, have been
    added.

*   The example in the psf(8) man page is now correct.

*   The man pages for changed commands have been updated.

*   The README files for various machine have been updated
    appropriately.
