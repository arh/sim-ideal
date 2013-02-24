#!/bin/bash
set -e

for inFile in $@
do
	echo -e " $inFile : `awk 'BEGIN { i=0 } ; {i = i + $2 } ; END { print i}'  $inFile` "
done
