#!/bin/bash

echo "=== Adding optimized PReLU functions to TCSC ==="

# Update tcsc.h to add new function declarations
echo "Updating sparse/tcsc.h..."
cat > sparse/tcsc.h << 'EOF'
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
EOF

# Update tcsc.c to add the optimized PReLU implementations
echo "Updating sparse/tcsc.c with optimized PReLU functions..."
cat >> sparse/tcsc.c << 'EOF'

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
EOF

echo "=== PReLU optimizations added successfully! ==="
echo ""
echo "New functions added:"
echo "1. tcsc_sgemm_prelu_optimized_separate() - Three-phase approach:"
echo "   - Phase 1: Precompute bias"
echo "   - Phase 2: Matrix multiplication with cache optimizations"
echo "   - Phase 3: Apply PReLU in separate vectorizable loop"
echo ""
echo "2. tcsc_sgemm_prelu_optimized_onthego() - On-the-go approach:"
echo "   - Phase 1: Precompute bias"
echo "   - Phase 2: Matrix multiplication with immediate PReLU application"
echo ""
echo "Optimizations included:"
echo "- Bias precomputation to reduce cache conflicts"
echo "- Column-major loop ordering for better cache locality"
echo "- Local accumulation to minimize memory writes"
echo "- Separate PReLU loop allows for future vectorization"
echo "- On-the-go PReLU reduces memory traffic"
echo ""
echo "Next steps:"
echo "1. Update main.cpp to test both PReLU versions"
echo "2. Run: ./build_and_run_m1.sh"
echo "3. Compare performance: basic vs separate vs on-the-go"