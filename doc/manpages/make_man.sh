#!/bin/sh

if [ "$#" -ne 5 ]; then
    echo "Usage: $0 <transcoder> <file> <manpage> <section> <version>"
    exit 1
fi

transcoder="$1"
input="$2"
man="$3"
sec="$4"
ver="$5"

transcoder_name=$(basename "${transcoder}")

echo ".TH \"${man}\" \"${sec}\" \"\" \"Netatalk ${ver}\" \"Netatalk AFP Fileserver Manual\""

if [ "${transcoder_name}" = "pandoc" ]; then
    ${transcoder} --from commonmark --to man "${input}"
elif [ "${transcoder_name}" = "cmark" ] || [ "${transcoder_name}" = "cmark-gfm" ]; then
    # cleaning up cmark's non-portable code block style
    tmpdir_base="${TMPDIR:-/tmp}/netatalk-make-man.$$"
    tmpdir="${tmpdir_base}"
    counter=0
    while ! (umask 077 && mkdir "${tmpdir}") 2> /dev/null; do
        counter=$((counter + 1))
        if [ "${counter}" -ge 100 ]; then
            echo "Unable to create a temporary directory in ${TMPDIR:-/tmp}" >&2
            exit 1
        fi
        tmpdir="${tmpdir_base}.${counter}"
    done
    tmpfile="${tmpdir}/output"
    trap 'rm -f "${tmpfile}"; rmdir "${tmpdir}"' 0
    "${transcoder}" --smart --to man "${input}" > "${tmpfile}" || exit $?
    sed 's/\\f\[C\]/\\f[CR]/g' "${tmpfile}"
else
    echo "Unknown transcoder: ${transcoder_name}"
    exit 1
fi
