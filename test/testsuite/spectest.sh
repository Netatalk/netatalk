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

if [ ! -f spectest.conf ] ; then
    cat > spectest.conf <<EOF
# AFPSERVER=127.0.0.1
# AFPPORT=548
# USER1=
# USER2=
# PASSWD=
#
# *** volumes with adouble:ea
# VOLUME1=
# LOCALVOL1PATH=
# VOLUME2=

# AFPVERSION: 1 = AFP 2.1, 2 = AFP 2.2, 3 = AFP 3.0, 4 = AFP 3.1, 5 = AFP 3.2, 6 = AFP 3.3, 7 = AFP 3.4
# AFPVERSION=5
EOF
    echo 'A template configuration file "spectest.conf" has been generated.'
    echo Adjust it to match your setup.
    echo =====================================
    exit 0
fi

. ./spectest.conf

# cleanup
if test ! -z "$LOCALVOL1PATH" ; then
    rm -rf "$LOCALVOL1PATH"/t*
fi

if [ -z "$VOLUME1" -o -z "$VOLUME2" -o -z "$LOCALVOL1PATH" ] ; then
    echo Need two volumes and the local path to volume1
    exit 1
fi

rm -f spectest.log

##
printf "Running spectest with two users..."
./spectest -"$AFPVERSION" -h "$AFPSERVER" -p "$AFPPORT" -u "$USER1" -d "$USER2" -w "$PASSWD" -s "$VOLUME1"  -S "$VOLUME2" >> spectest.log 2>&1
check_return

##
printf "Running spectest with local filesystem modifications..."
./T2_spectest -"$AFPVERSION" -h "$AFPSERVER" -p "$AFPPORT" -u "$USER1" -d "$USER2" -w "$PASSWD" -s "$VOLUME1" -S "$VOLUME2" -c "$LOCALVOL1PATH" >> spectest.log 2>&1
check_return

echo =====================================
echo Failed tests
echo ––––––––––––
grep "summary.*FAIL" spectest.log | sed s/test//g | sort -n | uniq
echo =====================================

echo Skipped tests
echo –––––––––––––
egrep 'summary.*NOT TESTED|summary.*SKIPPED' spectest.log | sed s/test//g | sort -n | uniq
echo =====================================

echo Successfull tests
echo –––––––––––––––––
grep "summary.*PASSED" spectest.log | sed s/test//g | sort -n | uniq
echo =====================================

# cleanup
if test ! -z "$LOCALVOL1PATH" ; then
    rm -rf "$LOCALVOL1PATH"/t*
fi

exit $ret
