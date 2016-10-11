#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
	rm -f /tmp/sc-test-socks-03.bin /tmp/sc-test-socks-03o.bin /tmp/sc-test-socks-03.chsum
}
trap finish EXIT

echo "Testing SOCKS5 on IPv4"

./socks &
nc -d -l 127.0.0.1 12346 >/tmp/sc-test-socks-03o.bin &
NC_PID=$!

dd if=/dev/urandom count=10240 bs=1024 > /tmp/sc-test-socks-03.bin

cat /tmp/sc-test-socks-03.bin | pv -r | nc -X5 -x 127.0.0.1:12345 127.0.0.1 12346

wait ${NC_PID}

ls -l  /tmp/sc-test-socks-03.bin /tmp/sc-test-socks-03o.bin
shasum /tmp/sc-test-socks-03.bin /tmp/sc-test-socks-03o.bin

cat /tmp/sc-test-socks-03.bin | shasum  > /tmp/sc-test-socks-03.chsum
cat /tmp/sc-test-socks-03o.bin | shasum --check /tmp/sc-test-socks-03.chsum

echo "TEST $0 OK"
