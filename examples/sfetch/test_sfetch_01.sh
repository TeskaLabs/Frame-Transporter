#!/bin/bash

COUNTER_OK=0
COUNTER_FAILED=0

while read p; do
	read -ra arr <<< "${p}"
	./sfetch ${arr[0]} ${arr[1]} > /dev/null
	if [ $? -ne 0 ]; then
		COUNTER_FAILED=$[$COUNTER_FAILED +1]
		echo ${p} "FAILED"
	else
		COUNTER_OK=$[$COUNTER_OK +1]
		echo ${p} "OK"
	fi
done < ssl_sites.txt



echo "TEST $0 OK $COUNTER_OK $COUNTER_FAILED"
