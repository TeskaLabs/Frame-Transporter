#!/bin/bash

dd if=/dev/zero count=10240000 bs=1024 | pv -r | nc 127.0.0.1 12345  | pv -r > /dev/null
