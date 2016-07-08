#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
	#rm -f /tmp/sc-test-sreply-01.bin /tmp/sc-test-sreply-01.chsum /tmp/sc-test-sreply-01o.bin
}
trap finish EXIT

./sreply &
sleep 0.2

dd if=/dev/urandom count=1024 bs=1024 > /tmp/sc-test-sreply-01.bin
cat /tmp/sc-test-sreply-01.bin | pv -rb | socat stdio openssl:127.0.0.1:12345,verify=0,shut-down,linger=1 | pv -rb > /tmp/sc-test-sreply-01o.bin
cat /tmp/sc-test-sreply-01o.bin | shasum > /tmp/sc-test-sreply-01.chsum
cat /tmp/sc-test-sreply-01.bin | shasum --check /tmp/sc-test-sreply-01.chsum

echo "TEST $0 OK"
