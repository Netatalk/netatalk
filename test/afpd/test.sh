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

SIGNATURE=$(LC_ALL=C tr -dc 'A-Za-z0-9' < /dev/urandom | head -c 16)
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
cnid scheme = sqlite
ea = none
path = $AFPTESTVOLUME
vol dbpath = $AFPTESTCNID
volume name = afpd_test
volume uuid = $VOLUUID
EOF
echo [ok]
