#!/bin/sh

make=`which gmake`

if [ -z ${make} ]; then
	make=make
fi

mkdir -p .deps/test

${make} test/logger_test.o; ${make} logger.o; gcc -o test/logger_test logger.o logger_test.o; test/logger_test
