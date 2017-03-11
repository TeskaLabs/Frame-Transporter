#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
	rm -f /tmp/sc-test-forward-02.bin /tmp/sc-test-forward-02o.bin /tmp/sc-test-forward-02.chsum
}
trap finish EXIT

dd if=/dev/urandom count=10240 bs=1024 > /tmp/sc-test-forward-02.bin

./forward 127.0.0.1 12346 &
socat tcp-listen:12346,bind=127.0.0.1,reuseaddr,shut-down file:/tmp/sc-test-forward-02.bin &
SO_PID=$!

sleep 1

nc 127.0.0.1 12345 | pv -r  > /tmp/sc-test-forward-02o.bin

wait ${SO_PID}

ls -l  /tmp/sc-test-forward-02.bin /tmp/sc-test-forward-02o.bin
shasum /tmp/sc-test-forward-02.bin /tmp/sc-test-forward-02o.bin

cat /tmp/sc-test-forward-02.bin | shasum  > /tmp/sc-test-forward-02.chsum
cat /tmp/sc-test-forward-02o.bin | shasum --check /tmp/sc-test-forward-02.chsum

echo "TEST $0 OK"
