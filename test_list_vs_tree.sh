#!/bin/bash

#
# Usage:
#
#	$ ./test_list_vs_tree.sh > /tmp/1
#	gnuplot> set key top left
#	gnuplot> plot '/tmp/1' u 6 t 'tree seach', '' u 7 t 'list search'
#

function mk_gnuplot()
{
	printf "set output '/tmp/o.png'\n"
	printf "set term pngcairo size 800,600\n"
	printf "set key top left\n"
	printf "set xlabel 'N'\n"
	printf "set ylabel 'Time, s'\n"
	printf "plot '/tmp/1.txt' u 6 t 'tree seach', '' u 7 t 'list search'\n"
}

#
# Warm up to make CPUs scale frequency to max performance
#
for ((v=1; v<10; v++)); do
	./test_list_vs_tree -v 1000 -k 1000;
done > /dev/null

#
# Real sampling
#
for ((v=1; v<200; v++)); do
	./test_list_vs_tree -v $v -k 1000;
done > /tmp/1.txt

mk_gnuplot | gnuplot
