#!/bin/bash
printOnScreen=false
printOnScreen=true

EXE=./bin/atpg
CIR_PATH=./sample_circuits
PAT_PATH=./tdf_patterns

# Run
mkdir -p $PAT_PATH
CKT_FILE=$CIR_PATH/c$1.ckt
PAT_FILE=$PAT_PATH/c$1.pat

print_help () {
    echo "[Help]"
    echo "==========================================================="
    echo "Usage: ./run.sh ckt# [c] [<n> <#det>]                      "
    echo "==========================================================="
    echo "Example: ./run.sh 17        (c17)                          "
    echo "         ./run.sh 17 c      (c17 + compression)            "
    echo "         ./run.sh 17 n 8    (c17 + ndet 8)                 "
    echo "         ./run.sh 17 c n 8  (c17 + compression + ndet 8)   "
    echo "==========================================================="
    echo "Circuits: 17 432 499 880 1355 1908 2670 3540 5315 6288 7552"
}

if [ "$printOnScreen" = true ] # Print stdout to the screen
then
    if [ $# -eq 1 ]; then
        $EXE -tdfatpg $CKT_FILE
    elif [ $# -eq 2 ]; then # only compression
        $EXE -tdfatpg -compression $CKT_FILE
    elif [ $# -eq 3 ]; then # only ndet
        $EXE -tdfatpg -ndet $3 $CKT_FILE
    elif [ $# -eq 4 ]; then # compression and ndet
        $EXE -tdfatpg -compression -ndet $4 $CKT_FILE
    else # help
        print_help
    fi
else # Redirect stdout to the file
    if [ $# -eq 1 ]; then
        $EXE -tdfatpg $CKT_FILE > $PAT_FILE
    elif [ $# -eq 2 ]; then # only compression
        $EXE -tdfatpg -compression $CKT_FILE > $PAT_FILE
    elif [ $# -eq 3 ]; then # only ndet
        $EXE -tdfatpg -ndet $3 $CKT_FILE > $PAT_FILE
    elif [ $# -eq 4 ]; then # compression and ndet
        $EXE -tdfatpg -compression -ndet $4 $CKT_FILE > $PAT_FILE
    else # help
        print_help
    fi
fi
