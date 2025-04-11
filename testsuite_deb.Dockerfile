ARG RUN_DEPS="\
    cracklib-runtime \
    libavahi-client3 \
    libdb5.3t64 \
    libevent-2.1-7 \
    libgcrypt20 \
    libglib2.0-0 \
    libiniparser4 \
    libldap2 \
    libmariadb3 \
    libpam0g \
    libssl3 \
    libtalloc2 \
    libtinysparql-3.0-0 \
    libwrap0 \
    localsearch \
    mariadb-client \
    systemtap \
    tracker\
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
    libtalloc-dev \
    libtirpc-dev \
    libtinysparql-dev \
    libwrap0-dev \
    meson \
    ninja-build \
    pkg-config \
    systemtap-sdt-dev\
    "

FROM debian:trixie-slim AS build

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
    -Dwith-rpath=false \
    -Dwith-spooldir=/var/spool/netatalk \
    -Dwith-tcp-wrappers=false \
    -Dwith-testsuite=true \
    -Dwith-tracker-prefix=/usr \
&&  meson compile -C build

RUN meson install --destdir=/staging/ -C build

FROM debian:trixie-slim AS deploy

ARG RUN_DEPS
ENV RUN_DEPS=$RUN_DEPS

COPY --from=build /staging/ /

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
&&  apt-get install --yes --no-install-recommends $RUN_DEPS

COPY /contrib/shell_utils/docker-entrypoint.sh /entrypoint.sh

WORKDIR /mnt
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
