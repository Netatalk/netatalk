subdir('data')

t2_fpopenfork_c = configure_file(
    input: 'T2_FPOpenFork.c.in',
    output: 'T2_FPOpenFork.c',
    configuration: cdata,
)

afptest_external_deps = []

if threads.found()
    afptest_external_deps += threads
endif

libafptest_sources = [
    'afpcli.c',
    'afpcmd.c',
    'afphelper.c',
    'adoublehelper.c',
]

libafptest = static_library(
    'afptest',
    libafptest_sources,
    include_directories: root_includes,
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
    link_args: ['-rdynamic'],
    link_with: libafptest,
    install: true,
)

lantest_sources = [
    'lantest.c',
]

executable(
    'afp_lantest',
    lantest_sources,
    include_directories: root_includes,
    dependencies: afptest_external_deps,
    link_with: libafptest,
    install: true,
)

login_test_sources = [
    'logintest.c',
]

executable(
    'afp_logintest',
    login_test_sources,
    include_directories: root_includes,
    link_args: ['-rdynamic'],
    link_with: libafptest,
    install: true,
)

speedtest_sources = [
    'speedtest.c',
]

executable(
    'afp_speedtest',
    speedtest_sources,
    include_directories: root_includes,
    link_args: ['-rdynamic'],
    link_with: libafptest,
    install: true,
)

spectest_sources = [
    'spectest.c',
    'FPAddAPPL.c',
    'FPAddComment.c',
    'FPAddIcon.c',
    'FPByteRangeLock.c',
    'FPByteRangeLockExt.c',
    'FPCloseFork.c',
    'FPCopyFile.c',
    'FPCreateDir.c',
    'FPCreateFile.c',
    'FPCloseDir.c',
    'FPCloseDT.c',
    'FPCloseVol.c',
    'FPDelete.c',
    'FPExchangeFiles.c',
    'FPOpenDT.c',
    'FPOpenFork.c',
    'FPOpenVol.c',
    'FPGetAPPL.c',
    'FPGetIcon.c',
    'FPGetIconInfo.c',
    'FPGetSrvrInfo.c',
    'FPGetSrvrMsg.c',
    'FPGetSrvrParms.c',
    'FPGetComment.c',
    'FPGetExtAttr.c',
    'FPGetForkParms.c',
    'FPGetFileDirParms.c',
    'FPGetVolParms.c',
    'FPGetUserInfo.c',
    'FPEnumerate.c',
    'FPEnumerateExt.c',
    'FPEnumerateExt2.c',
    'FPFlush.c',
    'FPFlushFork.c',
    'FPMapID.c',
    'FPMapName.c',
    'FPMoveAndRename.c',
    'FPOpenDir.c',
    'FPSetDirParms.c',
    'FPCatSearch.c',
    'FPCatSearchExt.c',
    'FPSetFileDirParms.c',
    'FPSetFileParms.c',
    'FPDisconnectOldSession.c',
    'FPSetForkParms.c',
    'FPSetVolParms.c',
    'FPRead.c',
    'FPReadExt.c',
    'FPRename.c',
    'FPRemoveAPPL.c',
    'FPRemoveComment.c',
    'FPResolveID.c',
    'FPGetSessionToken.c',
    'FPWrite.c',
    'FPWriteExt.c',
    'FPzzz.c',
    'Error.c',
    'Utf8.c',
    'FPGetACL.c',
    'FPSync.c',
    'FPzzz.c',
    'rotest.c',
    'T2_FPByteRangeLock.c',
    'T2_FPDelete.c',
    'T2_FPGetFileDirParms.c',
    'T2_FPGetSrvrParms.c',
    'T2_FPSetDirParms.c',
    'T2_FPSetFileParms.c',
	'T2_FPMoveAndRename.c',
    'T2_FPResolveID.c',
    'T2_FPGetSrvrParms.c',
    'T2_FPCreateFile.c',
	'T2_FPCopyFile.c',
    'T2_Dircache_attack.c',
    'T2_FPRead.c',
    'T2_FPSetForkParms.c',
    'encoding_test.c',
    t2_fpopenfork_c,
]

executable(
    'afp_spectest',
    spectest_sources,
    include_directories: root_includes,
    link_args: ['-rdynamic'],
    link_with: libafptest,
    install: true,
)
