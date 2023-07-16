subdir('src')
subdir('wolfcrypt')

ssl_sources = [
  src_sources,
  wc_sources]

libssl = static_library(
    'ssl',
    ssl_sources,
    include_directories : root_includes,
    install : false,
)
  