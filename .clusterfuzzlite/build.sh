#!/bin/bash -eu

cd $SRC/netatalk

# Generate test.conf (creates temp dirs and config file)
cd test/afpd
bash test.sh
cd $SRC/netatalk

# Only pass the fuzzing engine if the library file actually exists.
# The base-builder image sets $LIB_FUZZING_ENGINE but the file is
# only present when ClusterFuzzLite drives the build.
fuzz_engine_opt=""
if [[ -n "${LIB_FUZZING_ENGINE:-}" ]] && [[ -f "$LIB_FUZZING_ENGINE" ]]; then
    fuzz_engine_opt="-Dwith-fuzzing-engine=$LIB_FUZZING_ENGINE"
fi

# Configure with fuzzing enabled and optional features disabled
meson setup build \
    --default-library=static \
    --prefer-static \
    -Dwith-fuzzing=true \
    $fuzz_engine_opt \
    -Dwith-tests=false \
    -Dwith-docs= \
    -Dwith-dtrace=false \
    -Dwith-acls=false \
    -Dwith-cracklib=false \
    -Dwith-cups=false \
    -Dwith-appletalk=false \
    -Dwith-afpstats=false \
    -Dwith-gssapi=false \
    -Dwith-kerberos=false \
    -Dwith-krbV-uam=false \
    -Dwith-ldap=false \
    -Dwith-pam=false \
    -Dwith-quota=false \
    -Dwith-spotlight=false \
    -Dwith-tcp-wrappers=false \
    -Dwith-zeroconf=false \
    -Dwith-init-hooks=false \
    -Dwith-init-style=none \
    -Dwith-install-hooks=false \
    -Dwith-libiconv=false \
    -Dwith-sendfile=false

# Build the fuzz target
ninja -C build test/afpd/afpd_fuzz

# Copy artifacts to $OUT
cp build/test/afpd/afpd_fuzz $OUT/
cp test/afpd/test.conf $OUT/

# Copy seed corpus
zip -j $OUT/afpd_fuzz_seed_corpus.zip test/afpd/fuzz_seed_corpus/*
