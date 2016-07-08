#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
}
trap finish EXIT

./sreply &
sleep 0.2

dd if=/dev/zero count=1024000 bs=1024 | socat stdio openssl:127.0.0.1:12345,verify=0  | pv -rb > /dev/null

echo "TEST $0 OK"
