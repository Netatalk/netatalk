if have_atfuncs and have_bdb
    subdir('ad')
endif
subdir('adv1tov2')
if have_ssl
    subdir('afppasswd')
endif
if perl.found() and (use_dbd_backend or use_last_backend)
    subdir('cnid')
endif
subdir('megatron')
subdir('uniconv')
subdir('misc')
if have_ddp
    subdir('aecho')
    subdir('getzones')
    subdir('nbp')
    subdir('pap')
    subdir('psorder')
endif
