FROM gcr.io/oss-fuzz-base/base-builder@sha256:34dffbed717ad4bdc62286ff4c7a9d4a9363efc106c0c50f2154cac92c1a510b

RUN apt-get update \
&&  apt-get install --yes --no-install-recommends \
    libevent-dev \
    libgcrypt20-dev \
    libiniparser-dev \
    libsqlite3-dev \
    meson \
    ninja-build \
    pkg-config \
    uuid-runtime \
    zlib1g-dev \
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
