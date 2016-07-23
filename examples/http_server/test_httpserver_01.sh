#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
	rm -f /tmp/sc-test-reply-01.bin /tmp/sc-test-reply-01.chsum /tmp/sctext.tmp
}
trap finish EXIT

./httpserver &
sleep 0.2

ab -k -n 50000 -c 8 http://localhost:18080/test

echo "TEST $0 OK"
