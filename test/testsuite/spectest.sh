#!/bin/sh

ret=0

check_return() {
    ERRNO=$?
    if test $ERRNO -eq 0 -o $ERRNO -eq 3 ; then
        echo "[OK]"
    else
        echo "[error: $ERRNO]"
        ret=1
    fi
}

echo =====================================

if [ ! -f ./test/testsuite/spectest.conf ] ; then
    cat > ./test/testsuite/spectest.conf <<EOF
AFPSERVER=127.0.0.1
AFPPORT=548
USER1=
USER2=
PASSWD=

# *** volumes configured with appledouble = ea
VOLUME1=
VOLUME2=

# *** set this to the local path of VOLUME1 if you want to run T2_spectest
# *** requires that afpd and the testsuite are running on the same machine
# LOCALVOL1PATH=

# AFPVERSION: 1 = AFP 2.1, 2 = AFP 2.2, 3 = AFP 3.0, 4 = AFP 3.1, 5 = AFP 3.2, 6 = AFP 3.3, 7 = AFP 3.4
AFPVERSION=7
EOF
    echo "Tests cannot be run without a ./test/testsuite/spectest.conf file."
    echo "A template configuration file spectest.conf has been generated."
    echo "Adjust it to match the AFP server under test and run this script again."
    echo "====================================="
    exit 0
fi

. ./test/testsuite/spectest.conf

# cleanup
if test ! -z "$LOCALVOL1PATH" ; then
    rm -rf "$LOCALVOL1PATH"/t*
fi

if [ -z "$USER1" -o -z "$USER2" ] ; then
    echo "Need two users to run this test."
    exit 1
fi

if [ -z "$VOLUME1" -o -z "$VOLUME2" ] ; then
    echo "Need two volumes to run this test."
    exit 1
fi

rm -f ./test/testsuite/spectest.log

##
echo "Running spectest with two users..."
./test/testsuite/spectest -"$AFPVERSION" -x -h "$AFPSERVER" -p "$AFPPORT" -u "$USER1" -d "$USER2" -w "$PASSWD" -s "$VOLUME1" -S "$VOLUME2" >> ./test/testsuite/spectest.log 2>&1
check_return

##
if test ! -z "$LOCALVOL1PATH" ; then
    echo "Running spectest with local filesystem modifications..."
    ./test/testsuite/T2_spectest -"$AFPVERSION" -x -h "$AFPSERVER" -p "$AFPPORT" -u "$USER1" -d "$USER2" -w "$PASSWD" -s "$VOLUME1" -S "$VOLUME2" -c "$LOCALVOL1PATH" >> ./test/testsuite/spectest.log 2>&1
    check_return
else
    echo "Skipping spectest with local filesystem modifications..."
fi

echo "====================================="
echo "Test summary"
echo "------------"
echo "Passed tests:"
grep "PASS" ./test/testsuite/spectest.log | wc -l
echo "Failed tests:"
grep "FAIL" ./test/testsuite/spectest.log | wc -l
echo "Skipped tests:"
egrep "NOT TESTED|SKIPPED" ./test/testsuite/spectest.log | wc -l
echo "====================================="

echo "Failed tests"
echo "------------"
grep "FAIL" ./test/testsuite/spectest.log | sort -n | uniq
echo "====================================="

echo "Skipped tests"
echo "------------"
egrep "NOT TESTED|SKIPPED" ./test/testsuite/spectest.log | sort -n | uniq
echo "====================================="

echo "Successful tests"
echo "------------"
grep "PASSED" ./test/testsuite/spectest.log | sort -n | uniq
echo "====================================="

# cleanup
if test ! -z "$LOCALVOL1PATH" ; then
    rm -rf "$LOCALVOL1PATH"/t*
fi

exit $ret
