#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	[[ !  -z  ${JOBS}  ]] && ( echo ${JOBS} | xargs -n1 kill )
	rm -f /tmp/sc-test-reply-03.bin /tmp/sc-test-reply-03.chsum
}
trap finish EXIT

./reply &
TASK_PID=$!

dd if=/dev/urandom count=1024 bs=1024 > /tmp/sc-test-reply-03.bin
sleep 0.2

cat /tmp/sc-test-reply-03.bin | pv -r | nc -U /tmp/sctext.tmp | pv -r | shasum > /tmp/sc-test-reply-03.chsum

kill ${TASK_PID}
wait

cat /tmp/sc-test-reply-03.bin | shasum --check /tmp/sc-test-reply-03.chsum

echo "TEST $0 OK"
