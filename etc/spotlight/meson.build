libspotlight_backends_libs = []

if use_search_localsearch
    subdir('localsearch')
    libspotlight_backends_libs += libspotlight_localsearch
endif

if use_search_cnid
    subdir('cnid')
    libspotlight_backends_libs += libspotlight_cnid
endif

# Meta-dependency: links afpd against all enabled backend libraries.
libspotlight_backends = declare_dependency(
    link_with: libspotlight_backends_libs,
)
