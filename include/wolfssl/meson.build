if 'aarch64'.contains(host_machine.cpu_family())

    wolfssl_options = configure_file(input: 'options_aarch64.in', output: 'options.h', configuration: cdata)

elif 'x86_64'.contains(host_machine.cpu_family())

    wolfssl_options = configure_file(input: 'options_x86_64.in', output: 'options.h', configuration: cdata)
endif
