FROM debian:bookworm-slim

# Debugging, optional
# ENV DEBUG_DEPS net-tools iputils-ping vim procps
# Support reading tool man pages, and dos2unix for convenience
# ENV TOOL_DEPS dos2unix man
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
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install --yes --no-install-recommends $LIB_DEPS $BUILD_DEPS && apt-get clean

WORKDIR /build
COPY . .

RUN [ -f ./bootstrap ] && ./bootstrap
RUN ./configure \
    --enable-overwrite \
    --prefix=/usr
RUN make clean && make -j $(nproc) && make install

WORKDIR /mnt/afpshare

RUN rm -rf /build && apt-get remove --yes --auto-remove --purge $BUILD_DEPS
RUN ln -sf /dev/stdout /var/log/afpd.log

COPY contrib/shell_utils/docker-entrypoint.sh /docker-entrypoint.sh

EXPOSE 548 631

CMD ["/docker-entrypoint.sh"]
