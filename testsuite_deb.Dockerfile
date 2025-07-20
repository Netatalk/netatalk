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
    libsqlite3-0 \
    libssl3 \
    libtirpc3t64 \
    libwrap0 \
    mariadb-client \
    systemtap \
    "
ARG BUILD_DEPS="\
    build-essential \
    file \
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
    libsqlite3-dev \
    libtirpc-dev \
    libwrap0-dev \
    meson \
    ninja-build \
    pkg-config \
    systemtap-sdt-dev \
    "

FROM debian:trixie-slim@sha256:88ef4df0f82963ff3c0472493da188f082822b2a16b1be23d238d124d5c8c92e AS build

ARG RUN_DEPS
ARG BUILD_DEPS
ENV RUN_DEPS=$RUN_DEPS
ENV BUILD_DEPS=$BUILD_DEPS

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
&&  apt-get install --yes --no-install-recommends \
    $RUN_DEPS \
    $BUILD_DEPS

WORKDIR /netatalk-code
COPY . .
RUN rm -rf build

RUN meson setup build \
    -Dbuildtype=release \
    -Dwith-afpstats=true \
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
    -Dwith-spotlight=false \
    -Dwith-tcp-wrappers=false \
    -Dwith-testsuite=true \
&&  meson compile -C build

RUN meson install --destdir=/staging/ -C build

FROM debian:trixie-slim@sha256:88ef4df0f82963ff3c0472493da188f082822b2a16b1be23d238d124d5c8c92e AS deploy

ARG RUN_DEPS
ENV RUN_DEPS=$RUN_DEPS

COPY --from=build /staging/ /

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
&&  apt-get install --yes --no-install-recommends $RUN_DEPS

COPY /contrib/scripts/netatalk_container_entrypoint.sh /entrypoint.sh

WORKDIR /mnt
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
