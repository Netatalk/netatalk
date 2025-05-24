subdir('data')

afptest_external_deps = []

if threads.found()
    afptest_external_deps += threads
endif

libafptest_sources = [
    'adoublehelper.c',
    'afpcli.c',
    'afpcmd.c',
    'afphelper.c',
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
    'Error.c',
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
    'FPzzz.c',
    'T2_Dircache_attack.c',
    'T2_FPByteRangeLock.c',
    'T2_FPCopyFile.c',
    'T2_FPCreateFile.c',
    'T2_FPDelete.c',
    'T2_FPGetFileDirParms.c',
    'T2_FPGetSrvrParms.c',
    'T2_FPMoveAndRename.c',
    'T2_FPOpenFork.c',
    'T2_FPRead.c',
    'T2_FPResolveID.c',
    'T2_FPSetDirParms.c',
    'T2_FPSetFileParms.c',
    'T2_FPSetForkParms.c',
    'Utf8.c',
    'encoding_test.c',
    'rotest.c',
    'spectest.c',
]

executable(
    'afp_spectest',
    spectest_sources,
    include_directories: root_includes,
    c_args: ['-D_PATH_TESTDATA_DIR="' + datadir + '/netatalk/test-data"'],
    link_args: ['-rdynamic'],
    link_with: libafptest,
    install: true,
)
