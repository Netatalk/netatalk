# Bootstrap TLS with the CA bundle already included in the pinned Alpine image.
FROM alpine:3.24.3@sha256:be2656061b536c1883c4034790e21d0a09c5aaaf51968b89368a3c0371ea2c9a AS certificates

FROM ubuntu:26.04@sha256:0d5255be560e2d2ddcfbfda88549e9e815ef29e7cd7fc62183d5709e6e2963c6

ENV ABI_PROFILE=ubuntu26.04-x86_64-v1 HOME=/tmp

RUN --mount=type=bind,from=certificates,source=/etc/ssl/certs/ca-certificates.crt,target=/tmp/abi-ca-certificates.crt \
    sed -i -e 's|^URIs:.*|URIs: https://snapshot.ubuntu.com/ubuntu/20261110T120000Z/|' \
        -e '/^URIs:/a Check-Valid-Until: no' /etc/apt/sources.list.d/ubuntu.sources \
    && printf 'Acquire::https::CAInfo "/tmp/abi-ca-certificates.crt";\n' > /etc/apt/apt.conf.d/99abi-ca \
    && apt-get -o APT::Update::Error-Mode=any update \
    && apt-get install -y --no-install-recommends \
        abigail-tools \
        binutils \
        ca-certificates \
        gcc \
        git \
        libacl1-dev \
        libc6-dev \
        libevent-dev \
        libgcrypt20-dev \
        libiniparser-dev \
        libldap2-dev \
        libmariadb-dev \
        libsqlite3-dev \
        libtalloc-dev \
        meson \
        ninja-build \
        pkg-config \
    && rm /etc/apt/apt.conf.d/99abi-ca \
    && rm -rf /var/lib/apt/lists/*

RUN useradd --system --user-group --no-create-home --home-dir /tmp abi
USER abi:abi
