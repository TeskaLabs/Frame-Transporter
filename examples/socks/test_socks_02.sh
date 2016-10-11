#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
	rm -f /tmp/sc-test-socks-01.bin /tmp/sc-test-socks-01o.bin /tmp/sc-test-socks-01.chsum
}
trap finish EXIT

echo "Testing SOCKS4A"

./socks &
nc -d -l localhost 12346 >/tmp/sc-test-socks-01o.bin &
NC_PID=$!

dd if=/dev/urandom count=10240 bs=1024 > /tmp/sc-test-socks-01.bin

cat /tmp/sc-test-socks-01.bin | pv -r | socat stdin SOCKS4A:127.0.0.1:localhost:12346,socksport=12345,socksuser=ABCDEFZ

wait ${NC_PID}

ls -l  /tmp/sc-test-socks-01.bin /tmp/sc-test-socks-01o.bin
shasum /tmp/sc-test-socks-01.bin /tmp/sc-test-socks-01o.bin

cat /tmp/sc-test-socks-01.bin | shasum  > /tmp/sc-test-socks-01.chsum
cat /tmp/sc-test-socks-01o.bin | shasum --check /tmp/sc-test-socks-01.chsum

echo "TEST $0 OK"
