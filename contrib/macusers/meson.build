if perl.found() and grep.found()
    macusers_script = configure_file(
        input: 'macusers.in',
        output: 'macusers',
        configuration: cdata,
    )

    install_data(macusers_script, install_dir: bindir)
endif