if host_machine.cpu_family() == 'aarch64'
    wolfssl_options = configure_file(input: 'options_aarch64.in', output: 'options.h', configuration: cdata)
elif host_machine.cpu_family() == 'arm'
    wolfssl_options = configure_file(input: 'options_arm.in', output: 'options.h', configuration: cdata)
elif host_machine.cpu_family() == 'x86_64'
    wolfssl_options = configure_file(input: 'options_x86_64.in', output: 'options.h', configuration: cdata)
endif
