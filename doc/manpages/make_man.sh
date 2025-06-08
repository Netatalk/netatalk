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
    ${transcoder} --from commonmark --to man ${input}
elif [ "${transcoder_name}" = "cmark" ] || [ "${transcoder_name}" = "cmark-gfm" ]; then
    ${transcoder} --smart --to man ${input}
else
    echo "Unknown transcoder: ${transcoder_name}"
    exit 1
fi
