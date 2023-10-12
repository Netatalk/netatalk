#!/bin/sh
if [ ! -d /tmp/netatalk/AFPtestvolume ] ; then
    mkdir -p /tmp/netatalk/AFPtestvolume
    if [ $? -ne 0 ] ; then
        echo Error creating AFP test volume /tmp/netatalk/AFPtestvolume
        exit 1
    fi
fi

if [ ! -f test.conf ] ; then
    echo -n "Creating configuration template ... "
    cat > test.conf <<EOF
test -noddp -port 10548
EOF
    echo [ok]
fi

if [ ! -f test.default ] ; then
    echo -n "Creating volume config template ... "
    cat > test.default <<EOF
/tmp/netatalk/AFPtestvolume "test" ea:none cnidscheme:last
EOF
    echo [ok]
fi
