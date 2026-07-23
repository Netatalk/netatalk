# See https://github.com/Netatalk/netatalk/wiki/Testing for guidance

ARG RUN_DEPS="\
    acl \
    avahi \
    cups \
    db \
    dbus \
    glib \
    iniparser \
    krb5 \
    libedit \
    libevent \
    libgcrypt \
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
    "
ARG BUILD_DEPS="\
    acl-dev \
    avahi-dev \
    bison \
    ca-certificates \
    cups-dev \
    db-dev \
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

FROM alpine:3.24.1@sha256:28bd5fe8b56d1bd048e5babf5b10710ebe0bae67db86916198a6eec434943f8b AS build

ARG RUN_DEPS
ARG BUILD_DEPS
ENV RUN_DEPS=$RUN_DEPS
ENV BUILD_DEPS=$BUILD_DEPS

RUN apk update \
&&  apk add --no-cache \
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
    -Dbuildtype=debugoptimized \
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

FROM alpine:3.24.1@sha256:28bd5fe8b56d1bd048e5babf5b10710ebe0bae67db86916198a6eec434943f8b AS deploy

ARG RUN_DEPS
ENV RUN_DEPS=$RUN_DEPS

COPY --from=build /staging/ /

RUN apk update \
&&  apk add --no-cache $RUN_DEPS

COPY --from=build /netatalk-code/distrib/docker/config_watch.sh /config_watch.sh
COPY /distrib/docker/env_setup_netatalk.sh /env_setup.sh
COPY /distrib/docker/entrypoint_netatalk.sh /entrypoint.sh

WORKDIR /mnt
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
