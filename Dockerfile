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
    "

FROM alpine:3.22.1@sha256:4bcff63911fcb4448bd4fdacec207030997caf25e9bea4045fa6c8c44de311d1 AS build

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
    -Dwith-acls=false \
    -Dwith-afpstats=false \
    -Dwith-appletalk=true \
    -Dwith-cnid-backends=dbd,mysql,sqlite \
    -Dwith-dbus-daemon-path=/usr/bin/dbus-daemon \
    -Dwith-dbus-sysconf-path=/etc \
    -Dwith-docs= \
    -Dwith-dtrace=false \
    -Dwith-init-style=none \
    -Dwith-pkgconfdir-path=/etc/netatalk \
    -Dwith-quota=false \
    -Dwith-spotlight=false \
    -Dwith-tcp-wrappers=false \
&&  meson compile -C build

RUN meson install --destdir=/staging/ -C build

FROM alpine:3.22.1@sha256:4bcff63911fcb4448bd4fdacec207030997caf25e9bea4045fa6c8c44de311d1 AS deploy

ARG RUN_DEPS
ENV RUN_DEPS=$RUN_DEPS

COPY --from=build /staging/ /
COPY --from=build /netatalk-code/contrib/scripts/config_watch.sh /config_watch.sh

RUN apk update \
&&  apk add --no-cache $RUN_DEPS

RUN ln -sf /dev/stdout /var/log/afpd.log

COPY /contrib/scripts/netatalk_container_entrypoint.sh /entrypoint.sh

WORKDIR /mnt
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
