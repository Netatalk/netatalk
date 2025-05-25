# Netatalk Webmin Module

Webmin module for managing Netatalk.

It operates by parsing and modifying the Netatalk configuration files on the fly,
and allowing you to start and stop the Netatalk services.

Note: The `netatalk` service needs to be restarted or reloaded after modifying afp.conf
for the new settings to take effect.

## Installation

If you don't have it installed already, install Webmin itself by following the instructions
at [webmin.com](https://webmin.com/)

### From release tarball

1. Download a [release tarball](https://github.com/Netatalk/netatalk/releases) distributed
with the version of Netatalk you're using.
1. Install the tarball from within the Webmin UI, or on the CLI:
   * From the dir where Webmin is installed, run `./install-module.pl /path/to/netatalk_webmin-x.y.z.wbm.gz`
   * From within the Webmin UI: Configuration -> Modules -> select the tarball from the local file system
1. Hit the gear icon next to the module name to configure it.
By default, the module will be configured to use systemd init commands and */usr* prefix directories.

### From source

As of version 4.0.0, the Netatalk Webmin module is distributed with the Netatalk source code.
You package and install the module through the Netatalk build system.

From the root of of the Netatalk source tree, run:

```shell
meson setup build -Dwith-webmin=true
meson compile -C build
meson install -C build --tags webmin
```

If Webmin was detected on your system and you have the right privileges, the module will be installed automatically.
You can also find the packaged tarball in `build/contrib/webmin_module/netatalk_webmin-x.y.z.wbm.gz`
for manual installation or distribution.

## Configuration

When installed from source, the build system will attempt to detect and configure
the correct binary and config file paths, as well as init commands, for your system.
If needed, you can make manual adjustments in the `config` file or via the Webmin UI,
on the Netatalk module's Module Config page.

## Authors and License

The original Webmin module was written by Matthew Keller and Sven Mosimann.
Later additions by Frank Lahm and Steffan Cline.
Rewritten for Netatalk 3.0 by Ralph Boehme.
Adapted for Netatalk 4.0 by Daniel Markstedt.

The source code is distributed under the GNU GPLv2 license.
See `COPYING` in the root of the Netatalk source tree for the full text of the license.

Bitmap images under `images/`, with the exception of `icon.gif` (the Netatalk logo),
are borrowed from the [Webmin Authentic Theme](https://github.com/webmin/authentic-theme).
The Authentic Theme is created by Ilia Rostovtsev, licensed under the MIT license.
See `images/LICENSE` for the full text of the license.

## See Also

* [Webmin Module wiki page](https://netatalk.io/docs/Webmin-Module)
