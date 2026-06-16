ARG RUN_DEPS="\
    avahi \
    cups \
    db \
    dbus \
    glib \
    iniparser \
    krb5 \
    libevent \
    libgcrypt \
    linux-pam \
    mariadb-client \
    mariadb-connector-c \
    openldap \
    sqlite \
    talloc \
    tzdata \
    "
ARG BUILD_DEPS="\
    avahi-dev \
    cups-dev \
    db-dev \
    dbus-dev \
    build-base \
    gcc \
    iniparser-dev \
    krb5-dev \
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

WORKDIR /netatalk-code
COPY bin/ ./bin/
COPY config/ ./config/
COPY contrib/meson.build ./contrib/
COPY contrib/a2boot/ ./contrib/a2boot/
COPY contrib/bin_utils/ ./contrib/bin_utils/
COPY contrib/macipgw/ ./contrib/macipgw/
COPY contrib/timelord/ ./contrib/timelord/
COPY distrib/docker/ ./distrib/docker/
COPY etc/ ./etc/
COPY include/ ./include/
COPY libatalk/ ./libatalk/
COPY subprojects/ ./subprojects/
COPY sys/ ./sys/
COPY meson_config.h .
COPY meson_options.txt .
COPY meson.build .

RUN meson setup build \
    -Dbuildtype=release \
    -Dwith-acls=false \
    -Dwith-appletalk=true \
    -Dwith-cnid-backends=dbd,mysql,sqlite \
    -Dwith-docs= \
    -Dwith-dtrace=false \
    -Dwith-init-style=none \
    -Dwith-pkgconfdir-path=/etc/netatalk \
    -Dwith-quota=false \
    -Dwith-fce=false \
    -Dwith-spotlight=true \
    -Dwith-tcp-wrappers=false \
    -Dwith-tests=false \
    -Dwith-testsuite=false \
&&  meson compile -C build

RUN meson install --destdir=/staging/ -C build

FROM alpine:3.24.1@sha256:28bd5fe8b56d1bd048e5babf5b10710ebe0bae67db86916198a6eec434943f8b AS deploy

ARG RUN_DEPS
ENV RUN_DEPS=$RUN_DEPS

COPY --from=build /staging/ /
COPY --from=build /netatalk-code/distrib/docker/config_watch.sh /config_watch.sh

RUN apk update \
&&  apk add --no-cache $RUN_DEPS

RUN ln -sf /dev/stdout /var/log/afpd.log

COPY /distrib/docker/env_setup_netatalk.sh /env_setup.sh
COPY /distrib/docker/entrypoint_netatalk.sh /entrypoint.sh

WORKDIR /mnt
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
