project(
    'wolfssl',
    'c',
    version: '5.7.2',
    license: 'GPLv2',
    default_options: ['warning_level=3', 'c_std=c11'],
    meson_version: '>=0.61.2',
    # Reserved for future use:
    # meson_version: '>=0.62.0',
)

cdata = configuration_data()
cdata.set('undef', 'define')

threads = dependency('threads', required: true)

wolfssl_config = configure_file(
    input: 'conf.in',
    output: 'config.h',
    configuration: cdata,
)

wolfssl_includes = include_directories(
    '.',
    'wolfssl',
)

subdir('src')
subdir('wolfcrypt')

wolfssl_sources = [src_sources, wc_sources]

libwolfssl = shared_library(
    'wolfssl',
    wolfssl_sources,
    dependencies: threads,
    include_directories: wolfssl_includes,
    install: true,
)

libssl_dep = declare_dependency(
    include_directories : wolfssl_includes,
    link_with : libwolfssl
)
