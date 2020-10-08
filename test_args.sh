#!/bin/bash

echo "ARGC $#"

while getopts ":s:p:" o; do
case "${o}" in
	s)
		s=${OPTARG}
		;;
	p)
		p=${OPTARG}
		;;
	*)
		echo usage
		;;
	esac
done
shift $((OPTIND - 1))

echo "ARGC $#"

if [ -z "${s}" ] || [ -z "${p}" ]; then
    echo usage
fi

echo "s = ${s}"
echo "p = ${p}"

for pos; do
	printf "argv[$i] '$pos'\n"
done
