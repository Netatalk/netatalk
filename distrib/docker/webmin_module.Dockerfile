FROM debian:13.1-slim@sha256:a347fd7510ee31a84387619a492ad6c8eb0af2f2682b916ff3e643eb076f925a

ARG RUN_DEPS="\
    ca-certificates \
    iproute2 \
    libdb5.3t64 \
    libevent-2.1-7 \
    libgcrypt20 \
    libiniparser4 \
    libpam0g \
    sudo \
    systemtap \
    "
ARG BUILD_DEPS="\
    build-essential \
    curl \
    file \
    libdb-dev \
    libevent-dev \
    libgcrypt20-dev \
    libiniparser-dev \
    libpam0g-dev \
    meson \
    ninja-build \
    pkg-config \
    systemtap-sdt-dev \
    "

ARG RUN_DEPS
ARG BUILD_DEPS
ENV RUN_DEPS=$RUN_DEPS
ENV BUILD_DEPS=$BUILD_DEPS
ENV PERLLIB=/usr/share/webmin

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
&&  apt-get install --yes --no-install-recommends \
    $RUN_DEPS \
    $BUILD_DEPS \
&&  curl -o webmin-setup-repo.sh \
    https://raw.githubusercontent.com/webmin/webmin/master/webmin-setup-repo.sh \
&&  sh webmin-setup-repo.sh --force \
&&  apt-get install --yes --no-install-recommends webmin \
&&  apt-get clean

WORKDIR /netatalk-code
COPY bin/ ./bin/
COPY config/ ./config/
COPY contrib/meson.build ./contrib/
COPY contrib/a2boot/ ./contrib/a2boot/
COPY contrib/bin_utils/ ./contrib/bin_utils/
COPY contrib/macipgw/ ./contrib/macipgw/
COPY contrib/timelord/ ./contrib/timelord/
COPY contrib/webmin_module/ ./contrib/webmin_module/
COPY distrib/docker/ ./distrib/docker/
COPY etc/ ./etc/
COPY include/ ./include/
COPY libatalk/ ./libatalk/
COPY subprojects/ ./subprojects/
COPY sys/ ./sys/
COPY test/ ./test/
COPY COPYING .
COPY meson_config.h .
COPY meson_options.txt .
COPY meson.build .

RUN sed -i 's/hide_service_controls=0/hide_service_controls=1/' /netatalk-code/contrib/webmin_module/config.in \
&&  meson setup build \
    -Dbuildtype=release \
    -Dwith-afpstats=false \
    -Dwith-appletalk=true \
    -Dwith-docs= \
    -Dwith-dtrace=false \
    -Dwith-init-style=none \
    -Dwith-krbV-uam=false \
    -Dwith-pkgconfdir-path=/etc/netatalk \
    -Dwith-rpath=false \
    -Dwith-spotlight=false \
    -Dwith-tcp-wrappers=false \
    -Dwith-tests=false \
    -Dwith-testsuite=false \
    -Dwith-webmin=true \
&&  meson compile -C build \
&&  meson install -C build \
&&  apt-get remove --yes --auto-remove --purge $BUILD_DEPS \
&&  apt-get --quiet --yes autoclean \
&&  apt-get --quiet --yes autoremove \
&&  apt-get --quiet --yes clean \
&&  rm -rf \
    /netatalk-code \
    /usr/include/netatalk \
    /usr/share/man \
    /usr/share/mime \
    /usr/share/doc \
    /usr/share/poppler \
    /var/lib/apt/lists

COPY /distrib/docker/entrypoint_webmin.sh /entrypoint.sh

WORKDIR /
EXPOSE 10000
CMD ["/entrypoint.sh"]
