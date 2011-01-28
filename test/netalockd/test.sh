#!/bin/sh
../../etc/netalockd/netalockd -d -l log_maxdebug -f test.log -p 4702 &
PID=$!
sleep 1
./test
kill $PID
exit $?