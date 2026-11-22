#!/bin/sh
#
# (c) 2026 Daniel Markstedt <daniel@mindani.net>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.

set -eu

export LC_ALL=C
abi_path=libatalk/abi
ledger=$abi_path/libatalk-linux-x86_64.tsv

fail() {
    printf '%s\n' "$*" >&2
    exit 1
}

run_container() {
    [ "$#" -ge 1 ] && [ "$#" -le 3 ] \
        || fail "Usage: $0 {check BASE | fingerprint [REVISION]} [OUTPUT_DIRECTORY]"
    mode=$1
    case "$mode" in
        check)
            ref=${2:-${LIBATALK_ABI_BASE:-}}
            [ -n "$ref" ] || fail "Pass a base Git revision or set LIBATALK_ABI_BASE."
            ;;
        fingerprint) ref=${2:-HEAD} ;;
        *) fail "Unknown ABI command '$mode'." ;;
    esac

    source=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
    git_dir=$(git -C "$source" rev-parse --absolute-git-dir)
    git_common_dir=$(git -C "$source" rev-parse --git-common-dir)
    case "$git_common_dir" in
        /*) ;;
        *) git_common_dir=$source/$git_common_dir ;;
    esac
    ref=$(git -C "$source" rev-parse --verify --end-of-options "$ref^{commit}")

    output=${3:-${MESON_BUILD_ROOT:-$source/build}/abi}
    mkdir -p "$output"
    output=$(cd "$output" && pwd -P)
    printf 'Writing libatalk ABI results to %s\n' "$output"

    image=$(docker build --platform linux/amd64 --quiet "$source/$abi_path")
    exec docker run --rm --platform linux/amd64 --user "$(id -u):$(id -g)" \
        --mount "type=bind,src=$source,dst=/repo,readonly" \
        --mount "type=bind,src=$git_dir,dst=/git-dir,readonly" \
        --mount "type=bind,src=$git_common_dir,dst=/git-common,readonly" \
        --mount "type=bind,src=$output,dst=/output" \
        --env GIT_DIR=/git-dir --env GIT_COMMON_DIR=/git-common --env GIT_WORK_TREE=/repo \
        --workdir /repo "$image" \
        "$abi_path/libatalk_abi.sh" --in-container "$mode" "$ref" /output
}

if [ "${1:-}" != --in-container ]; then
    run_container "$@"
fi
shift

fingerprint() {
    revision=$1
    name=$2
    tree=$scratch/$name
    mkdir -p "$tree/src"
    git archive --format=tar --output="$tree/source.tar" "$revision"
    python3 -m tarfile -e "$tree/source.tar" "$tree/src"

    CC=gcc CFLAGS="-ffile-prefix-map=$tree=/netatalk" meson setup \
        --buildtype=debug -Dstrip=false -Db_lto=false -Ddefault_library=shared \
        -Dwith-acls=true -Dwith-appletalk=true -Dwith-cnid-backends=mysql \
        -Dwith-docs=[] -Dwith-dtrace=false -Dwith-eventloop=libevent \
        -Dwith-init-hooks=false -Dwith-install-hooks=false -Dwith-ldap=true \
        -Dwith-spotlight=true -Dwith-spotlight-backends=cnid -Dwith-subprojects=true \
        "$tree/build" "$tree/src" >&2
    for feature in HAVE_POSIX_ACLS HAVE_LDAP CNID_BACKEND_MYSQL SPOTLIGHT_BACKEND_CNID; do
        grep -qx "#define $feature 1" "$tree/build/config.h" \
            || fail "The ABI build is missing $feature."
    done
    meson compile -C "$tree/build" atalk >&2

    library=$tree/build/libatalk/libatalk.so
    readelf -d "$library" > "$tree/dynamic.txt"
    soname=$(sed -n 's/.*(SONAME).*\[\(.*\)\].*/\1/p' "$tree/dynamic.txt")
    case "$soname" in
        libatalk.so.*) ;;
        *) fail "Expected a versioned libatalk SONAME, got '$soname'." ;;
    esac
    readelf -WS "$library" > "$tree/sections.txt"
    grep -Eq '[[:blank:]]\.debug_info[[:blank:]]' "$tree/sections.txt" \
        || fail "The libatalk build has no DWARF debug information."
    abidw --no-corpus-path --no-comp-dir-path --no-show-locs --no-parameter-names \
        --no-elf-needed --drop-undefined-syms --exported-interfaces-only --type-id-style hash \
        --out-file "$tree/libatalk.abixml" "$library"
    grep -q '<abi-instr ' "$tree/libatalk.abixml" \
        || fail "abidw did not produce type information for libatalk."
    # abidw still emits separate ELF symbol tables for imports.
    python3 - "$tree/libatalk.abixml" "$scratch/$name.abixml" << 'PY'
import sys
from xml.etree.ElementTree import canonicalize

