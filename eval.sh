#!/bin/bash

EVAL=./eval/golden_tdfsim
CIR_PATH=./sample_circuits
PAT_PATH=./tdf_patterns

CKT_FILE=$CIR_PATH/c$1.ckt
PAT_FILE=$PAT_PATH/c$1.pat

print_help () {
    echo "[Help]                   "
    echo "Usage:                   "
    echo "         ./eval.sh <ckt#>"
    echo "Example:                 "
    echo "         ./eval.sh 17    "
}

if [ $# -eq 1 ]; then
    $EVAL -ndet 1 -tdfsim $PAT_FILE $CKT_FILE
else
    print_help
fi
