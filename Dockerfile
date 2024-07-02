FROM alpine:3.20

ENV LIB_DEPS \
    acl \
    bash \
    cups \
    db \
    krb5 \
    libgcrypt \
    libtirpc \
    linux-pam \
    openldap \
    rpcsvc-proto \
    shadow
ENV BUILD_DEPS \
    acl-dev \
    cups-dev \
    curl \
    db-dev \
    build-base \
    gcc \
    krb5-dev \
    libgcrypt-dev \
    libtirpc-dev \
    linux-pam-dev \
    meson \
    ninja \
    openldap-dev \
    openssl \
    openssl-dev \
    pkgconfig \
    rpcsvc-proto-dev

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
