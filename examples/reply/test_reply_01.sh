#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
	rm -f /tmp/sc-test-reply-01.bin /tmp/sc-test-reply-01.chsum
}
trap finish EXIT

./reply &
sleep 0.2

dd if=/dev/urandom count=1024 bs=1024 > /tmp/sc-test-reply-01.bin
cat /tmp/sc-test-reply-01.bin | pv -r | nc 127.0.0.1 12345 | pv -r | shasum > /tmp/sc-test-reply-01.chsum
cat /tmp/sc-test-reply-01.bin | shasum --check /tmp/sc-test-reply-01.chsum

echo "TEST $0 OK"
