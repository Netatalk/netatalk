FROM gcr.io/oss-fuzz-base/base-builder:v1@sha256:08bb27ab1f273341ef91e68713c27e609c0c085b596757ee64d07aaff494b97c

COPY .clusterfuzzlite/requirements-python.txt /tmp/requirements-python.txt

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
&&  pip install --no-deps --require-hashes -r /tmp/requirements-python.txt \
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
