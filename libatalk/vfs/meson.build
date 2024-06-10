vfs_sources = ['ea_sys.c', 'ea.c', 'sys_ea.c', 'unix.c', 'vfs.c']

if have_acls
    vfs_sources += ['acl.c']
endif

libvfs = static_library(
    'vfs',
    vfs_sources,
    include_directories: root_includes,
    install: false,
)
