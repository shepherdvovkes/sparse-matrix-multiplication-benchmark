#!/bin/bash

HEADER="M,K,N,nonZero,cycles_GEMM,flops_GEMM,performance_GEMM,cycles_sGEMM,flops_sGEMM,performance_sGEMM,cycles_GEMM_PReLU,flops_GEMM_PReLU,performance_GEMM_PReLU,cycles_sGEMM_PReLU,flops_sGEMM_PReLU,performance_sGEMM_PReLU"

# print csv header
echo $HEADER
# format benchmark's output to csv
cat out.txt | \
    # remove unwanted output
    grep -vE "init|Error|Test" | \
    # join every third line
    awk 'ORS=NR%5==0 ? "\n" : " "' | \
    # replace GEMM and sGEMM with ,
    sed -E 's/\s+s*GEMM\s+/, /g' | \
    # replace GEMM_PReLU and sGEMM_PReLU with ,
    sed -E 's/\s+s*GEMM_PReLU\s+/, /g' | \
    # remove key= from key=value pairs
    sed -E "s/\w+=//g" | \
    # remove remaining spaces
    tr -d ' '

