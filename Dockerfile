FROM alpine:3.20

ENV LIB_DEPS \
    acl \
    avahi \
    avahi-compat-libdns_sd \
    bash \
    cups \
    db \
    dbus \
    krb5 \
    libevent \
    libgcrypt \
    libtracker \
    linux-pam \
    nettle \
    openldap \
    talloc \
    tracker \
    tracker-miners \
    tzdata
ENV BUILD_DEPS \
    acl-dev \
    avahi-dev \
    bison \
    cups-dev \
    curl \
    db-dev \
    dbus-dev \
    build-base \
    flex \
    gcc \
    krb5-dev \
    libevent-dev \
    libgcrypt-dev \
    linux-pam-dev \
    meson \
    ninja \
    nettle-dev \
    openldap-dev \
    perl \
    pkgconfig \
    talloc-dev \
    tracker-dev \
    unicode-character-database
RUN apk update \
&&  apk add --no-cache \
    $LIB_DEPS \
    $BUILD_DEPS \
&&  addgroup builder \
&&  adduser \
    --disabled-password \
    --ingroup builder \
    --no-create-home \
    builder

WORKDIR /netatalk-code
COPY . .
RUN chown -R builder:builder . \
&&  rm -rf build || true
USER builder

RUN meson setup build \
    -Dwith-afpstats=false \
    -Dwith-appletalk=true \
    -Dwith-dbus-daemon-path=/usr/bin/dbus-daemon \
    -Dwith-dbus-sysconf-path=/etc \
    -Dwith-dtrace=false \
    -Dwith-embedded-ssl=true \
    -Dwith-init-style=none \
    -Dwith-manual=none \
    -Dwith-pgp-uam=false \
    -Dwith-quota=false \
    -Dwith-tcp-wrappers=false \
&&  meson compile -C build

USER root

RUN meson install -C build \
&&  apk del $BUILD_DEPS \
&&  rm -rf \
    /netatalk-code \
    /usr/local/include/atalk \
    /var/tmp \
    /tmp \
&&  ln -sf /dev/stdout /var/log/afpd.log

WORKDIR /mnt
COPY contrib/shell_utils/docker-entrypoint.sh /entrypoint.sh
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
