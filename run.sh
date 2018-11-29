#!/bin/bash
EXE=./bin/atpg
CIR_PATH=./sample_circuits/
PAT_PATH=./tdf_patterns/

mkdir -p $PAT_PATH

# Run
CKT_FILE=c$1.ckt
PAT_FILE=c$1.pat
if [ $# -eq 1 ]
then
    $EXE -tdfatpg $CIR_PATH$CKT_FILE > $PAT_PATH$PAT_FILE
elif [ $# -eq 2 ] # only compression
then
    $EXE -tdfatpg -compression $CIR_PATH$CKT_FILE > $PAT_PATH$PAT_FILE
elif [ $# -eq 3 ] # only ndet
then
    $EXE -tdfatpg -ndet $3 $CIR_PATH$CKT_FILE > $PAT_PATH$PAT_FILE
elif [ $# -eq 4 ] # compression and ndet
then
    $EXE -tdfatpg -compression -ndet $4 $CIR_PATH$CKT_FILE > $PAT_PATH$PAT_FILE
else # help
    echo "[Help]"
    echo "========================================================"
    echo "Usage: ./run.sh ckt# [c] [<n> <#det>]                   "
    echo "========================================================"
    echo "Example: ./run.sh 17        (c17)                       "
    echo "         ./run.sh 17 c      (c17 + compression)         "
    echo "         ./run.sh 17 n 8    (c17 + ndet 8)              "
    echo "         ./run.sh 17 c n 8  (c17 + compression + ndet 8)"
fi
