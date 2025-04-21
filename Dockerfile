ARG RUN_DEPS="\
    acl \
    avahi \
    cups \
    db \
    dbus \
    iniparser \
    krb5 \
    libevent \
    libgcrypt \
    linux-pam \
    localsearch \
    mariadb-client \
    mariadb-connector-c \
    openldap \
    talloc \
    tinysparql \
    tzdata"
ARG BUILD_DEPS="\
    acl-dev \
    avahi-dev \
    bison \
    cups-dev \
    db-dev \
    dbus-dev \
    build-base \
    flex \
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
    talloc-dev \
    tinysparql-dev"

FROM alpine:3.21 AS build

ARG RUN_DEPS
ARG BUILD_DEPS
ENV RUN_DEPS=$RUN_DEPS
ENV BUILD_DEPS=$BUILD_DEPS

RUN apk update \
&&  apk add --no-cache \
    $RUN_DEPS \
    $BUILD_DEPS

WORKDIR /netatalk-code
COPY . .

RUN meson setup build \
    -Dbuildtype=release \
    -Dwith-afpstats=false \
    -Dwith-appletalk=true \
    -Dwith-dbus-daemon-path=/usr/bin/dbus-daemon \
    -Dwith-dbus-sysconf-path=/etc \
    -Dwith-docs= \
    -Dwith-dtrace=false \
    -Dwith-init-style=none \
    -Dwith-pkgconfdir-path=/etc/netatalk \
    -Dwith-quota=false \
    -Dwith-tcp-wrappers=false \
&&  meson compile -C build

RUN meson install --destdir=/staging/ -C build

FROM alpine:3.21 AS deploy

ARG RUN_DEPS
ENV RUN_DEPS=$RUN_DEPS

COPY --from=build /staging/ /
COPY --from=build /netatalk-code/contrib/shell_utils/config_watch.sh /config_watch.sh

RUN apk update \
&&  apk add --no-cache $RUN_DEPS

RUN ln -sf /dev/stdout /var/log/afpd.log

COPY /contrib/shell_utils/netatalk_container_entrypoint.sh /entrypoint.sh

WORKDIR /mnt
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
