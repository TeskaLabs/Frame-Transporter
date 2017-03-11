#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
	rm -f /tmp/sc-test-stun-02.bin /tmp/sc-test-stun-02o.bin /tmp/sc-test-stun-02.chsum
}
trap finish EXIT

dd if=/dev/urandom count=10240 bs=1024 > /tmp/sc-test-stun-02.bin

./stun 127.0.0.1 12346 &
socat openssl-listen:12346,bind=127.0.0.1,reuseaddr,cert=cert.pem,key=key.pem,verify=0,shut-down file:/tmp/sc-test-stun-02.bin &
SOCAT_PID=$!

sleep 1

nc 127.0.0.1 12345 | pv -r > /tmp/sc-test-stun-02o.bin

wait ${SOCAT_PID}

ls -l  /tmp/sc-test-stun-02.bin /tmp/sc-test-stun-02o.bin
shasum /tmp/sc-test-stun-02.bin /tmp/sc-test-stun-02o.bin

cat /tmp/sc-test-stun-02.bin | shasum  > /tmp/sc-test-stun-02.chsum
cat /tmp/sc-test-stun-02o.bin | shasum --check /tmp/sc-test-stun-02.chsum

echo "TEST $0 OK"
