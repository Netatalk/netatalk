FROM gcr.io/oss-fuzz-base/base-builder:v1@sha256:dbc19274e11b37f844f9093ddcb805d98301365d65768a64378f01c74cfd1e67

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
    python3-venv \
    uuid-runtime \
    zlib1g-dev \
&&  python3 -m pip install --no-cache-dir --upgrade pip pipenv \
&&  mkdir -p /opt/pydeps \
&&  cd /opt/pydeps \
&&  PIPENV_VENV_IN_PROJECT=1 pipenv --python 3 \
&&  PIPENV_VENV_IN_PROJECT=1 pipenv run pip install --no-deps --require-hashes -r /tmp/requirements-python.txt \
&&  rm -rf /var/lib/apt/lists/*

ENV PATH="/opt/pydeps/.venv/bin:${PATH}"

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
