#!/bin/zsh

cd "$(dirname "$0")/../src"
tmp=$(mktemp)

echo 'n t_io t_decompose t_rectangular_dual t_port_assignment'
zeros=0
for num_zeros in {1..8}
do
    for i in {1..9}
    do
        n="${i}${zeros}"
        echo -n "$n "
        ./sample-triangulation "$n" >"$tmp" 2>/dev/null
        ./ldrawing --time <"$tmp" 2>&1 >/dev/null
    done
    zeros="0$zeros"
done

rm "$tmp"
