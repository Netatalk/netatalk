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

AFPTESTVOLUMESYS=$(mktemp -d /tmp/AFPtestvolumeSys-XXXXXX)
if [ $? -ne 0 ]; then
    echo Error creating ea=sys AFP test volume >&2
    exit 1
fi

AFPTESTCNIDSYS=$(mktemp -d /tmp/AFPtestCNIDSys-XXXXXX)
if [ $? -ne 0 ]; then
    echo Error creating ea=sys CNID test directory >&2
    exit 1
fi

SIGNATURE=$(LC_ALL=C tr -dc 'A-Za-z0-9' < /dev/urandom | head -c 16)

gen_uuid() {
    hex=$(od -An -tx1 -N16 /dev/urandom | tr -d ' \n')
    printf '%s-%s-%s-%s-%s\n' \
        "$(echo "$hex" | cut -c1-8)" \
        "$(echo "$hex" | cut -c9-12)" \
        "$(echo "$hex" | cut -c13-16)" \
        "$(echo "$hex" | cut -c17-20)" \
        "$(echo "$hex" | cut -c21-32)" | tr 'a-z' 'A-Z'
    return 0
}

VOLUUID=$(gen_uuid)
VOLUUIDSYS=$(gen_uuid)

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
cnid server = 127.0.0.1

[test]
cnid scheme = sqlite
ea = none
path = $AFPTESTVOLUME
vol dbpath = $AFPTESTCNID
volume name = afpd_test
volume uuid = $VOLUUID

[testsys]
cnid scheme = sqlite
ea = sys
path = $AFPTESTVOLUMESYS
vol dbpath = $AFPTESTCNIDSYS
volume name = afpd_test_sys
volume uuid = $VOLUUIDSYS
EOF
echo [ok]
