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
    libtalloc-dev \
    libtinysparql-dev \
    libtirpc-dev \
    libwrap0-dev \
    meson \
    ninja-build \
    pkg-config \
    systemtap-sdt-dev \
    "

FROM debian:trixie-slim@sha256:cc92da07b99dd5c078cb5583fdb4ba639c7c9c14eb78508a2be285ca67cc738a AS build

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
    -Dwith-spotlight=true \
    -Dwith-tcp-wrappers=false \
    -Dwith-testsuite=true \
&&  meson compile -C build

RUN meson install --destdir=/staging/ -C build

FROM debian:trixie-slim@sha256:cc92da07b99dd5c078cb5583fdb4ba639c7c9c14eb78508a2be285ca67cc738a AS deploy

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
