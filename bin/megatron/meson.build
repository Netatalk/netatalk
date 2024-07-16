megatron_sources = [
    'asingle.c',
    'hqx.c',
    'macbin.c',
    'megatron.c',
    'nad.c',
    'updcrc.c',
]

if enable_rpath
    executable(
        'megatron',
        megatron_sources,
        include_directories: root_includes,
        link_with: libatalk,
        c_args: dversion,
        install: true,
        build_rpath: libdir,
        install_rpath: libdir,
    )
else
    executable(
        'megatron',
        megatron_sources,
        include_directories: root_includes,
        link_with: libatalk,
        c_args: dversion,
        install: true,
    )
endif

megatron_symlinks = [
    'binheader',
    'hqx2bin',
    'macbinary',
    'nadheader',
    'single2bin',
    'unbin',
    'unhex',
    'unsingle',
]

foreach link : megatron_symlinks
    install_symlink(link, install_dir: bindir, pointing_to: 'megatron')
endforeach