with open(sys.argv[2], "w", encoding="utf-8") as output:
    canonicalize(
        from_file=sys.argv[1],
        out=output,
        strip_text=True,
        exclude_tags={"undefined-elf-function-symbols", "undefined-elf-variable-symbols"},
    )
PY
    digest=$(sha256sum "$scratch/$name.abixml")
    printf '%s\t%s\t%s\n' "$ABI_PROFILE" "$soname" "${digest%% *}" > "$scratch/$name.tsv"
    cp "$scratch/$name.tsv" "$output/$name.tsv"
}

compare_abi() {
    diff_status=0
    abidiff --no-default-suppression "$@" "$scratch/baseline.abixml" \
        "$scratch/candidate.abixml" > "$scratch/$report.diff" 2>&1 || diff_status=$?
    cp "$scratch/$report.diff" "$output/$report.diff"
    cat "$scratch/$report.diff"
    case "$diff_status" in
        0 | 4 | 12) ;;
        *) fail "abidiff failed with status $diff_status." ;;
    esac
}

[ "$#" -eq 3 ] || fail "Usage: $0 {fingerprint REVISION | check BASE} OUTPUT_DIRECTORY"
: "${ABI_PROFILE:?Run this script in the pinned libatalk ABI container.}"
# Bind mounts can report a different owner inside the container.
export GIT_CONFIG_COUNT=1 GIT_CONFIG_KEY_0=safe.directory GIT_CONFIG_VALUE_0="$PWD"
mode=$1
ref=$(git rev-parse --verify "$2^{commit}")
mkdir -p "$3"
output=$(cd "$3" && pwd)
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
export MESON_PACKAGE_CACHE_DIR=$scratch/packages

case "$mode" in
    fingerprint)
        fingerprint "$ref" candidate
        cat "$scratch/candidate.tsv"
        exit 0
        ;;
    check) ;;
    *) fail "Unknown ABI command '$mode'." ;;
esac

git show "HEAD:$ledger" > "$scratch/proposed.tsv"
fingerprint HEAD candidate

if git cat-file -e "$ref:$ledger" 2> /dev/null; then
    git show "$ref:$ledger" > "$scratch/expected.tsv"
    if cmp -s "$scratch/expected.tsv" "$scratch/candidate.tsv"; then
        cmp -s "$scratch/proposed.tsv" "$scratch/candidate.tsv" \
            || fail "The proposed ledger entry differs from the verified candidate."
        printf '%s\n' 'libatalk matches the active ABI ledger entry.'
        exit 0
    fi
    # The commit that last updated the entry contains the source it describes.
    baseline=$(git log --first-parent -1 --format=%H "$ref" -- "$ledger")
else
    # Releases published before the ledger was introduced use their source as the baseline.
    baseline=$ref
fi

printf 'Rebuilding the libatalk ABI baseline from %s\n' "$baseline"
fingerprint "$baseline" baseline
if [ ! -f "$scratch/expected.tsv" ]; then
    cp "$scratch/baseline.tsv" "$scratch/expected.tsv"
fi

expected_profile=$(cut -f 1 "$scratch/expected.tsv")
expected_soname=$(cut -f 2 "$scratch/expected.tsv")
candidate_soname=$(cut -f 2 "$scratch/candidate.tsv")
if [ "$expected_profile" = "$ABI_PROFILE" ]; then
    cmp -s "$scratch/expected.tsv" "$scratch/baseline.tsv" \
        || fail "The rebuilt baseline does not match the active ABI ledger entry."
else
    if git diff --quiet "$ref" HEAD -- "$abi_path/Dockerfile"; then
        fail "The toolchain profile differs from the active ledger without a Dockerfile update."
    fi
    [ "$(cut -f 2 "$scratch/baseline.tsv")" = "$expected_soname" ] \
        || fail "The baseline SONAME changed while rebuilding with the new profile."
    printf '%s\n' 'Comparing both revisions with the new ABI toolchain profile.'
fi

report=abi
compare_abi
if [ "$candidate_soname" = "$expected_soname" ]; then
    if [ "$diff_status" -ne 0 ]; then
        report=abi-existing-interfaces
        compare_abi --no-added-syms
        [ "$diff_status" -eq 0 ] \
            || fail "Existing libatalk interfaces changed under $candidate_soname. Restore compatibility or bump the Meson SONAME."
    fi
else
    [ "${candidate_soname##*.}" -gt "${expected_soname##*.}" ] \
        || fail "The libatalk SONAME must increase ($expected_soname -> $candidate_soname)."
    printf 'Review the libatalk ABI report for the SONAME bump %s -> %s.\n' \
        "$expected_soname" "$candidate_soname"
fi

cmp -s "$scratch/proposed.tsv" "$scratch/candidate.tsv" \
    || fail "Update $ledger with candidate.tsv from the ABI output directory."
printf '%s\n' 'libatalk ABI and the proposed ledger entry verified.'
