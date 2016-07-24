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

# HTTP/1.0 (SSL)
ab -n 4000 -c 8 https://localhost:18443/test

echo "TEST $0 OK"
