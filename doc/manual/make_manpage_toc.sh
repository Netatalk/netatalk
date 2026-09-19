#!/bin/sh

# Generate one manpage navigation group for the website sidebar or index.
set -eu

mode=$1
shift

for manpage in "$@"; do
    name=$(awk '
        $0 == "# Name" {
            while (getline && $0 == "") {
            }
            print
            exit
        }
    ' "$manpage")
    if [ -z "$name" ]; then
        printf '%s: missing Name section\n' "$manpage" >&2
        exit 1
    fi

    filename=${manpage##*/}
    filename=${filename%.md}
    label=${filename%.[158]}

    if [ "$mode" = toc ]; then
        label=$name
    fi

    printf '* [%s](%s.html)\n' "$label" "$filename"
done
