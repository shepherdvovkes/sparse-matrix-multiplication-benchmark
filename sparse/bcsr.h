#pragma once

#include "../dense/dense.h"

typedef float bcsr_elem_t;

typedef struct {
    int r, c, br, bc, k;
    int *b_row_start;
    int *b_col_idx;
    bcsr_elem_t *b_values;
} bcsr_t;

bcsr_t *bcsr_from_dense(dense_t dense, int rows, int cols, int r, int c);

void bcsr_sgemm_basic(
    const  dense_t __restrict X, const bcsr_t __restrict W, const dense_t __restrict B,  dense_t __restrict Y,
    int M, int N, int K
);

void bcsr_sgemm_prelu_basic(
    const dense_t __restrict X, const bcsr_t __restrict W, const dense_t __restrict B, float a, dense_t __restrict Y,
    int M, int N, int K
);

void bcsr_sgemm_avx(
    const dense_t __restrict X, const bcsr_t __restrict W, const dense_t __restrict B, dense_t __restrict Y,
    int M, int N, int K
);

void bcsr_sgemm_prelu_avx(
    const dense_t __restrict X, const  bcsr_t __restrict W, const dense_t __restrict B, float a, dense_t __restrict Y,
    int M, int N, int K
);

void bcsr_sgemm_avx2(
    const dense_t __restrict X, const bcsr_t __restrict W, const dense_t __restrict B, dense_t __restrict Y,
    int M, int N, int K
);