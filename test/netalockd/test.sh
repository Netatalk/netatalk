#!/bin/sh
../../etc/netalockd/netalockd -d -p 4702 &
PID=$!
sleep 1
./test
kill $PID
exit $?