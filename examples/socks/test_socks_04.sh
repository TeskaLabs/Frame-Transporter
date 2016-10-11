#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
	rm -f /tmp/sc-test-socks-04.bin /tmp/sc-test-socks-04o.bin /tmp/sc-test-socks-04.chsum
}
trap finish EXIT

echo "Testing SOCKS5 on IPv6"

./socks &
nc -d -l ::1 12347 >/tmp/sc-test-socks-04o.bin &
NC_PID=$!

dd if=/dev/urandom count=10240 bs=1024 > /tmp/sc-test-socks-04.bin

cat /tmp/sc-test-socks-04.bin | pv -r | nc -X5 -x 127.0.0.1:12345 ::1 12347

wait ${NC_PID}

ls -l  /tmp/sc-test-socks-04.bin /tmp/sc-test-socks-04o.bin
shasum /tmp/sc-test-socks-04.bin /tmp/sc-test-socks-04o.bin

cat /tmp/sc-test-socks-04.bin | shasum  > /tmp/sc-test-socks-04.chsum
cat /tmp/sc-test-socks-04o.bin | shasum --check /tmp/sc-test-socks-04.chsum

echo "TEST $0 OK"
