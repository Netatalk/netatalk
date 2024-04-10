FROM debian:bookworm-slim

ENV LIB_DEPS \
    libavahi-client3 \
    libdb5.3 \
    libevent-2.1-7 \
    libgcrypt20 \
    libglib2.0-0 \
    libpam0g \
    libssl3 \
    libtalloc2 \
    libtracker-sparql-3.0-0 \
    systemtap \
    tracker
ENV BUILD_DEPS \
    bison \
    build-essential \
    file \
    flex \
    libavahi-client-dev \
    libcups2-dev \
    libdb5.3-dev \
    libevent-dev \
    libgcrypt20-dev \
    libglib2.0-dev \
    libltdl-dev \
    libpam0g-dev \
    libssl-dev \
    libtalloc-dev \
    libtracker-sparql-3.0-dev \
    meson \
    ninja-build \
    pkg-config \
    python3-pip \
    systemtap-sdt-dev
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install --yes --no-install-recommends \
    $LIB_DEPS \
    $BUILD_DEPS


RUN useradd builder
WORKDIR /netatalk-code
COPY . .
RUN chown -R builder:builder .
USER builder

RUN meson setup build -Dbuild-manual=false
RUN ninja -C build

USER root
RUN userdel builder && ninja -C build install

WORKDIR /mnt
RUN apt-get remove --yes --auto-remove --purge $BUILD_DEPS && \
    apt-get --quiet --yes autoclean && \
    apt-get --quiet --yes autoremove && \
    apt-get --quiet --yes clean
RUN rm -rf \
    /netatalk-code \
    /usr/include/netatalk \
    /usr/share/man \
    /usr/share/mime \
    /usr/share/doc \
    /usr/share/poppler \
    /var/lib/apt/lists \
    /tmp \
    /var/tmp
RUN ln -sf /dev/stdout /var/log/afpd.log
COPY contrib/shell_utils/docker-entrypoint.sh /docker-entrypoint.sh
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/docker-entrypoint.sh"]
