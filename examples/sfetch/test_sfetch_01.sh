#!/bin/bash -e

while read p; do
	read -ra arr <<< "${p}"
	./sfetch ${arr[0]} ${arr[1]} > /dev/null
	if [ $? -ne 0 ]; then
		echo ${p} "FAILED"
	else
		echo ${p} "OK"
	fi
done < ssl_sites.txt

echo "TEST $0 OK"
