nad_sources = [
    'crc16.c',
    'ftw.c',
    'hqx.c',
    'macbin.c',
    'megatron.c',
    'nad_adouble.c',
    'nad_cp.c',
    'nad_find.c',
    'nad_ls.c',
    'nad_mkdir.c',
    'nad_mv.c',
    'nad_rm.c',
    'nad_rmdir.c',
    'nad_set.c',
    'nad_util.c',
    'nad.c',
]

nad_deps = [bstring, iniparser]
nad_c_args = [naddir, dversion]

if have_iconv
    nad_deps += iconv
endif

if use_mysql_backend
    nad_deps += mysqlclient
endif

if get_option('with-stuffit')
    nad_sources += 'nad_stuffit.c'

    stuffit = dependency('stuffit-ffi', required: false)
    if not stuffit.found()
        stuffit_local_include = include_directories('/usr/local/include')
        stuffit_local_lib = cc.find_library(
            'stuffit_ffi',
            dirs: '/usr/local/lib',
            required: false,
        )

        if stuffit_local_lib.found() and cc.has_header(
            'stuffit_ffi.h',
            include_directories: stuffit_local_include,
        )
            stuffit = declare_dependency(
                dependencies: stuffit_local_lib,
                include_directories: stuffit_local_include,
            )
        else
            error('stuffit-ffi is required for StuffIt support in nad')
        endif
    endif

    nad_deps += stuffit
    have_stuffit = true
    nad_c_args += '-DHAVE_STUFFIT'
endif

executable(
    'nad',
    nad_sources,
    include_directories: root_includes,
    dependencies: nad_deps,
    link_with: libatalk,
    c_args: nad_c_args,
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
