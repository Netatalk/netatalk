FROM alpine:3.19

ENV LIB_DEPS \
    acl \
    avahi \
    bash \
    db \
    dbus \
    dbus-glib \
    krb5 \
    libevent \
    libgcrypt \
    linux-pam \
    openldap \
    talloc \
    tracker \
    tracker-miners
ENV BUILD_DEPS \
    acl-dev \
    avahi-dev \
    bison \
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
    openldap-dev \
    pkgconfig \
    talloc-dev \
    tracker-dev
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
    -Denable-pgp-uam=disabled \
    -Dwith-dbus-daemon=/usr/bin/dbus-daemon \
    -Dwith-dbus-sysconf-dir=/etc \
    -Dwith-afpstats=disabled \
    -Dwith-dtrace=false \
    -Dwith-embedded-ssl=true \
    -Dwith-init-style=none \
&&  ninja -C build

USER root

RUN ninja -C build install \
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
