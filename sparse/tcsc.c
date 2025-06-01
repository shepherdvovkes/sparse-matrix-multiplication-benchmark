#include "tcsc.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

tcsc_t *tcsc_from_dense(dense_t dense, int rows, int cols) {
    // Initialize counters
    int n_elem_pos = 0, n_elem_neg = 0;
    
    // Count the number of pos and neg elements for malloc
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            float value = dense[i * cols + j]; // Fixed indexing: was i * rows + j
            if (value == 1.0f)
                n_elem_pos++;
            else if (value == -1.0f)
                n_elem_neg++;
        }
    }

    // Allocate memory for the sparse matrix
    tcsc_t* sparse = (tcsc_t*) malloc(sizeof(tcsc_t));
    if (!sparse) return NULL;

    sparse->rows = rows;
    sparse->cols = cols;
    sparse->n_elem_pos = n_elem_pos;
    sparse->n_elem_neg = n_elem_neg;

    sparse->col_start_pos = (int*) malloc((cols + 1) * sizeof(int));
    sparse->col_start_neg = (int*) malloc((cols + 1) * sizeof(int));
    sparse->row_index_pos = (int*) malloc(n_elem_pos * sizeof(int));
    sparse->row_index_neg = (int*) malloc(n_elem_neg * sizeof(int));

    if (!sparse->col_start_pos || !sparse->col_start_neg || 
        !sparse->row_index_pos || !sparse->row_index_neg) {
        free(sparse->col_start_pos);
        free(sparse->col_start_neg);
        free(sparse->row_index_pos);
        free(sparse->row_index_neg);
        free(sparse);
        return NULL;
    }

    int pos_counter = 0, neg_counter = 0;
    
    // Populate sparse matrix arrays - column-wise traversal
    for (int j = 0; j < cols; ++j) {
        sparse->col_start_pos[j] = pos_counter;
        sparse->col_start_neg[j] = neg_counter;

        for (int i = 0; i < rows; ++i) {
            float value = dense[i * cols + j];
            if (value == 1.0f) {
                sparse->row_index_pos[pos_counter++] = i;
            } else if (value == -1.0f) {
                sparse->row_index_neg[neg_counter++] = i;
            }
        }
    }

    sparse->col_start_pos[cols] = pos_counter;
    sparse->col_start_neg[cols] = neg_counter;

    return sparse;
}

// Basic TCSC SGEMM implementation
void tcsc_sgemm_basic(
    const dense_t X, const tcsc_t* W, const dense_t B, dense_t Y,
    int M, int N, int K
) {
    // Initialize Y with bias values
    for (int m = 0; m < M; ++m) {
        for (int n = 0; n < N; ++n) {
            Y[m * N + n] = B[n];
        }
    }
    
    // Sparse matrix multiplication
    for (int m = 0; m < M; ++m) {
        for (int n = 0; n < N; ++n) {
            float y = Y[m * N + n]; // Start with bias
            
            // Process positive values
            for (int k = W->col_start_pos[n]; k < W->col_start_pos[n + 1]; ++k) {
                y += X[m * K + W->row_index_pos[k]];
            }
            
            // Process negative values
            for (int k = W->col_start_neg[n]; k < W->col_start_neg[n + 1]; ++k) {
                y -= X[m * K + W->row_index_neg[k]];
            }
            
            Y[m * N + n] = y;
        }
    }
}

// Optimized TCSC SGEMM with precomputed bias and better cache usage
void tcsc_sgemm_optimized(
    const dense_t X, const tcsc_t* W, const dense_t B, dense_t Y,
    int M, int N, int K
) {
    // Precompute bias in separate loop to reduce cache conflicts
    for (int m = 0; m < M; ++m) {
        for (int n = 0; n < N; ++n) {
            Y[m * N + n] = B[n];
        }
    }
    
    // Sparse matrix multiplication - optimized loop order for better cache usage
    for (int n = 0; n < N; ++n) {
        // Process positive values for this column
        int pos_start = W->col_start_pos[n];
        int pos_end = W->col_start_pos[n + 1];
        
        for (int m = 0; m < M; ++m) {
            float acc_pos = 0.0f;
            // Local accumulation to reduce memory accesses
            for (int k = pos_start; k < pos_end; ++k) {
                acc_pos += X[m * K + W->row_index_pos[k]];
            }
            Y[m * N + n] += acc_pos;
        }
        
        // Process negative values for this column
        int neg_start = W->col_start_neg[n];
        int neg_end = W->col_start_neg[n + 1];
        
        for (int m = 0; m < M; ++m) {
            float acc_neg = 0.0f;
            // Local accumulation to reduce memory accesses
            for (int k = neg_start; k < neg_end; ++k) {
                acc_neg += X[m * K + W->row_index_neg[k]];
            }
            Y[m * N + n] -= acc_neg;
        }
    }
}

