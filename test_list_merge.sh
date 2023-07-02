#!/bin/bash

set -u

declare -a input=(
   '4 3
    5 10 15 40
    2 3 20'

   '2 2
    1 1
    2 4'

   '2 5
    1 1
    1 3 45 56 77'
)

declare -a expected=(
    '2 3 5 10 15 20 40'
    '1 1 2 4'
    '1 1 1 3 45 56 77'
)

gen_input()
{
    nr=${#input[@]}
    echo "$nr"
    for (( i=0; i<$nr; i++ )); do
        echo "${input[$i]}" | sed 's/^[ ]\+//g'
    done
}

i=0

make test_list_merge &&
gen_input | ./test_list_merge | while read l; do
    if [[ "$l" != "${expected[$i]}" ]]; then
        echo "Missmatch $i: expected '${expected[$i]}', got '$l'"
        exit 1
    fi
    i=$(( $i + 1 ))
done

if [[ $? -eq 0 ]]; then
    echo "All testcases passed successfully"
fi
