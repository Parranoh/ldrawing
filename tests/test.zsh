#!/bin/zsh

cd "$(dirname "$0")/../src"
graph=$(mktemp)
drawing=$(mktemp)

for i in {1..1000}
do
    ./sample-triangulation 1000 >"$graph" 2>../tests/bitstring$i
    ./ldrawing <"$graph" >"$drawing" &&
        cat "$graph" "$drawing" | ./test-planar &&
        rm ../tests/bitstring$i ||
        exit 1
done

rm "$graph" "$drawing"
