#include <stdio.h>
#include <stdlib.h>
#include "../dense/dense.h"
#include "../sparse/bcsr.h"

int main() {
    // Test dimensions
    // int M = 16;    // Number of rows in X
    // int K = 512;   // Columns in X, Rows in W
    // int N = 1024;  // Columns in W/Y
    // int r = 4;     // Block row size
    // int c = 4;     // Block column size
    int M = 1;    // Number of rows in X
    int K = 512;   // Columns in X, Rows in W
    int N = 2048;  // Columns in W/Y
    int r = 1;     // Block row size
    int c = 8;   
    
    // Initialize matrices
    dense_t X = init_rand_dense(M, K);
    dense_t W_dense = init_rand_sparse(K, N, 2); // 1/8 non-zero elements
    dense_t B = init_rand_dense(N, 1);  // Bias vector
    dense_t Y = (dense_t)malloc(M * N * sizeof(dense_elem_t));
    dense_t Y_ref = (dense_t)malloc(M * N * sizeof(dense_elem_t));
    
    // Convert dense W to BCSR format
    bcsr_t* W_bcsr = bcsr_from_dense(W_dense, K, N, r, c);
    
    // Compute reference result using dense GEMM
    gemm_basic(X, W_dense, B, Y_ref, M, N, K);
    
    // Compute result using BCSR GEMM
    bcsr_sgemm_basic(X, *W_bcsr, B, Y, M, N, K);
    
    // Compare results
    if (compare(Y, Y_ref, M, N)) {
        printf("Test passed! Results match.\n");
    } else {
        printf("Test failed! Results don't match.\n");
    }
    
    // Free memory
    free(X);
    free(W_dense);
    free(B);
    free(Y);
    free(Y_ref);
    free(W_bcsr->b_values);
    free(W_bcsr->b_row_start);
    free(W_bcsr->b_col_idx);
    free(W_bcsr);
    
    return 0;
}