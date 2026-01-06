#!/bin/sh

AFPTESTVOLUME=$(mktemp -d /tmp/AFPtestvolume-XXXXXX)
if [ $? -ne 0 ]; then
    echo Error creating AFP test volume >&2
    exit 1
fi

AFPTESTCNID=$(mktemp -d /tmp/AFPtestCNID-XXXXXX)
if [ $? -ne 0 ]; then
    echo Error creating CNID test directory >&2
    exit 1
fi

SIGNATURE=$(od -An -N16 -tx1 /dev/urandom | tr -d ' \n' | tr 'a-z' 'A-Z')
VOLUUID=$(uuidgen | tr 'a-z' 'A-Z')

if [ -f test.conf ]; then
    echo "Removing stale configuration template test.conf ..."
    rm -f test.conf
fi

echo -n "Creating configuration template test.conf ..."
cat > test.conf << EOF
[Global]
afp port = 10548
log file = $(pwd)/meson-logs/afpd.log
log level = default:debug
signature = $SIGNATURE

[test]
path = $AFPTESTVOLUME
cnid scheme = sqlite
ea = none
vol dbpath = $AFPTESTCNID
volume name = afpd_test
volume uuid = $VOLUUID
EOF
echo [ok]
