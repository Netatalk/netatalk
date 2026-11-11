# See https://github.com/Netatalk/netatalk/wiki/Testing for guidance

ARG RUN_DEPS="\
    acl \
    avahi \
    cups \
    dbus \
    glib \
    iniparser \
    krb5 \
    libedit \
    libevent \
    libgcrypt \
    libunwind \
    linux-pam \
    localsearch \
    mariadb \
    mariadb-client \
    mariadb-connector-c \
    openldap \
    sqlite \
    talloc \
    tinysparql \
    tzdata \
    util-linux-misc \
    "
ARG BUILD_DEPS="\
    acl-dev \
    avahi-dev \
    bison \
    ca-certificates \
    cups-dev \
    dbus-dev \
    build-base \
    flex \
    gcc \
    gnupg \
    iniparser-dev \
    krb5-dev \
    libedit-dev \
    libevent-dev \
    libgcrypt-dev \
    libunwind-dev \
    linux-pam-dev \
    mariadb-dev \
    meson \
    ninja \
    openldap-dev \
    pkgconfig \
    sqlite-dev \
    talloc-dev \
    tinysparql-dev \
    xz \
    "

FROM alpine:3.24.3@sha256:be2656061b536c1883c4034790e21d0a09c5aaaf51968b89368a3c0371ea2c9a AS build

ARG RUN_DEPS
ARG BUILD_DEPS
ENV RUN_DEPS=$RUN_DEPS
ENV BUILD_DEPS=$BUILD_DEPS

# Perf jobs pass release, so their numbers do not include the asserts
ARG BUILDTYPE=debugoptimized

# Perf jobs pass true, to build the newest SQLite release from source
ARG SQLITE_FROM_SOURCE=false

RUN apk update \
&&  apk add --no-cache \
    $RUN_DEPS \
    $BUILD_DEPS

WORKDIR /sqlite-code

# The flags and library options of aports main/sqlite/APKBUILD. pkgconf and
# the musl loader search /usr/local first, so every libsqlite3 user loads it
RUN if [ "$SQLITE_FROM_SOURCE" = true ]; then \
        apk add --no-cache curl \
    &&  curl -fsS -o download.html https://sqlite.org/download.html \
    &&  product=$(grep -m 1 -E '^PRODUCT,[0-9.]+,[0-9]{4}/sqlite-autoconf-[0-9]+\.tar\.gz,' download.html) \
    &&  curl -fsS -o sqlite.tar.gz "https://sqlite.org/$(echo "$product" | cut -d , -f 3)" \
    &&  echo "$(echo "$product" | cut -d , -f 5)  sqlite.tar.gz" | sha3sum -a 256 -c - \
    &&  tar -xzf sqlite.tar.gz --strip-components=1 \
    &&  CFLAGS="-O2 -fstack-clash-protection -Wformat -Werror=format-security \
            -DSQLITE_ENABLE_FTS3_PARENTHESIS \
            -DSQLITE_ENABLE_COLUMN_METADATA \
            -DSQLITE_SECURE_DELETE \
            -DSQLITE_ENABLE_UNLOCK_NOTIFY \
            -DSQLITE_ENABLE_RTREE \
            -DSQLITE_ENABLE_GEOPOLY \
            -DSQLITE_USE_URI \
            -DSQLITE_ENABLE_DBSTAT_VTAB \
            -DSQLITE_SOUNDEX \
            -DSQLITE_MAX_VARIABLE_NUMBER=250000" \
        LDFLAGS="-Wl,--as-needed,-O1,--sort-common -Wl,-z,pack-relative-relocs" \
        ./configure \
            --prefix=/usr/local \
            --enable-threadsafe \
            --disable-readline \
            --enable-session \
            --enable-static \
            --enable-fts3 \
            --enable-fts4 \
            --enable-fts5 \
            --soname=legacy \
    &&  make -j"$(nproc)" \
    &&  make install DESTDIR=/staging \
    &&  cp -a /staging/usr/local/. /usr/local/; \
    fi

