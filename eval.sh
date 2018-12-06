#!/bin/bash

EVAL=./eval/atpg
CIR_PATH=./sample_circuits
PAT_PATH=./tdf_patterns

CKT_FILE=$CIR_PATH/c$1.ckt
PAT_FILE=$PAT_PATH/c$1.pat

$EVAL -tdfsim $PAT_FILE $CKT_FILE
