FROM debian:bookworm-slim

ENV LIB_DEPS cups \
    libavahi-client3 \
    libcups2 \
    libdb5.3 \
    libgcrypt20 \
    libpam0g \
    libssl3
ENV BUILD_DEPS autoconf \
    automake \
    build-essential \
    libavahi-client-dev \
    libcups2-dev \
    libdb-dev \
    libgcrypt20-dev \
    libltdl-dev \
    libpam0g-dev \
    libssl-dev \
    libtool-bin \
    pkg-config
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install --yes --no-install-recommends $LIB_DEPS $BUILD_DEPS && apt-get clean

RUN useradd builder
WORKDIR /build
COPY . .
RUN chown -R builder:builder .
USER builder

RUN [ -f ./bootstrap ] && ./bootstrap
RUN ./configure \
    --enable-overwrite \
    --prefix=/usr
RUN make clean && make -j $(nproc)

USER root
RUN userdel builder && make install

WORKDIR /mnt/afpshare

RUN rm -rf /build && apt-get remove --yes --auto-remove --purge $BUILD_DEPS
RUN ln -sf /dev/stdout /var/log/afpd.log

COPY contrib/shell_utils/docker-entrypoint.sh /docker-entrypoint.sh

EXPOSE 548 631
VOLUME ["/mnt/afpshare"]

CMD ["/docker-entrypoint.sh"]
