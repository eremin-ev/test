#!/bin/bash
for ((i=16; i<$[1024*1024]; i<<=1)); do
	printf "$i\t";
	./test_hex2str $i;
done
