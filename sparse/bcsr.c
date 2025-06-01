// uncomment to disable assert()
// #define NDEBUG
#include <stdio.h>
#include <cstdlib>
#include <stdbool.h>
#include <cassert>
// Use (void) to silence unused warnings.
#define assertm(exp, msg) assert((void(msg), exp))

#include "../dense/dense.h"
#include "bcsr.h"
#include <immintrin.h>

template<typename T>
void build(T **a, int m, int n){
    *a = static_cast<T *>(aligned_alloc(32, m * n * sizeof(T)));
}

bcsr_t *bcsr_from_dense(dense_t dense, int rows, int cols, int r, int c) {
    //assertm(rows % r == 0, "rows not divisible by r")
    //assertm(cols % c == 0, "cols not divisible by c")

    int br = rows / r;
    int bc = cols / c;

    /*
     * COUNT THE SHIT
     */

    // number of blocks needed
    int k = 0;
    // array of zeros and indexes, non-zero value denoting the block has some values in it
    int *is_valid_block;
    build(&is_valid_block, br, bc);
    
    if (!is_valid_block) {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < br * bc; i++) {
        is_valid_block[i] = -1;
    }

    int b_row_start_index = 0;

    // block row/col index
    for (int brow = 0; brow < br; brow++) {
        for (int bcol = 0; bcol < bc; bcol++) {

            bool need_new_block = false;

            // element row/col index inside of block
            for (int row = 0; row < r; row++) {
                for (int col = 0; col < c; col++) {

                    // index into dense matrix
                    int i = brow * r + row;
                    int j = bcol * c + col;
                    int ij = i * cols + j;

                    dense_elem_t val = dense[ij];
                    need_new_block = need_new_block || (val == -1.0 || val == 1.0);
                }
            }

            if (need_new_block) {
                // TODOX
                //matrix->b_row_start[b_row_start_index++] = block_index;
                is_valid_block[brow * bc + bcol] = k++;
            }
        }
    }

    
    bcsr_t *matrix;
    build(&matrix, 1, 1);

    if (!matrix) {
        exit(EXIT_FAILURE);
    }

    // TODO: handle malloc failure

    matrix->k = k;
    matrix->r = r;
    matrix->c = c;
    matrix->br = br;
    matrix->bc = bc;
    
    build(&matrix->b_values, k, r * c);
    build(&matrix->b_row_start, 1, br + 1);
    build(&matrix->b_col_idx, 1, k);
    
    if (!matrix->b_values || !matrix->b_row_start || !matrix->b_col_idx) {
        exit(EXIT_FAILURE);
    }


    b_row_start_index = 0;

    for (int brow = 0; brow < br; brow++) {

        bool is_new_row = true;

        for (int bcol = 0; bcol < bc; bcol++) {

            int block_index = is_valid_block[brow * bc + bcol];

            if (block_index == -1) {
                // block has only zeros, thus skip it
                continue;
            }

            if (is_new_row) {
                matrix->b_row_start[b_row_start_index++] = block_index;
                is_new_row = false;
            }

            matrix->b_col_idx[block_index] = bcol;

            // element row/col index inside of block
            for (int row = 0; row < r; row++) {
                for (int col = 0; col < c; col++) {

                    // index into dense matrix
                    int i = brow * r + row;
                    int j = bcol * c + col;
                    int ij = i * cols + j;

                    // index into block values
                    int bij = block_index * r * c + (row * c + col);
                    matrix->b_values[bij] = (bcsr_elem_t) dense[ij];
                }
            }
        }
    }
    matrix->b_row_start[b_row_start_index] = k;
    return matrix;
}

void bcsr_sgemm_basic(
    const dense_t __restrict X, const  bcsr_t __restrict W,  const dense_t __restrict B, dense_t __restrict Y,
    int M, int N, int K
) {
    // Initialize output matrix Y with bias values
    for (int m = 0; m < M; m++) {
        for (int n = 0; n < N; n++) {
            Y[m * N + n] = B[n];
        }
    }
    
    int r = W.r;  // #rows in each block
    int c = W.c;  // #columns in each block
    
    // Perform sparse-dense matrix multiplication using blocks
    for (int m = 0; m < M; m++) { // For each row in the dense input matrix X
        for (int br = 0; br < W.br; br++) { // For each block row in the sparse matrix W
            // Process only non-zero blocks in the current block row
            for (int bi = W.b_row_start[br]; bi < W.b_row_start[br + 1]; bi++) {
                int bc = W.b_col_idx[bi]; // Column position of the current block in the output
                
                // Dense multiplication within the current block
                for (int i = 0; i < r; i++) { // For each row in the current block
                    for (int j = 0; j < c; j++) { // For each column in the current block
                        // Get the element value from the current block
                        dense_elem_t val = W.b_values[bi * r * c + i * c + j];
                        
                        // Multiply and accumulate
                        Y[m * N + bc * c + j] += X[m * K + br * r + i] * val;
                    }
                }
            }
        }
    }
}

