FROM debian:bookworm-slim

ENV LIB_DEPS \
    libavahi-client3 \
    libdb5.3 \
    libevent-2.1-7 \
    libgcrypt20 \
    libglib2.0-0 \
    libldap-2.5-0 \
    libmariadb3 \
    libpam0g \
    libtalloc2 \
    libtracker-sparql-3.0-0 \
    libwrap0 \
    tracker \
    tracker-miner-fs
ENV BUILD_DEPS \
    bison \
    build-essential \
    default-libmysqlclient-dev \
    file \
    flex \
    libacl1-dev \
    libattr1-dev \
    libavahi-client-dev \
    libdb5.3-dev \
    libevent-dev \
    libgcrypt20-dev \
    libldap2-dev \
    libpam0g-dev \
    libtalloc-dev \
    libtracker-sparql-3.0-dev \
    meson \
    ninja-build \
    pkg-config
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
&&  apt-get remove --yes --auto-remove --purge \
&&  apt-get install --yes --no-install-recommends \
    $LIB_DEPS \
    $BUILD_DEPS \
&&  useradd builder

WORKDIR /netatalk-code
COPY . .
RUN chown -R builder:builder . \
&&  rm -rf build || true
USER builder

RUN meson setup build \
    -Denable-pgp-uam=disabled \
    -Dwith-afpstats=disabled \
    -Dwith-dbus-daemon=/usr/bin/dbus-daemon \
    -Dwith-dbus-sysconf-dir=/etc \
    -Dwith-dtrace=false \
    -Dwith-embedded-ssl=true \
    -Dwith-init-style=none \
&&  ninja -C build

USER root

RUN userdel builder \
&&  ninja -C build install \
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
    /var/lib/apt/lists \
    /tmp \
    /var/tmp \
&&  ln -sf /dev/stdout /var/log/afpd.log

WORKDIR /mnt
COPY contrib/shell_utils/docker-entrypoint.sh /entrypoint.sh
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
