#!/bin/bash

set -u

declare -a input=(
   '4
    10 20 4 30
    4'

   '5
    10 20 4 30 40
    10'
)

declare -a expected=(
    '10 20 30'
    '20 4 30 40'
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

make test_list_delete_node &&
gen_input | ./test_list_delete_node | while read l; do
    if [[ "$l" != "${expected[$i]}" ]]; then
        echo "Missmatch $i: expected '${expected[$i]}', got '$l'"
        exit 1
    fi
    i=$(( $i + 1 ))
done

if [[ $? -eq 0 ]]; then
    echo "All testcases passed successfully"
fi
