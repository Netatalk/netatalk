manpages_command = [
    xsltproc,
    '--stringparam',
    'man.output.quietly',
    '1',
    '-o',
    '@OUTPUT@',
    manpages_stylesheet,
    '@INPUT@',
]

subdir('man1')
subdir('man3')
subdir('man4')
subdir('man5')
subdir('man8')
