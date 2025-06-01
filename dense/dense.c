#include <stdio.h>
#include <stdlib.h>

#include "dense.h"
#include "utils.h"

/*
 * Initialize elements uniformly at random in [-1, +1)
 */
dense_t init_rand_dense(int rows, int cols) {
    dense_t m;
    // Use posix_memalign for better macOS compatibility
    if (posix_memalign((void**)&m, 32, rows * cols * sizeof(dense_elem_t)) != 0) {
        perror("posix_memalign failed @ init_rand_dense()");
        exit(EXIT_FAILURE);
    }
    rands_dense<dense_elem_t>(m, rows, cols);
    return m;
}

/*
 * Initialize elements in {-1, 0, +1} with non-uniform probabilities 
 * as defined by parameter non_zero. Meaning
 *  prob(-1) = 1 / (2 * non_zero)
 *  prob( 0) = 1 - 1 / non_zero
 *  prob(+1) = 1 / (2 * non_zero)
 */
dense_t init_rand_sparse(int rows, int cols, int non_zero) {
    dense_t m;
    // Use posix_memalign for better macOS compatibility
    if (posix_memalign((void**)&m, 32, rows * cols * sizeof(dense_elem_t)) != 0) {
        perror("posix_memalign failed @ init_rand_sparse()");
        exit(EXIT_FAILURE);
    }
    rands_sparse<dense_elem_t>(m, rows, cols, non_zero);
    return m;
}

/*
 * Comparison of two dense matrices
 */
bool compare(const dense_t result, const dense_t target, int rows, int cols) {
    dense_elem_t tol = 1e-4;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int ij = row * cols + col;
            dense_elem_t res = result[ij];
            dense_elem_t tar = target[ij];
            if (fabs(res - tar) > tol) {
                printf(
                    "Error at (row, col) = (%d, %d): expected=%f got=%f\n",
                    row, col, tar, res
                );
                return false;
            }
        }
    }
    return true;
}

/*
 * unoptimized GEMM (as baseline)
 */
void gemm_basic(
    const dense_t X, const dense_t W, const dense_t B, dense_t Y,
    int M, int N, int K
    ) {
    for (int m = 0; m < M; m++) {
        for (int n = 0; n < N; n++) {
            dense_elem_t y = 0.0;
            for (int k = 0; k < K; k++) {
                y += X[m * K + k] * W[k * N + n];
            }
            Y[m * N + n] = y + B[n];
        }
    }
}

/*
 * unoptimized GEMM PReLU (as baseline)
 */
void gemm_prelu_basic(
    const dense_t *X, const dense_t *W, const dense_t *B, float a, dense_t *Y,
    int M, int N, int K
);
