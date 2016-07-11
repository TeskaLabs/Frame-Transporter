#!/bin/bash -e

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
sleep 0.2

dd if=/dev/zero count=1024000 bs=2024 | socat stdio tcp:127.0.0.1:12345  | pv -rb > /dev/null

echo "TEST $0 OK"