void bcsr_sgemm_prelu_basic(
    const dense_t __restrict X, const  bcsr_t __restrict W, const dense_t __restrict B, float a, dense_t __restrict Y,
    int M, int N, int K
) {
    int r = W.r;  // Number of rows in each block
    int c = W.c;  // Number of columns in each block
    
    // Initialize output matrix Y with bias values
    for (int m = 0; m < M; m++) {
        for (int n = 0; n < N; n++) {
            Y[m * N + n] = B[n];
        }
    }
    
    // Perform sparse-dense matrix multiplication using blocks and apply PReLU in one pass
    for (int m = 0; m < M; m++) { // For each row in the dense input matrix X
        for (int br = 0; br < W.br; br++) { // For each block row in the sparse matrix W
            // Process only non-zero blocks in the current block row
            for (int bi = W.b_row_start[br]; bi < W.b_row_start[br + 1]; bi++) {
                int bc = W.b_col_idx[bi]; // Column position of the current block in the output
                
                // Dense multiplication within the current block
                for (int i = 0; i < r; i++) { // For each row in the current block
                    for (int j = 0; j < c; j++) { // For each column in the current block
                        // Get the element value from the current block
                        dense_elem_t val = W.b_values[bi * r * c + i * c + j];
                        
                        // Get current output value
                        float result = Y[m * N + bc * c + j];
                        
                        // Multiply, accumulate, and apply PReLU in one step
                        result += X[m * K + br * r + i] * val;
                        result = (result > 0) ? result : a * result;
                        
                        // Write back to memory only once
                        Y[m * N + bc * c + j] = result;
                    }
                }
            }
        }
    }
}



void bcsr_sgemm_avx(
    const dense_t __restrict X, const bcsr_t __restrict W, const dense_t __restrict B, dense_t __restrict Y,
    int M, int N, int K
){
    // Initialize output matrix Y with bias values
    for (int m = 0; m < M; m++) {
        for (int n = 0; n < N; n+=8) {
            __m256 b = _mm256_load_ps(&B[n]);
            _mm256_store_ps(&Y[m * N + n], b);
        }
    }
    
    int r = W.r;  // #rows in each block
    int c = W.c;  // #columns in each block
    
    // Perform sparse-dense matrix multiplication using blocks
    for (int m = 0; m < M; m++) { // For each row in the dense input matrix X
        for (int br = 0; br < W.br; br++) { // For each block row in the sparse matrix W
            // Process only non-zero blocks in the current block row
            for (int bi = W.b_row_start[br]; bi < W.b_row_start[br + 1]; bi++) {
                int bc = W.b_col_idx[bi]; // Column position of the current block in the output
                
                // Dense multiplication within the current block, unrolled with avx
                for (int i = 0; i < r; i++) {
                    
                    dense_elem_t val = X[m * K + br * r + i];
                    __m256 x = _mm256_set1_ps(val);

                    __m256 w = _mm256_load_ps(&W.b_values[bi * r * c + i * c]);
                    __m256 y = _mm256_load_ps(&Y[m * N + bc * c]);

                    // multiply and accumulate
                    y = _mm256_fmadd_ps(x, w, y);

                    _mm256_store_ps(&Y[m * N + bc * c],  y);
                }
            }
        }
    }
}


