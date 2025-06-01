#include"SparseGEMM.h"
#include"papi/my_papi.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef __x86_64__
#include "tsc_x86.h"
#endif

#ifdef __aarch64__
#include "vct_arm.h"

#ifdef PMU
#include "kperf.h"
#endif
#endif

#define NUM_RUNS 20


vector<float> generateDenseMatrix(int rows, int columns) {
    vector<float> y = vector<float>(rows * columns, 0.0f);
    for (int h = 0; h < rows; h++) {
        for (int w = 0; w < columns; w ++) {
            // Assign random float 
            y[h*columns + w] = (float)(rand()) / (float)(rand());
        }
    }
    return y;
}
  

template<typename... Args>
#ifdef __x86_64__
double measure_cycles(void (*func)(Args...), Args... args) {
    int i, num_runs;
    myInt64 cycles;
    myInt64 start;
    num_runs = NUM_RUNS;
    start = start_tsc();
    for (i = 0; i < num_runs; ++i) {
        func(args...);
    }
    cycles =  stop_tsc(start) / num_runs;
    return (double)cycles;
}
#endif

#ifdef __aarch64__
double measure_cycles(void (*func)(Args...), Args... args) {
    int i, num_runs;
    TIMESTAMP cycles;
    TIMESTAMP start;
    num_runs = NUM_RUNS;
    start = start_vct();
    for (i = 0; i < num_runs; ++i) {
        func(args...);
    }
    cycles =  stop_vct(start) / num_runs;
    return (double)cycles;
}
#endif

