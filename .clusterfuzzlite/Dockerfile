FROM gcr.io/oss-fuzz-base/base-builder:v1@sha256:4c20153f428f2ddf623a725251c4b85f71eb19cf3718a33fa41f299a2c3a5226

RUN apt-get update \
&&  apt-get install --yes --no-install-recommends \
    libevent-dev \
    libgcrypt20-dev \
    libiniparser-dev \
    libsqlite3-dev \
    ninja-build \
    pkg-config \
    python3-pip \
    uuid-runtime \
    zlib1g-dev \
&&  pip3 install --no-cache-dir meson \
&&  rm -rf /var/lib/apt/lists/*

COPY .clusterfuzzlite/build.sh $SRC/build.sh
COPY bin/ $SRC/netatalk/bin/
COPY config/ $SRC/netatalk/config/
COPY contrib/meson.build $SRC/netatalk/contrib/
COPY contrib/bin_utils/ $SRC/netatalk/contrib/bin_utils/
COPY etc/ $SRC/netatalk/etc/
COPY include/ $SRC/netatalk/include/
COPY libatalk/ $SRC/netatalk/libatalk/
COPY subprojects/ $SRC/netatalk/subprojects/
COPY sys/ $SRC/netatalk/sys/
COPY test/ $SRC/netatalk/test/
COPY meson_config.h $SRC/netatalk/
COPY meson_options.txt $SRC/netatalk/
COPY meson.build $SRC/netatalk/
WORKDIR $SRC/netatalk
