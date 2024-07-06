FROM alpine:3.20

ENV LIB_DEPS \
    acl \
    avahi \
    avahi-compat-libdns_sd \
    bash \
    cups \
    db \
    krb5 \
    libgcrypt \
    linux-pam \
    openldap \
    openssl \
    shadow
ENV BUILD_DEPS \
    acl-dev \
    avahi-dev \
    cups-dev \
    curl \
    db-dev \
    build-base \
    gcc \
    krb5-dev \
    libgcrypt-dev \
    linux-pam-dev \
    meson \
    ninja \
    openldap-dev \
    openssl-dev \
    pkgconfig

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
    -Dwith-cracklib=false \
    -Dwith-embedded-ssl=true \
    -Dwith-init-style=none \
    -Dwith-pgp-uam=false \
    -Dwith-quota=false \
    -Dwith-tcp-wrappers=false \
&&  meson compile -C build

USER root
RUN meson install -C build \
&&  apk del $BUILD_DEPS && \
    rm -rf \
    /netatalk-code \
    /usr/local/include/atalk \
    /var/tmp \
    /tmp \
&&  ln -sf /dev/stdout /var/log/afpd.log

COPY contrib/shell_utils/docker-entrypoint.sh /entrypoint.sh
WORKDIR /mnt
EXPOSE 548 631
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
