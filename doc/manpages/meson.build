if pandoc.found()
    transcoder = pandoc
elif cmarkgfm.found()
    transcoder = cmarkgfm
elif cmark.found()
    transcoder = cmark
else
    warning('No supported Markdown parser found')
endif

make_man = find_program('make_man.sh')

subdir('man1')
subdir('man5')
subdir('man8')
