#!/bin/bash -e

# This is a test that introduces a rate limit on the outbound side of reply command
# It tests a proper throttling mechanism

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
	rm -f /tmp/sctext.tmp
}
trap finish EXIT

./reply &
job_pid=$!
sleep 0.2

dd if=/dev/zero count=1024000 bs=1024 | nc 127.0.0.1 12345  | pv -L 10m -rb > /dev/null

kill $job_pid
wait $job_pid

echo "TEST $0 OK"
