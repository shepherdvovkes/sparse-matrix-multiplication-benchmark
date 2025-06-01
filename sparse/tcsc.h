#ifndef TCSC_H
#define TCSC_H

#include "../dense/dense.h"

typedef struct {
    int rows, cols;
    int n_elem_pos; // number of matrix elements with value +1
    int n_elem_neg; // number of matrix elements with value -1
    // has cols+1 many elements
    int* col_start_pos;
    int* col_start_neg;
    // has n_elem_pos many elements
    int* row_index_pos;
    // has n_elem_neg many elements
    int* row_index_neg;
} tcsc_t;

tcsc_t *tcsc_from_dense(dense_t dense, int rows, int cols);

void tcsc_sgemm_basic(
    const dense_t X, const tcsc_t* W, const dense_t B, dense_t Y,
    int M, int N, int K
);

void tcsc_sgemm_optimized(
    const dense_t X, const tcsc_t* W, const dense_t B, dense_t Y,
    int M, int N, int K
);

void tcsc_sgemm_prelu_basic(
    const dense_t X, const tcsc_t* W, const dense_t B, float a, dense_t Y,
    int M, int N, int K
);

// PReLU optimized version - separate loop for PReLU computation
void tcsc_sgemm_prelu_optimized_separate(
    const dense_t X, const tcsc_t* W, const dense_t B, float a, dense_t Y,
    int M, int N, int K
);

// PReLU optimized version - compute PReLU on-the-go
void tcsc_sgemm_prelu_optimized_onthego(
    const dense_t X, const tcsc_t* W, const dense_t B, float a, dense_t Y,
    int M, int N, int K
);

void tcsc_free(tcsc_t *W);

#endif
