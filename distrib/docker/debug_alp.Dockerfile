# Debug / profiling Alpine Dockerfile for Netatalk
# Builds with -Dbuildtype=debugoptimized (-O2 -g) and includes perf + FlameGraph tools
# See doc/developer/profiling.md for usage guidance

ARG RUN_DEPS="\
    acl \
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
    sqlite-dev \
    talloc-dev \
    tinysparql-dev \
    "
ARG DEBUG_DEPS="\
    binutils \
    git \
    perf \
    perl \
    "

FROM alpine:3.23.4@sha256:5b10f432ef3da1b8d4c7eb6c487f2f5a8f096bc91145e68878dd4a5019afde11 AS build

ARG RUN_DEPS
ARG BUILD_DEPS
ARG DEBUG_DEPS
ENV RUN_DEPS=$RUN_DEPS
ENV BUILD_DEPS=$BUILD_DEPS
ENV DEBUG_DEPS=$DEBUG_DEPS

RUN apk update \
&&  apk add --no-cache \
    $RUN_DEPS \
    $BUILD_DEPS \
    $DEBUG_DEPS

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
COPY test/ ./test/
COPY meson_config.h .
COPY meson_options.txt .
COPY meson.build .
RUN rm -rf build

# Build with debugoptimized: -O2 optimization + debug symbols (-g)
# Also preserve frame pointers for accurate perf stack unwinding
RUN meson setup build \
    -Dbuildtype=debugoptimized \
    -Dc_args=-fno-omit-frame-pointer \
    -Dwith-afpstats=false \
    -Dwith-appletalk=true \
    -Dwith-dbus-daemon-path=/usr/bin/dbus-daemon \
    -Dwith-dbus-sysconf-path=/etc \
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

RUN meson install --destdir=/staging/ -C build

FROM alpine:3.23.4@sha256:5b10f432ef3da1b8d4c7eb6c487f2f5a8f096bc91145e68878dd4a5019afde11 AS deploy

ARG RUN_DEPS
ARG DEBUG_DEPS
ENV RUN_DEPS=$RUN_DEPS
ENV DEBUG_DEPS=$DEBUG_DEPS

COPY --from=build /staging/ /

RUN apk update \
&&  apk add --no-cache $RUN_DEPS $DEBUG_DEPS musl-dbg

# Pre-clone FlameGraph tools so we don't need network at runtime
RUN git clone --depth 1 https://github.com/brendangregg/FlameGraph.git /opt/FlameGraph

COPY distrib/docker/config_watch.sh /config_watch.sh
COPY distrib/docker/env_setup_netatalk.sh /env_setup.sh
COPY distrib/docker/debug_entrypoint_netatalk.sh /entrypoint.sh

WORKDIR /mnt
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
