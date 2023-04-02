#!/bin/bash
set -e
make build
./create_test_file
./myprogram fileA fileB
gzip -k fileA
gzip -k fileB
gzip -cd fileB.gz | ./myprogram fileC
./myprogram -b 100 fileA fileD
for filename in fileA fileA.gz fileB fileB.gz fileC fileD; do
	echo "${filename} actual size: $(stat -c "%b * 512" $filename | bc) bytes"
done
