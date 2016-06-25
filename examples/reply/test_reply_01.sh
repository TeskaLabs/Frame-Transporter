#!/bin/bash

dd if=/dev/urandom count=1024 bs=1024 > /tmp/sc-test-reply-01.bin
md5 /tmp/sc-test-reply-01.bin

cat /tmp/sc-test-reply-01.bin | pv -r | nc -U /tmp/sctext.tmp | pv -r | md5
#cat /tmp/sc-test-reply-01.bin | pv -r | nc 127.0.0.1 12345  | pv -r | md5
