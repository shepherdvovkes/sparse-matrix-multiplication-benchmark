#pragma once

#include <cstdbool>

typedef float dense_elem_t;
typedef dense_elem_t *dense_t;

dense_t dense_random(int rows, int cols);

bool compare(const dense_t result, const dense_t target, int rows, int cols);

dense_t init_rand_dense(int rows, int cols);
dense_t init_rand_sparse(int rows, int cols, int non_zero);

// NOTE:
//  dense_t* should be used to eventually make dense_t a struct in the future,
//  however due to the lack of time, this will most likely not happen
void gemm_basic(
    const dense_t X, const dense_t W, const dense_t B, dense_t Y,
    int M, int N, int K
);
void gemm_prelu_basic(
    const dense_t X, const dense_t W, const dense_t B, float a, dense_t Y,
    int M, int N, int K
);
