psf_c_args = [
    '-DZEROWIDTH',
    '-D_PATH_PAP="' + sbindir + '/pap"',
    '-D_PATH_PSORDER="' + bindir + '/psorder"',
    '-D_PATH_PSA="' + libexecdir + '/psa"',
    '-D_PATH_PSFILTER="' + libexecdir + '/etc2ps.sh"',
    '-D_PATH_PAGECOUNT="' + datadir + '/pagecount.ps"',
]

executable(
    'psa',
    'psa.c',
    include_directories: root_includes,
    link_with: libatalk,
    c_args: psf_c_args,
    export_dynamic: true,
    install: true,
    install_dir: libexecdir,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
executable(
    'psf',
    'psf.c',
    include_directories: root_includes,
    link_with: libatalk,
    c_args: psf_c_args,
    export_dynamic: true,
    install: true,
    install_dir: libexecdir,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)

install_data('pagecount.ps', install_dir: datadir / 'netatalk')

psf_symlinks = [
    'ofpap',
    'ifpap',
    'tfpap',
    'ifpaprev',
    'tfpaprev',
    'ofwpap',
    'ifwpap',
    'tfwpap',
    'ifwpaprev',
    'tfwpaprev',
    'ofmpap',
    'ifmpap',
    'tfmpap',
    'ifmpaprev',
    'tfmpaprev',
    'ofwmpap',
    'ifwmpap',
    'tfwmpap',
    'ifwmpaprev',
    'tfwmpaprev',
]

foreach link : psf_symlinks
    install_symlink(link, install_dir: libexecdir, pointing_to: 'psf')
endforeach
