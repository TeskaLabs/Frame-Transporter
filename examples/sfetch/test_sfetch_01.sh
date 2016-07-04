#!/bin/bash -e

while read p; do
	echo "Testing", ${p}
	read -ra arr <<< "${p}"
	./sfetch ${arr[0]} ${arr[1]}
done < ssl_sites.txt

echo "TEST $0 OK"
