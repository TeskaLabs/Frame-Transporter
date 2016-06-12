#!/bin/bash

dd if=/dev/zero count=1024 bs=1024 > /tmp/sc-test-reply-01.bin
md5 /tmp/sc-test-reply-01.bin

cat /tmp/sc-test-reply-01.bin | nc -D -U /tmp/sctext.tmp | md5