// PReLU version - basic
void tcsc_sgemm_prelu_basic(
    const dense_t X, const tcsc_t* W, const dense_t B, float a, dense_t Y,
    int M, int N, int K
) {
    for (int m = 0; m < M; ++m) {
        for (int n = 0; n < N; ++n) {
            float y = 0.0f;
            
            // Process positive values
            for (int k = W->col_start_pos[n]; k < W->col_start_pos[n + 1]; ++k) {
                y += X[m * K + W->row_index_pos[k]];
            }
            
            // Process negative values
            for (int k = W->col_start_neg[n]; k < W->col_start_neg[n + 1]; ++k) {
                y -= X[m * K + W->row_index_neg[k]];
            }
            
            y += B[n];
            Y[m * N + n] = (y < 0.0f) ? a * y : y;
        }
    }
}

void tcsc_free(tcsc_t *W) {
    if (W) {
        free(W->col_start_pos);
        free(W->col_start_neg);
        free(W->row_index_pos);
        free(W->row_index_neg);
        free(W);
    }
}

// PReLU optimized version - separate loop approach
// First compute matrix multiplication, then apply PReLU in separate pass
void tcsc_sgemm_prelu_optimized_separate(
    const dense_t X, const tcsc_t* W, const dense_t B, float a, dense_t Y,
    int M, int N, int K
) {
    // Phase 1: Precompute bias in separate loop to reduce cache conflicts
    for (int m = 0; m < M; ++m) {
        for (int n = 0; n < N; ++n) {
            Y[m * N + n] = B[n];
        }
    }
    
    // Phase 2: Sparse matrix multiplication with optimized loop order
    for (int n = 0; n < N; ++n) {
        // Process positive values for this column
        int pos_start = W->col_start_pos[n];
        int pos_end = W->col_start_pos[n + 1];
        
        for (int m = 0; m < M; ++m) {
            float acc_pos = 0.0f;
            // Local accumulation to reduce memory accesses
            for (int k = pos_start; k < pos_end; ++k) {
                acc_pos += X[m * K + W->row_index_pos[k]];
            }
            Y[m * N + n] += acc_pos;
        }
        
        // Process negative values for this column
        int neg_start = W->col_start_neg[n];
        int neg_end = W->col_start_neg[n + 1];
        
        for (int m = 0; m < M; ++m) {
            float acc_neg = 0.0f;
            // Local accumulation to reduce memory accesses
            for (int k = neg_start; k < neg_end; ++k) {
                acc_neg += X[m * K + W->row_index_neg[k]];
            }
            Y[m * N + n] -= acc_neg;
        }
    }
    
    // Phase 3: Apply PReLU activation in separate optimized loop
    // This allows for better vectorization and cache usage
    for (int m = 0; m < M; ++m) {
        for (int n = 0; n < N; ++n) {
            float val = Y[m * N + n];
            Y[m * N + n] = (val < 0.0f) ? a * val : val;
        }
    }
}

// PReLU optimized version - compute PReLU on-the-go
// Apply PReLU immediately after computing each element
void tcsc_sgemm_prelu_optimized_onthego(
    const dense_t X, const tcsc_t* W, const dense_t B, float a, dense_t Y,
    int M, int N, int K
) {
    // Precompute bias in separate loop to reduce cache conflicts
    for (int m = 0; m < M; ++m) {
        for (int n = 0; n < N; ++n) {
            Y[m * N + n] = B[n];
        }
    }
    
    // Sparse matrix multiplication with PReLU applied on-the-go
    // Column-major processing for better cache locality
    for (int n = 0; n < N; ++n) {
        // Process positive values for this column
        int pos_start = W->col_start_pos[n];
        int pos_end = W->col_start_pos[n + 1];
        
        for (int m = 0; m < M; ++m) {
            float acc_pos = 0.0f;
            // Local accumulation for positive values
            for (int k = pos_start; k < pos_end; ++k) {
                acc_pos += X[m * K + W->row_index_pos[k]];
            }
            Y[m * N + n] += acc_pos;
        }
        
        // Process negative values for this column
        int neg_start = W->col_start_neg[n];
        int neg_end = W->col_start_neg[n + 1];
        
        for (int m = 0; m < M; ++m) {
            float acc_neg = 0.0f;
            // Local accumulation for negative values
            for (int k = neg_start; k < neg_end; ++k) {
                acc_neg += X[m * K + W->row_index_neg[k]];
            }
            Y[m * N + n] -= acc_neg;
            
            // Apply PReLU immediately after computing final value
            float val = Y[m * N + n];
            Y[m * N + n] = (val < 0.0f) ? a * val : val;
        }
    }
}
