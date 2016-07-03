#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
}
trap finish EXIT

./reply &
sleep 0.2

dd if=/dev/zero count=1024000 bs=1024| nc 127.0.0.1 12345  | pv -r > /dev/null

echo "TEST $0 OK"
