#!/bin/bash -e

function finish
{
	JOBS=$(jobs -rp)
	if [[ !  -z  ${JOBS}  ]] ; then
		echo ${JOBS} | xargs kill ;
	fi
	rm -f /tmp/ft_proto_socks.bin /tmp/sc-test-socks-02o.bin /tmp/sc-test-socks-02.chsum
}
trap finish EXIT

echo "Testing SOCKS4A"

./socks &
nc -d -l localhost 12346 >/tmp/sc-test-socks-02o.bin &
NC_PID=$!

dd if=/dev/urandom count=10240 bs=1024 > /tmp/sc-test-socks-02.bin

cat /tmp/sc-test-socks-02.bin | pv -r | socat stdin SOCKS4A:127.0.0.1:localhost:12346,socksport=12345,socksuser=ABCDEFZ

wait ${NC_PID}

ls -l  /tmp/sc-test-socks-02.bin /tmp/sc-test-socks-02o.bin
shasum /tmp/sc-test-socks-02.bin /tmp/sc-test-socks-02o.bin

cat /tmp/sc-test-socks-02.bin | shasum  > /tmp/sc-test-socks-02.chsum
cat /tmp/sc-test-socks-02o.bin | shasum --check /tmp/sc-test-socks-02.chsum

echo "TEST $0 OK"
