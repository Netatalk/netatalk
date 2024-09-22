# Netatalk Webmin Module

Webmin module for managing Netatalk.

It operates by parsing and modifying the Netatalk configuration files on the fly, and allowing you to start and stop the Netatalk services.

Note: The `netatalk` service needs to be restarted or reloaded after modifying afp.conf for the new settings to take effect.

# Installation

If you don't have it installed already, install Webmin itself by following the instructions at https://webmin.com/

## From release tarball

1. Download a [release tarball](https://github.com/Netatalk/netatalk/releases) distributed with the version of Netatalk you're using.
1. Install the tarball from within the Webmin UI, or on the CLI:
   * From the dir where Webmin is installed, run `./install-module.pl /path/to/netatalk_webmin-x.y.z.wbm.gz`
   * From within the Webmin UI: Configuration -> Modules -> select the tarball from the local file system

## From source

As of version 4.0.0, the Netatalk Webmin module is distributed with the Netatalk source code.
You package and install the module through the Netatalk build system.

From the root of of the Netatalk source tree, run:

```
meson setup build -Dwith-webmin=true
meson compile -C build
meson install -C build --tags webmin
```

If Webmin was detected on your system and you have the right privileges, the module will be installed automatically.
You can also find the packaged tarball in `build/contrib/webmin_module/netatalk_webmin-x.y.z.wbm.gz` for manual installation or distribution.

# Configuration

The build system will attempt to detect and configure the correct binary and config file paths, as well as init commands, for your system.
If needed, you can make manual adjustments in the `config` file or via the Webmin UI, on the Netatalk module's Module Config page.

# See Also
- https://netatalk.io/docs/Webmin-Module
