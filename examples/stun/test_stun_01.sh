#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
	rm -f /tmp/sc-test-stun-01.bin /tmp/sc-test-stun-01o.bin /tmp/sc-test-stun-01.chsum
}
trap finish EXIT

./stun 127.0.0.1 12346 &
socat openssl-listen:12346,bind=127.0.0.1,reuseaddr,cert=cert.pem,key=key.pem,verify=0,shut-none file:/tmp/sc-test-stun-01o.bin,create &
SOCAT_PID=$!

dd if=/dev/urandom count=10240 bs=1024 > /tmp/sc-test-stun-01.bin

cat /tmp/sc-test-stun-01.bin | pv -r | nc 127.0.0.1 12345

wait ${SOCAT_PID}

ls -l  /tmp/sc-test-stun-01.bin /tmp/sc-test-stun-01o.bin
shasum /tmp/sc-test-stun-01.bin /tmp/sc-test-stun-01o.bin

cat /tmp/sc-test-stun-01.bin | shasum  > /tmp/sc-test-stun-01.chsum
cat /tmp/sc-test-stun-01o.bin | shasum --check /tmp/sc-test-stun-01.chsum

echo "TEST $0 OK"
