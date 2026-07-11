afptest_external_deps = []

if threads.found()
    afptest_external_deps += threads
endif

enable_testsuite_uams = get_option('with-testsuite-uams')

if enable_testsuite_uams
    testsuite_libafpclient = dependency('libafpclient', required: false)
endif

libafptest_sources = [
    'adoublehelper.c',
    'afpclient.c',
    'afpcmd.c',
    'afphelper.c',
    'testhelper.c',
]

# Spotlight-only testsuite additions. The talloc dependency and the
# spotlight_marshalling.c TU are reused only when have_spotlight is true;
# afpcmd_spotlight.c uses the testsuite-local dsi.h exclusively (via
# afpclient.h) and must NOT be compiled against <atalk/dsi.h>.
libafptest_extra = []
libafptest_deps = []
libafptest_c_args = []
spectest_extra_c_args = []

if enable_testsuite_uams
    if testsuite_libafpclient.found()
        libafptest_extra += ['afpclient_libafpclient.c']
        libafptest_deps += testsuite_libafpclient
        afptest_external_deps += testsuite_libafpclient
        libafptest_c_args += ['-DHAVE_TESTSUITE_LIBAFPCLIENT=1']
        spectest_extra_c_args += ['-DHAVE_TESTSUITE_LIBAFPCLIENT=1']
    endif
endif

if have_spotlight
    libafptest_extra += [
        'afpcmd_spotlight.c',
        '../../etc/afpd/spotlight_marshalling.c',
    ]
    # spotlight_marshalling.c transitively pulls in <atalk/util.h>, which
    # in turn includes <bstrlib.h>; the bstring header search path comes
    # from the top-level `bstring` dependency. afpcmd_spotlight.c
    # transitively pulls in <atalk/spotlight.h> -> <atalk/globals.h> ->
    # <iniparser.h>, so the iniparser include path is required too.
    libafptest_deps += [talloc, bstring, iniparser]
    # HAVE_SPOTLIGHT_TEST gates the FPSpotlightRPC EXT_FN/FN_N
    # registrations in spectest.c. Local to the testsuite, not in
    # meson_config.h, so it cannot leak into other build units.
    spectest_extra_c_args += ['-DHAVE_SPOTLIGHT_TEST=1']
endif

libafptest = static_library(
    'afptest',
    libafptest_sources + libafptest_extra,
    include_directories: root_includes,
    dependencies: libafptest_deps,
    c_args: libafptest_c_args,
    link_with: libatalk,
)

afparg_sources = [
    'afparg.c',
    'afparg_FPfuncs.c',
]

executable(
    'afparg',
    afparg_sources,
    include_directories: root_includes,
    dependencies: afptest_external_deps,
    link_args: ['-rdynamic'],
    link_with: libafptest,
    install: true,
    install_rpath: rpath_libdir,
    build_rpath: rpath_libdir,
)

lantest_sources = [
    'lantest.c',
    'lantest_dircache_stats.c',
    'lantest_io_monitor.c',
]

executable(
    'afp_lantest',
    lantest_sources,
    include_directories: root_includes,
    dependencies: [afptest_external_deps, iniparser],
    c_args: confdir,
    link_with: libafptest,
    install: true,
    install_rpath: rpath_libdir,
    build_rpath: rpath_libdir,
)

login_test_sources = [
    'logintest.c',
    'logintest_dsi.c',
]

login_test_deps = []
login_test_c_args = []

if enable_testsuite_uams
    if testsuite_libafpclient.found()
        login_test_sources += [
            'logintest_uam.c',
            'testsuite_libafpclient.c',
            'logintest_libafpclient.c',
        ]
        login_test_deps += testsuite_libafpclient
        if threads.found()
            login_test_deps += threads
        endif
        login_test_c_args += ['-DHAVE_TESTSUITE_LIBAFPCLIENT=1']
    else
        warning(
            'libafpclient-backed UAM tests requested but libafpclient was not found',
        )
    endif
endif