void bcsr_sgemm_prelu_avx(
    const dense_t __restrict X, const  bcsr_t __restrict W, const dense_t __restrict B, float a, dense_t __restrict Y,
    int M, int N, int K
) {
    int r = W.r;  // Number of rows in each block
    int c = W.c;  // Number of columns in each block
    
    // Initialize output matrix Y with bias values
    for (int m = 0; m < M; m++) {
        for (int n = 0; n < N; n+=8) {
            __m256 b = _mm256_load_ps(&B[n]);
            _mm256_store_ps(&Y[m * N + n], b);
        }
    }
    
    __m256 relu_param = _mm256_set1_ps(a);  
    __m256 zero = _mm256_setzero_ps();    
    
    // Perform sparse-dense matrix multiplication using blocks and apply PReLU in one pass
    for (int m = 0; m < M; m++) { // For each row in the dense input matrix X
        for (int br = 0; br < W.br; br++) { // For each block row in the sparse matrix W
            // Process only non-zero blocks in the current block row
            for (int bi = W.b_row_start[br]; bi < W.b_row_start[br + 1]; bi++) {
                int bc = W.b_col_idx[bi]; // Column position of the current block in the output
                

                // Dense multiplication within the current block, unrolled with avx
                for (int i = 0; i < r; i++) {
                    
                    dense_elem_t val = X[m * K + br * r + i];
                    __m256 x = _mm256_set1_ps(val);

                    __m256 w = _mm256_load_ps(&W.b_values[bi * r * c + i * c]);
                    __m256 y = _mm256_load_ps(&Y[m * N + bc * c]);

                    // multiply and accumulate
                    y = _mm256_fmadd_ps(x, w, y);

                    __m256 mask = _mm256_cmp_ps(y, zero, _CMP_GT_OS);
                    __m256 neg_part = _mm256_mul_ps(y, relu_param);
                    y = _mm256_blendv_ps(neg_part, y, mask);

                    _mm256_store_ps(&Y[m * N + bc * c],  y);
                }
            
            }
        }
    }
}



void bcsr_sgemm_avx2(
    const dense_t __restrict X, const bcsr_t __restrict W, const dense_t __restrict B, dense_t __restrict Y,
    int M, int N, int K
){  
    
    // Initialize output matrix Y with bias values
    for (int m = 0; m < M; m++) {
        for (int n = 0; n < N; n+=8) {
            __m256 b = _mm256_load_ps(&B[n]);
            _mm256_store_ps(&Y[m * N + n], b);
        }
    }
    
    int r = W.r;  // #rows in each block
    int c = W.c;  // #columns in each block
    
    // Perform sparse-dense matrix multiplication using blocks
    for (int m = 0; m < M; m++) { // For each row in the dense input matrix X
        for (int br = 0; br < W.br; br++) { // For each block row in the sparse matrix W
            // Process only non-zero blocks in the current block row
            for (int bi = W.b_row_start[br]; bi < W.b_row_start[br + 1]; bi++) {
                int bc = W.b_col_idx[bi]; // Column position of the current block in the output
                
                // INDEX
                int index_x = m * K + br * r;
                int index_w = bi * r * c;
                int index_y = m * N + bc * c;

                // LOADSS

                // X
                __m256 x_0 = _mm256_set1_ps(X[index_x]);
                __m256 x_1 = _mm256_set1_ps(X[index_x+1]);
                __m256 x_2 = _mm256_set1_ps(X[index_x+2]);
                __m256 x_3 = _mm256_set1_ps(X[index_x+3]);
                __m256 x_4 = _mm256_set1_ps(X[index_x+4]);
                __m256 x_5 = _mm256_set1_ps(X[index_x+5]);
                __m256 x_6 = _mm256_set1_ps(X[index_x+6]);
                __m256 x_7 = _mm256_set1_ps(X[index_x+7]);

                // W
                float *w = W.b_values + index_w;
                __m256 w_0 = _mm256_load_ps(&w[0]);
                __m256 w_1 = _mm256_load_ps(&w[1 * c]);
                __m256 w_2 = _mm256_load_ps(&w[2 * c]);
                __m256 w_3 = _mm256_load_ps(&w[3 * c]);
                __m256 w_4 = _mm256_load_ps(&w[4 * c]);
                __m256 w_5 = _mm256_load_ps(&w[5 * c]);
                __m256 w_6 = _mm256_load_ps(&w[6 * c]);
                __m256 w_7 = _mm256_load_ps(&w[7 * c]);

                // Y
                __m256 y = _mm256_load_ps(&Y[index_y]);

                y = _mm256_fmadd_ps(x_0, w_0, y);
                y = _mm256_fmadd_ps(x_1, w_1, y);
                y = _mm256_fmadd_ps(x_2, w_2, y);
                y = _mm256_fmadd_ps(x_3, w_3, y);
                y = _mm256_fmadd_ps(x_4, w_4, y);
                y = _mm256_fmadd_ps(x_5, w_5, y);
                y = _mm256_fmadd_ps(x_6, w_6, y);
                y = _mm256_fmadd_ps(x_7, w_7, y);


                _mm256_store_ps(&Y[index_y],  y);
                
            }
        }
    }
}