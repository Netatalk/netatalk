# See https://github.com/Netatalk/netatalk/wiki/Testing for guidance

ARG RUN_DEPS="\
    ca-certificates \
    cracklib-runtime \
    libacl1 \
    libavahi-client3 \
    libdb5.3t64 \
    libcups2t64 \
    libevent-2.1-7 \
    libgcrypt20 \
    libglib2.0-0 \
    libiniparser4 \
    libkrb5-3 \
    libldap2 \
    libmariadb3 \
    libpam0g \
    libreadline8t64 \
    libsqlite3-0 \
    libssl3 \
    libtalloc2 \
    libtinysparql-3.0-0 \
    libtirpc3t64 \
    libwrap0 \
    localsearch \
    mariadb-client \
    systemtap \
    "
ARG BUILD_DEPS="\
    bison \
    build-essential \
    file \
    flex \
    gnupg \
    libacl1-dev \
    libattr1-dev \
    libavahi-client-dev \
    libcrack2-dev \
    libcups2-dev \
    libdb-dev \
    libevent-dev \
    libgcrypt20-dev \
    libglib2.0-dev \
    libiniparser-dev \
    libkrb5-dev \
    libldap2-dev \
    libltdl-dev \
    libmariadb-dev \
    libpam0g-dev \
    libreadline-dev \
    libsqlite3-dev \
    libtalloc-dev \
    libtinysparql-dev \
    libtirpc-dev \
    libwrap0-dev \
    meson \
    ninja-build \
    pkg-config \
    systemtap-sdt-dev \
    xz-utils \
    "

FROM debian:13.5-slim@sha256:28de0877c2189802884ccd20f15ee41c203573bd87bb6b883f5f46362d24c5c2 AS build

ARG RUN_DEPS
ARG BUILD_DEPS
ENV RUN_DEPS=$RUN_DEPS
ENV BUILD_DEPS=$BUILD_DEPS

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
&&  apt-get install --yes --no-install-recommends \
    $RUN_DEPS \
    $BUILD_DEPS

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
    -Dbuildtype=debugoptimized \
    -Dwith-appletalk=true \
    -Dwith-dbus-daemon-path=/usr/bin/dbus-daemon \
    -Dwith-docs= \
    -Dwith-dtrace=false \
    -Dwith-init-style=none \
    -Dwith-krbV-uam=true \
    -Dwith-pam-config-path=/etc/pam.d \
    -Dwith-pkgconfdir-path=/etc/netatalk \
    -Dwith-rpath=false \
    -Dwith-spooldir=/var/spool/netatalk \
    -Dwith-spotlight=true \
    -Dwith-tcp-wrappers=false \
    -Dwith-tests=true \
    -Dwith-testsuite=true \
&&  meson compile -C build

RUN meson test -C build --print-errorlogs

RUN meson install --destdir=/staging/ -C build

FROM debian:13.5-slim@sha256:28de0877c2189802884ccd20f15ee41c203573bd87bb6b883f5f46362d24c5c2 AS deploy

ARG RUN_DEPS
ENV RUN_DEPS=$RUN_DEPS

COPY --from=build /staging/ /

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
&&  apt-get install --yes --no-install-recommends $RUN_DEPS

COPY --from=build /netatalk-code/distrib/docker/config_watch.sh /config_watch.sh
COPY /distrib/docker/env_setup_netatalk.sh /env_setup.sh
COPY /distrib/docker/entrypoint_netatalk.sh /entrypoint.sh

WORKDIR /mnt
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
