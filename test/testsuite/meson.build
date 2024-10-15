afptest_external_deps = []

if threads.found()
    afptest_external_deps += threads
endif

libafptest_sources = [
    'afpcli.c',
    'afpcmd.c',
    'afphelper.c',
    'adoublehelper.c',
    'extattr.c',
    'compat.c',
]

libafptest = static_library(
    'afptest',
    libafptest_sources,
    include_directories: root_includes,
)

afparg_sources = [
    'afparg.c',
    'afparg_FPfuncs.c',
]

afparg = executable(
    'afparg',
    afparg_sources,
    include_directories: root_includes,
    link_with: libafptest,
)

lantest_sources = [
    'lantest.c',
]

lantest = executable(
    'lantest',
    lantest_sources,
    include_directories: root_includes,
    dependencies: afptest_external_deps,
    link_with: libafptest,
)

afp_ls_sources = [
    'afp_ls.c',
]

afp_ls = executable(
    'afp_ls',
    afp_ls_sources,
    include_directories: root_includes,
    link_with: libafptest,
)

encoding_test_sources = [
    'encoding_test.c',
]

encoding_test = executable(
    'encoding_test',
    encoding_test_sources,
    include_directories: root_includes,
    link_with: libafptest,
)

login_test_sources = [
    'logintest.c',
]

login_test = executable(
    'logintest',
    login_test_sources,
    include_directories: root_includes,
    link_with: libafptest,
)

rotest_sources = [
    'rotest.c',
]

rotest = executable(
    'rotest',
    rotest_sources,
    include_directories: root_includes,
    link_with: libafptest,
)

speedtest_sources = [
    'speedtest.c',
]

speedtest = executable(
    'speedtest',
    speedtest_sources,
    include_directories: root_includes,
    link_with: libafptest,
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
]

spectest = executable(
    'spectest',
    spectest_sources,
    include_directories: root_includes,
    link_with: libafptest,
)

sleeptest_sources = [
    'sleeptest.c',
    'FPzzz.c',
]

sleeptest = executable(
    'sleeptest',
    sleeptest_sources,
    include_directories: root_includes,
    link_with: libafptest,
)

fail_spectest_sources = [
    'fail_spectest.c',
    'failed_FPEnumerate.c',
    'failed_FPExchangeFiles.c',
	'failed_FPMoveAndRename.c',
    'failed_Error.c',
]

fail_spectest = executable(
    'fail_spectest',
    fail_spectest_sources,
    include_directories: root_includes,
    link_with: libafptest,
    c_args: ['-DQUIRK'],
)

T2_spectest_sources = [
    'T2_spectest.c',
    'T2_FPByteRangeLock.c',
    'T2_FPDelete.c',
    'T2_FPGetFileDirParms.c',
    'T2_FPOpenFork.c',
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
]

T2_spectest = executable(
    'T2_spectest',
    T2_spectest_sources,
    include_directories: root_includes,
    link_with: libafptest,
)

spectest_sh = find_program('spectest.sh')

test(
    'spectest',
    spectest_sh,
    is_parallel: true,
)