WORKDIR /netatalk-client-code
ADD "https://netatalk.io/NetatalkDistributionPublicKey.asc" netatalk-distribution.asc
ADD "https://github.com/Netatalk/netatalk-client/releases/download/0.9.5/netatalk-client-0.9.5.tar.xz" netatalk-client.tar.xz
ADD "https://github.com/Netatalk/netatalk-client/releases/download/0.9.5/netatalk-client-0.9.5.tar.xz.asc" netatalk-client.tar.xz.asc
RUN gpg --batch --import netatalk-distribution.asc \
&&  gpg --batch --list-keys --with-colons 835A65428C822F69C45B817A7B13E1BFE4DDE8BD \
        | grep -q '^fpr:::::::::835A65428C822F69C45B817A7B13E1BFE4DDE8BD:' \
&&  gpg --batch --verify netatalk-client.tar.xz.asc netatalk-client.tar.xz \
&&  mkdir src \
&&  tar -xJf netatalk-client.tar.xz \
        -C src \
        --strip-components=1 \
&&  gpgconf --kill all \
&&  rm -rf \
        netatalk-distribution.asc \
        netatalk-client.tar.xz \
        netatalk-client.tar.xz.asc \
        /root/.gnupg

RUN meson setup src/build src \
    -Dbuildtype=debugoptimized \
    -Dc_link_args=-Wl,-Bsymbolic-functions \
    -Denable-docs=false \
&&  meson compile -C src/build \
&&  meson install --destdir=/staging/ -C src/build \
&&  cp -a /staging/usr/local/. /usr/local/

WORKDIR /netatalk-code
COPY bin/ ./bin/
COPY config/ ./config/
COPY contrib/meson.build ./contrib/
COPY contrib/a2boot/ ./contrib/a2boot/
COPY contrib/bin_utils/ ./contrib/bin_utils/
COPY contrib/macipgw/ ./contrib/macipgw/
COPY contrib/scripts/ ./contrib/scripts/
COPY contrib/timelord/ ./contrib/timelord/
COPY distrib/docker/ ./distrib/docker/
COPY etc/ ./etc/
COPY include/ ./include/
COPY libatalk/ ./libatalk/
COPY subprojects/ ./subprojects/
COPY sys/ ./sys/
COPY test/ ./test/
COPY meson_config.h .
COPY meson_options.txt .
COPY meson.build .
RUN rm -rf build

RUN meson setup build \
    -Dbuildtype=$BUILDTYPE \
    -Dwith-appletalk=true \
    -Dwith-dbus-daemon-path=/usr/bin/dbus-daemon \
    -Dwith-docs= \
    -Dwith-dtrace=false \
    -Dwith-init-style=none \
    -Dwith-pkgconfdir-path=/etc/netatalk \
    -Dwith-quota=false \
    -Dwith-spotlight=true \
    -Dwith-tcp-wrappers=false \
    -Dwith-tests=true \
    -Dwith-testsuite=true \
&&  meson compile -C build

RUN meson test -C build --print-errorlogs

RUN meson install --destdir=/staging/ -C build

FROM alpine:3.24.3@sha256:be2656061b536c1883c4034790e21d0a09c5aaaf51968b89368a3c0371ea2c9a AS deploy

ARG RUN_DEPS
ENV RUN_DEPS=$RUN_DEPS

COPY --from=build /staging/ /

RUN apk update \
&&  apk add --no-cache $RUN_DEPS

COPY --from=build /netatalk-code/distrib/docker/config_watch.sh /config_watch.sh
COPY /distrib/docker/env_setup_netatalk.sh /env_setup.sh
COPY /distrib/docker/entrypoint_netatalk.sh /entrypoint.sh
COPY /test/integration/ /integration/

WORKDIR /mnt
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