executable(
    'afp_logintest',
    login_test_sources,
    include_directories: root_includes,
    dependencies: [afptest_external_deps, login_test_deps],
    c_args: login_test_c_args,
    link_args: ['-rdynamic'],
    link_with: libafptest,
    install: true,
    install_rpath: rpath_libdir,
    build_rpath: rpath_libdir,
)

speedtest_sources = [
    'speedtest.c',
    'speedtest_local_vfs.c',
    'afp_tcp_analytics.c',
]

executable(
    'afp_speedtest',
    speedtest_sources,
    include_directories: root_includes,
    dependencies: afptest_external_deps,
    link_args: ['-rdynamic', '-lm'],
    link_with: libafptest,
    install: true,
    install_rpath: rpath_libdir,
    build_rpath: rpath_libdir,
)

spectest_sources = [
    'T3_Error.c',
    'FPAddAPPL.c',
    'FPAddComment.c',
    'FPAddIcon.c',
    'FPByteRangeLock.c',
    'FPByteRangeLockExt.c',
    'FPCatSearch.c',
    'FPCatSearchExt.c',
    'FPCloseDir.c',
    'FPCloseDT.c',
    'FPCloseFork.c',
    'FPCloseVol.c',
    'FPCopyFile.c',
    'FPCreateDir.c',
    'FPCreateFile.c',
    'FPDelete.c',
    'FPDisconnectOldSession.c',
    'FPEnumerate.c',
    'FPEnumerateExt.c',
    'FPEnumerateExt2.c',
    'FPExchangeFiles.c',
    'FPFlush.c',
    'FPFlushFork.c',
    'FPGetACL.c',
    'FPSetACL.c',
    'FPGetAPPL.c',
    'FPGetComment.c',
    'FPGetExtAttr.c',
    'FPGetFileDirParms.c',
    'FPGetForkParms.c',
    'FPGetIcon.c',
    'FPGetIconInfo.c',
    'FPGetSessionToken.c',
    'FPGetSrvrInfo.c',
    'FPGetSrvrMsg.c',
    'FPGetSrvrParms.c',
    'FPGetUserInfo.c',
    'FPGetVolParms.c',
    'FPMapID.c',
    'FPMapName.c',
    'FPMoveAndRename.c',
    'FPOpenDT.c',
    'FPOpenDir.c',
    'FPOpenFork.c',
    'FPOpenVol.c',
    'FPRead.c',
    'FPReadExt.c',
    'FPRename.c',
    'FPRemoveAPPL.c',
    'FPRemoveComment.c',
    'FPResolveID.c',
    'FPSetDirParms.c',
    'FPSetFileDirParms.c',
    'FPSetFileParms.c',
    'FPSetForkParms.c',
    'FPSetVolParms.c',
    'FPSync.c',
    'FPWrite.c',
    'FPWriteExt.c',
    'T3_FPRead.c',
    'T3_FPzzz.c',
    'T2_Dircache_attack.c',
    'T2_FPByteRangeLock.c',
    'T2_FPCopyFile.c',
    'T2_FPCreateFile.c',
    'T2_FPDelete.c',
    'T2_FPGetFileDirParms.c',
    'T2_FPGetSrvrParms.c',
    'T2_FPGetVolParms.c',
    'T2_FPMoveAndRename.c',
    'T2_FPOpenFork.c',
    'T2_FPRead.c',
    'T2_FPResolveID.c',
    'T2_FPSetDirParms.c',
    'T2_FPSetFileParms.c',
    'T2_FPSetForkParms.c',
    'T2_Lock_attack.c',
    'Utf8.c',
    'encoding_test.c',
    'rotest.c',
    'spectest.c',
]

if have_spotlight
    spectest_sources += ['FPSpotlightRPC.c']
endif

executable(
    'afp_spectest',
    spectest_sources,
    include_directories: root_includes,
    dependencies: afptest_external_deps,
    c_args: spectest_extra_c_args,
    link_args: ['-rdynamic'],
    link_with: libafptest,
    install: true,
    install_rpath: rpath_libdir,
    build_rpath: rpath_libdir,
)
