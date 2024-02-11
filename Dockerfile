FROM debian:bullseye-slim

ENV LIB_DEPS \
    libavahi-client3 \
    libdb5.3 \
    libevent-2.1-7 \
    libgcrypt20 \
    libglib2.0-0 \
    libpam0g \
    libssl1.1 \
    libtalloc2 \
    libtracker-control-2.0-0 \
    libtracker-miner-2.0-0 \
    libtracker-sparql-2.0-0 \
    systemtap \
    tracker
ENV BUILD_DEPS \
    autoconf \
    automake \
    build-essential \
    libavahi-client-dev \
    libcups2-dev \
    libdb-dev \
    libevent-dev \
    libgcrypt20-dev \
    libglib2.0-dev \
    libltdl-dev \
    libpam0g-dev \
    libssl-dev \
    libtalloc-dev \
    libtool-bin \
    libtracker-control-2.0-dev \
    libtracker-miner-2.0-dev \
    libtracker-sparql-2.0-dev \
    pkg-config \
    systemtap-sdt-dev
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install --yes --no-install-recommends $LIB_DEPS $BUILD_DEPS && apt-get clean

WORKDIR /build
COPY . .

RUN [ -f ./bootstrap ] && ./bootstrap
RUN ./configure \
    --enable-overwrite \
    --prefix=/usr \
    --with-pam-confdir=/etc/pam.d \
    --with-dbus-daemon=/usr/bin/dbus-daemon \
    --with-tracker-pkgconfig-version=2.0
RUN make clean && make -j $(nproc) && make install

WORKDIR /mnt

RUN rm -rf /build && apt-get remove --yes --auto-remove --purge $BUILD_DEPS
RUN ln -sf /dev/stdout /var/log/afpd.log

COPY contrib/shell_utils/docker-entrypoint.sh /docker-entrypoint.sh

EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]

CMD ["/docker-entrypoint.sh"]
