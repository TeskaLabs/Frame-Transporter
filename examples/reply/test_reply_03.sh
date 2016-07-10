#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
	rm -f /tmp/sc-test-reply-03.bin /tmp/sc-test-reply-03.chsum /tmp/sctext.tmp

}
trap finish EXIT

./reply &

dd if=/dev/urandom count=1024 bs=1024 > /tmp/sc-test-reply-03.bin
sleep 0.2

cat /tmp/sc-test-reply-03.bin | pv -r | nc -U /tmp/sctext.tmp | pv -r | shasum > /tmp/sc-test-reply-03.chsum

cat /tmp/sc-test-reply-03.bin | shasum --check /tmp/sc-test-reply-03.chsum

echo "TEST $0 OK"
