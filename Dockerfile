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
    openssl \
    talloc \
    tracker
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
    openssl-dev \
    pkgconfig \
    talloc-dev \
    tracker-dev
RUN apk update && \
    apk add --no-cache \
    $LIB_DEPS \
    $BUILD_DEPS

RUN adduser -D -H builder
WORKDIR /netatalk-code
COPY . .
RUN chown -R builder:builder . && rm -rf build || true
USER builder

RUN meson setup build \
    -Denable-pgp-uam=disabled \
    -Dwith-dbus-daemon=/usr/bin/dbus-daemon \
    -Dwith-dbus-sysconf-dir=/etc \
    -Dwith-init-style=none && \
    ninja -C build

USER root
RUN deluser builder && ninja -C build install

WORKDIR /mnt
RUN apk del $BUILD_DEPS && \
    rm -rf \
    /netatalk-code \
    /usr/local/include/atalk \
    /var/tmp \
    /tmp && \
    ln -sf /dev/stdout /var/log/afpd.log
COPY contrib/shell_utils/docker-entrypoint.sh /entrypoint.sh
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
