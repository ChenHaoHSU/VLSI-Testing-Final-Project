#!/bin/bash

EVAL=./eval/golden_tdfsim

CASES="432 499 880 1355 2670 3540 6288 7552"


for i in $CASES;
do
    echo "           *******************"
    echo "           * $i START"
    echo "           *******************"
    echo "==================="
    ./run.sh $i $1 $2 $3
    echo "           *******************"
    echo "           * $i DONE"
    echo "           *******************"
done