vfs_sources = ['ea_ad.c', 'ea_sys.c', 'extattr.c', 'unix.c', 'vfs.c']

if have_acls
    vfs_sources += ['acl.c']
endif

libvfs = static_library(
    'vfs',
    vfs_sources,
    include_directories: root_includes,
    install: false,
)
