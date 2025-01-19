#!/bin/sh

if [ "$#" -ne 5 ]; then
  echo "Usage: $0 <cmark> <file> <manpage> <section> <version>"
  exit 1
fi

cmark="$1"
input="$2"
man="$3"
sec="$4"
ver="$5"

echo ".TH \"${man}\" \"${sec}\" \"\" \"Netatalk ${ver}\" \"Netatalk AFP Fileserver Manual\""

${cmark} --smart --to man ${input}