int main() {
    // original test cases
    // int TEST_CASES = 8;
    // int M[] = {    1,   16,   64,   256, 1000, 4000, 16000, 64000 };
    // int K[] = {  512, 1024, 2048,  4096, 2048, 4096,  8192, 16384 };
    // int N[] = { 2048, 4096, 8192, 16384,  512, 1024,  2048,  4096 };
    // preliminary test cases
    int TEST_CASES = 3;
    int M[] = {   1,   16,   64 }; //,  256 };
    int K[] = { 256,  512, 1024 }; //, 2048 };
    int N[] = { 512, 1024, 2048 }; //, 4096 };

    int TEST_CASES_SPARSITY = 3;
    int nonZero[] = { 2, 8, 16 }; // 1/2, 1/4, 1/8, 1/16 non-zero values
    float a = 0.25f; // parameter for relu, maybe array to see and test best?

    init_papi();

    for (int z = 0; z < TEST_CASES_SPARSITY; z++) {
        for (int m = 0; m < TEST_CASES; m++) {
            for (int n = 0; n < TEST_CASES; n++) {

                //
                // SETUP
                //

                printf("M=%d, K=%d, N=%d, nonZero=%d\n", M[m], K[n], N[n], nonZero[z]);

                vector<float> X = initX<float>(M[m] * K[n], 512);
                vector<int> W = generateSparseMatrix<int>(K[n], N[n], nonZero[z], false);
                vector<float> W_FP32(W.begin(), W.end());
                vector<float> Y(M[m]  * N[n], 0);
                vector<float> B(N[n], 2);
                vector<float> refY(M[m]  * N[n], 0);
                SparseFormat sf = SparseFormat(W.data(), K[n], N[n]);

                //
                // TESTING VALIDITY (sGEMM vs GEMM)
                //

                
                start_flop_count();
                sparseGEMM(X.data(), sf.col_start_pos.data(), sf.col_start_neg.data(), sf.row_index_pos.data(), sf.row_index_neg.data(), B.data(), Y.data(), M[m], N[n], K[n]);
                double flops_sparseGEMM = stop_flop_count();
                
                start_flop_count();
                GEMM(X.data(), W_FP32.data(), B.data(), refY.data(), M[m], N[n], K[n]);
                double flops_GEMM = stop_flop_count();

                if (!compare_results(Y.data(), refY.data(), M[m], N[n])) {
                    cout << "Test case not passed!" << endl;
                }

                // clean up
                fill(Y.begin(), Y.end(), 0);
                fill(refY.begin(), refY.end(), 0);

                //
                // TESTING VALIDITY (sGEMM_PReLU vs GEMM_PReLU)
                //
                start_flop_count();
                sparseGEMM_PReLU(X.data(), sf.col_start_pos.data(),sf.col_start_neg.data(), sf.row_index_pos.data(), sf.row_index_neg.data() , B.data(), Y.data(), M[m], N[n], K[n], a);
                double flops_sparseGEMM_PReLU = stop_flop_count();
                
                start_flop_count();
                GEMM_PReLU(X.data(), W_FP32.data(), B.data(), refY.data(), M[m], N[n], K[n], a);
                double flops_GEMM_PReLU = stop_flop_count();


                if (!compare_results(Y.data(), refY.data(), M[m], N[n])) {
                    cout << "Test case not passed, PReLU!" << endl;
                }

                // clean up
                fill(Y.begin(), Y.end(), 0);
                fill(refY.begin(), refY.end(), 0);

                //
                // TIMING
                //

            
                double cycles_sparseGEMM = measure_cycles(sparseGEMM, X.data(), sf.col_start_pos.data(), sf.col_start_neg.data(), sf.row_index_pos.data(), sf.row_index_neg.data(), B.data(), Y.data(), M[m], N[n], K[n]);
                double cycles_GEMM = measure_cycles(GEMM, X.data(), W_FP32.data(), B.data(), refY.data(), M[m], N[n], K[n]);

                fill(Y.begin(), Y.end(), 0);
                fill(refY.begin(), refY.end(), 0);

                double cycles_sparseGEMM_PReLU = measure_cycles(sparseGEMM_PReLU, X.data(), sf.col_start_pos.data(),sf.col_start_neg.data(), sf.row_index_pos.data(), sf.row_index_neg.data() , B.data(), Y.data(), M[m], N[n], K[n], a);
                double cycles_GEMM_PReLU = measure_cycles(GEMM_PReLU, X.data(), W_FP32.data(), B.data(), refY.data(), M[m], N[n], K[n], a);


                //
                // FLOP COUNT + PERFORMANCE
                //

                // for convenience to not obscure the flop count calculations
                double _m = double(M[m]), _n = double(N[n]);
                double _k = double(K[n]), _z = double(nonZero[z]);

                // double flops_sparseGEMM = _m * _n * (_k / _z + 1);

                // NOTE: the flops for PReLUs are lower bounded,
                //       this ensures the performance is lower bounded,
                //       otherwise one would need to make probabilistic
                //       assumptions about how many negative elements exist in
                //       expectation
                //       This is essentially a worst-case bound of performance

                double performance_GEMM = flops_GEMM / cycles_GEMM;
                double performance_sparseGEMM = flops_sparseGEMM / cycles_sparseGEMM;

                double performance_GEMM_PReLU = flops_GEMM_PReLU / cycles_GEMM_PReLU;
                double performance_sparseGEMM_PReLU = flops_sparseGEMM_PReLU / cycles_sparseGEMM_PReLU;

                printf(
                    "GEMM  cycles=%.0f, flops=%.0f, performance=%.4f\n",
                    cycles_GEMM, flops_GEMM, performance_GEMM
                );
                printf(
                    "sGEMM cycles=%.0f, flops=%.0f, performance=%.4f\n", 
                    cycles_sparseGEMM, flops_sparseGEMM, performance_sparseGEMM
                );

                printf(
                    "GEMM_PReLU  cycles=%.0f, flops=%.0f, performance=%.4f\n",
                    cycles_GEMM_PReLU, flops_GEMM_PReLU, performance_GEMM_PReLU
                );
                printf(
                    "sGEMM_PReLU cycles=%.0f, flops=%.0f, performance=%.4f\n",
                    cycles_sparseGEMM_PReLU, flops_sparseGEMM_PReLU, performance_sparseGEMM_PReLU
                );
            }
        }
    }
    return 0;
}
