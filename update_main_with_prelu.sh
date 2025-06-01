#!/bin/bash

echo "=== Updating main.cpp to test PReLU optimizations ==="

cat > main.cpp << 'EOF'
/*
 * This file is part of the Sparse Ternary Matrix Multiplication Project
 * of the Advanced Systems Lab course 2025 at ETH Zurich.
 *
 * Modified for TCSC testing on Mac M1 with PReLU optimizations
 */

// number of runs for measuring the cycles of a function
#define NUM_RUNS 20
// comment this macro out for no warm-up
#define DO_WARMUP_BEFORE_MEASURING
// minimum number of cycles required for one measurement
#define CYCLES_REQUIRED 1e8
// number of times to repeat a measurement with each iteration running NUM_RUNS
#define REP 50
// numerical tolerance between computed and ground truth
#define EPS (1e-6)

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <iomanip>

#include "common.h"
#include "dense/dense.h"
#include "sparse/tcsc.h"
#include "measure.h"
#include "progress_bar.h"
#include <string>
#include <fstream>

#include "papi/my_papi.h"

using namespace std;

template<typename T>
void build_and_check(T **a, int m, int n){
    // Use posix_memalign for better macOS compatibility
    if (posix_memalign((void**)a, 32, m * n * sizeof(T)) != 0) {
        printf("ERROR: posix_memalign failed for allocation !!!");
        exit(EXIT_FAILURE);
    }
}

// Helper function to calculate FLOP count for sparse operations
long long calculate_sparse_flops(const tcsc_t* W, int M, int N) {
    // Each non-zero element contributes 1 multiply-add = 2 FLOPs
    // Plus M*N additions for bias
    return (long long)M * (W->n_elem_pos + W->n_elem_neg) * 2 + (long long)M * N;
}

// Direct timing function for TCSC (avoiding template issues)
double measure_tcsc_cycles(void (*func)(const dense_t, const tcsc_t*, const dense_t, dense_t, int, int, int),
                          const dense_t X, const tcsc_t* W, const dense_t B, dense_t Y,
                          int M, int N, int K) {
    int i, num_runs = NUM_RUNS;
    double cycles = 0.;
#ifdef __x86_64__
    myInt64 start, end;
#endif
#ifdef __aarch64__
    TIMESTAMP start, end;
#endif

#ifdef DO_WARMUP_BEFORE_MEASURING
    double multiplier = 1.;
    do {
        num_runs = num_runs * multiplier;
#ifdef __x86_64__
        start = start_tsc();
#endif
#ifdef __aarch64__
        start = start_vct();
#endif
        for (size_t i = 0; i < num_runs; i++) {
            func(X, W, B, Y, M, N, K);
        }
#ifdef __x86_64__
        end = stop_tsc(start);
#endif
#ifdef __aarch64__
        end = stop_vct(start);
#endif
        cycles = (double)end;
        multiplier = (CYCLES_REQUIRED) / (cycles);
    } while (multiplier > 2);
#endif

    double total_cycles = 0.;
    for (int j = 0; j < REP; j++) {
#ifdef __x86_64__
        start = start_tsc();
#endif
#ifdef __aarch64__
        start = start_vct();
#endif
        for (i = 0; i < num_runs; ++i) {
            func(X, W, B, Y, M, N, K);
        }
#ifdef __x86_64__
        end = stop_tsc(start);
#endif
#ifdef __aarch64__
        end = stop_vct(start);
#endif
        cycles = ((double)end) / num_runs;
        total_cycles += cycles;
    }
    total_cycles /= REP;
    cycles = total_cycles;
    return cycles;
}

// Timing function for PReLU versions
double measure_tcsc_prelu_cycles(void (*func)(const dense_t, const tcsc_t*, const dense_t, float, dense_t, int, int, int),
                                const dense_t X, const tcsc_t* W, const dense_t B, float a, dense_t Y,
                                int M, int N, int K) {
    int i, num_runs = NUM_RUNS;
    double cycles = 0.;
#ifdef __x86_64__
    myInt64 start, end;
#endif
#ifdef __aarch64__
    TIMESTAMP start, end;
#endif

#ifdef DO_WARMUP_BEFORE_MEASURING
    double multiplier = 1.;
    do {
        num_runs = num_runs * multiplier;
#ifdef __x86_64__
        start = start_tsc();
#endif
#ifdef __aarch64__
        start = start_vct();
#endif
        for (size_t i = 0; i < num_runs; i++) {
            func(X, W, B, a, Y, M, N, K);
        }
#ifdef __x86_64__
        end = stop_tsc(start);
#endif
#ifdef __aarch64__
        end = stop_vct(start);
#endif
        cycles = (double)end;
        multiplier = (CYCLES_REQUIRED) / (cycles);
    } while (multiplier > 2);
#endif

    double total_cycles = 0.;
    for (int j = 0; j < REP; j++) {
#ifdef __x86_64__
        start = start_tsc();
#endif
#ifdef __aarch64__
        start = start_vct();
#endif
        for (i = 0; i < num_runs; ++i) {
            func(X, W, B, a, Y, M, N, K);
        }
#ifdef __x86_64__
        end = stop_tsc(start);
#endif
#ifdef __aarch64__
        end = stop_vct(start);
#endif
        cycles = ((double)end) / num_runs;
        total_cycles += cycles;
    }
    total_cycles /= REP;
    cycles = total_cycles;
    return cycles;
}

void print_fancy_header() {
    cout << "\n";
    cout << "████████╗ ██████╗███████╗ ██████╗    ██████╗ ███████╗███╗   ██╗ ██████╗██╗  ██╗\n";
    cout << "╚══██╔══╝██╔════╝██╔════╝██╔════╝    ██╔══██╗██╔════╝████╗  ██║██╔════╝██║  ██║\n";
    cout << "   ██║   ██║     ███████╗██║         ██████╔╝█████╗  ██╔██╗ ██║██║     ███████║\n";
    cout << "   ██║   ██║     ╚════██║██║         ██╔══██╗██╔══╝  ██║╚██╗██║██║     ██╔══██║\n";
    cout << "   ██║   ╚██████╗███████║╚██████╗    ██████╔╝███████╗██║ ╚████║╚██████╗██║  ██║\n";
    cout << "   ╚═╝    ╚═════╝╚══════╝ ╚═════╝    ╚═════╝ ╚══════╝╚═╝  ╚═══╝ ╚═════╝╚═╝  ╚═╝\n";
    cout << "\n** Welcome to the TCSC Performance Optimization Showcase! **\n";
    cout << ">> Testing sparse matrix multiplication with PReLU optimizations on Mac M1\n";
    cout << "========================================================================\n\n";
}

void print_test_case_header(int test_num, int total_tests, int M, int K, int N) {
    cout << "\n+----------------------------------------------------------------------+\n";
    cout << "|  [TEST " << test_num << "/" << total_tests << "] Matrix Size: " << M << "x" << K << "x" << N;
    cout << " (Sparsity: 50%)";
    cout << setw(max(0, 20 - (int)to_string(M).length() - (int)to_string(K).length() - (int)to_string(N).length())) << "|\n";
    cout << "+----------------------------------------------------------------------+\n";
}

void print_results_table(double cycles_gemm, long long flops_gemm, double perf_gemm,
                         double cycles_basic, long long flops_basic, double perf_basic,
                         double cycles_opt, long long flops_opt, double perf_opt,
                         double cycles_prelu_basic, long long flops_prelu_basic, double perf_prelu_basic,
                         double cycles_prelu_sep, long long flops_prelu_sep, double perf_prelu_sep,
                         double cycles_prelu_otg, long long flops_prelu_otg, double perf_prelu_otg) {
    
    cout << "\n[*] PERFORMANCE RESULTS:\n";
    cout << "+---------------------+-------------+-------------+-------------+\n";
    cout << "|     Algorithm       |   Cycles    |    FLOPs    | Performance |\n";
    cout << "+---------------------+-------------+-------------+-------------+\n";
    cout << "| Dense GEMM          |" << setw(12) << (long long)cycles_gemm << " |" 
         << setw(12) << flops_gemm << " |" << setw(11) << fixed << setprecision(4) << perf_gemm << " |\n";
    cout << "| TCSC Basic          |" << setw(12) << (long long)cycles_basic << " |" 
         << setw(12) << flops_basic << " |" << setw(11) << fixed << setprecision(4) << perf_basic << " |\n";
    cout << "| TCSC Optimized      |" << setw(12) << (long long)cycles_opt << " |" 
         << setw(12) << flops_opt << " |" << setw(11) << fixed << setprecision(4) << perf_opt << " |\n";
    cout << "| TCSC PReLU Basic    |" << setw(12) << (long long)cycles_prelu_basic << " |" 
         << setw(12) << flops_prelu_basic << " |" << setw(11) << fixed << setprecision(4) << perf_prelu_basic << " |\n";
    cout << "| TCSC PReLU Separate |" << setw(12) << (long long)cycles_prelu_sep << " |" 
         << setw(12) << flops_prelu_sep << " |" << setw(11) << fixed << setprecision(4) << perf_prelu_sep << " |\n";
    cout << "| TCSC PReLU OnTheGo  |" << setw(12) << (long long)cycles_prelu_otg << " |" 
         << setw(12) << flops_prelu_otg << " |" << setw(11) << fixed << setprecision(4) << perf_prelu_otg << " |\n";
    cout << "+---------------------+-------------+-------------+-------------+\n";
    
    double speedup_basic = cycles_gemm / cycles_basic;
    double speedup_opt_vs_basic = cycles_basic / cycles_opt;
    double speedup_overall = cycles_gemm / cycles_opt;
    double speedup_prelu_sep = cycles_prelu_basic / cycles_prelu_sep;
    double speedup_prelu_otg = cycles_prelu_basic / cycles_prelu_otg;
    
    cout << "\n[*] SPEEDUP ANALYSIS:\n";
    cout << "  [1] TCSC Basic vs Dense:         " << fixed << setprecision(2) << speedup_basic << "x faster\n";
    cout << "  [2] TCSC Optimized vs Basic:     " << fixed << setprecision(2) << speedup_opt_vs_basic << "x faster\n";
    cout << "  [3] Overall Optimization:        " << fixed << setprecision(2) << speedup_overall << "x faster\n";
    cout << "  [4] PReLU Separate vs Basic:     " << fixed << setprecision(2) << speedup_prelu_sep << "x faster\n";
    cout << "  [5] PReLU OnTheGo vs Basic:      " << fixed << setprecision(2) << speedup_prelu_otg << "x faster\n";
    
    // Determine best PReLU optimization
    if (cycles_prelu_sep < cycles_prelu_otg) {
        cout << "  >>> Best PReLU approach: Separate loop (better vectorization potential)\n";
    } else {
        cout << "  >>> Best PReLU approach: On-the-go (reduced memory traffic)\n";
    }
    
    if (speedup_opt_vs_basic > 1.2) {
        cout << "  >>> Excellent optimization! C optimizations are working great!\n";
    } else if (speedup_opt_vs_basic > 1.1) {
        cout << "  >>> Good optimization! Visible improvement from C optimizations.\n";
    } else {
        cout << "  >>> Modest optimization. Consider more advanced techniques.\n";
    }
}

int main(int argc, char **argv) {
    print_fancy_header();
    
    ProgressBar::show_thinking_animation("Initializing PAPI and performance counters", 1500);
    init_papi();
    
    vector<tuple<int, int, int>> basicTestCases = {
        {   1,  512,  2048},
        {   1, 1024,  4096},
        {   1, 2048,  8192},
        { 256,  512,  2048},
        { 256, 1024,  4096},
    };

    ProgressBar overall_progress(basicTestCases.size(), "[*] Overall Benchmark Progress");
    
    float prelu_alpha = 0.2f; // PReLU parameter
    
    for (size_t test_idx = 0; test_idx < basicTestCases.size(); test_idx++) {
        const auto& [M_ROW, K_LEN, N_COL] = basicTestCases[test_idx];
        
        print_test_case_header(test_idx + 1, basicTestCases.size(), M_ROW, K_LEN, N_COL);
        
        ProgressBar::show_thinking_animation("Allocating and initializing matrices", 1000);

        dense_elem_t *Y, *refY, *Y_prelu, *refY_prelu, *Y_prelu_sep, *Y_prelu_otg;
        const dense_t W_dense = init_rand_sparse(K_LEN, N_COL, 2);
        const dense_t X = init_rand_dense(M_ROW, K_LEN);
        const dense_t B = init_rand_dense(N_COL, 1);

        build_and_check(&Y, M_ROW, N_COL);
        build_and_check(&refY, M_ROW, N_COL);
        build_and_check(&Y_prelu, M_ROW, N_COL);
        build_and_check(&refY_prelu, M_ROW, N_COL);
        build_and_check(&Y_prelu_sep, M_ROW, N_COL);
        build_and_check(&Y_prelu_otg, M_ROW, N_COL);

        ProgressBar::show_thinking_animation("Converting to TCSC sparse format", 800);
        tcsc_t *W_tsparse = tcsc_from_dense(W_dense, K_LEN, N_COL);

        // Calculate FLOP counts
        long long flops_gemm_basic = 2LL * M_ROW * N_COL * K_LEN + M_ROW * N_COL;
        long long flops_sparse = calculate_sparse_flops(W_tsparse, M_ROW, N_COL);

        cout << "[*] Matrix info: " << W_tsparse->n_elem_pos + W_tsparse->n_elem_neg 
             << " non-zeros out of " << K_LEN * N_COL << " elements\n";

        // Validation phase
        cout << "\n[*] Running validation tests...\n";
        
        // Test basic functions
        start_flop_count();
#ifdef DISABLE_PAPI
        set_flop_count(flops_gemm_basic);
#endif
        gemm_basic(X, W_dense, B, refY, M_ROW, N_COL, K_LEN);
        long long measured_flops_gemm = stop_flop_count();

        start_flop_count();
#ifdef DISABLE_PAPI
        set_flop_count(flops_sparse);
#endif
        tcsc_sgemm_basic(X, W_tsparse, B, Y, M_ROW, N_COL, K_LEN);
        long long measured_flops_basic = stop_flop_count();

        if (!compare(Y, refY, M_ROW, N_COL)) {
            cout << "[ERROR] basic_tcsc failed validation!!!" << endl;
            exit(1);
        }

        build_and_check(&Y, M_ROW, N_COL);
        start_flop_count();
#ifdef DISABLE_PAPI
        set_flop_count(flops_sparse);
#endif
        tcsc_sgemm_optimized(X, W_tsparse, B, Y, M_ROW, N_COL, K_LEN);
        long long measured_flops_opt = stop_flop_count();

        if (!compare(Y, refY, M_ROW, N_COL)) {
            cout << "[ERROR] optimized_tcsc failed validation!!!" << endl;
            exit(1);
        }

        // Test PReLU functions
        start_flop_count();
#ifdef DISABLE_PAPI
        set_flop_count(flops_sparse);
#endif
        tcsc_sgemm_prelu_basic(X, W_tsparse, B, prelu_alpha, Y_prelu, M_ROW, N_COL, K_LEN);
        long long measured_flops_prelu_basic = stop_flop_count();

        start_flop_count();
#ifdef DISABLE_PAPI
        set_flop_count(flops_sparse);
#endif
        tcsc_sgemm_prelu_optimized_separate(X, W_tsparse, B, prelu_alpha, Y_prelu_sep, M_ROW, N_COL, K_LEN);
        long long measured_flops_prelu_sep = stop_flop_count();

        start_flop_count();
#ifdef DISABLE_PAPI
        set_flop_count(flops_sparse);
#endif
        tcsc_sgemm_prelu_optimized_onthego(X, W_tsparse, B, prelu_alpha, Y_prelu_otg, M_ROW, N_COL, K_LEN);
        long long measured_flops_prelu_otg = stop_flop_count();

        // Validate PReLU versions against each other
        if (!compare(Y_prelu, Y_prelu_sep, M_ROW, N_COL)) {
            cout << "[ERROR] PReLU separate failed validation!!!" << endl;
            exit(1);
        }
        
        if (!compare(Y_prelu, Y_prelu_otg, M_ROW, N_COL)) {
            cout << "[ERROR] PReLU on-the-go failed validation!!!" << endl;
            exit(1);
        }

        cout << "[OK] All validation tests passed!\n";

        // Performance measurements
        cout << "\n[*] Starting performance measurements...\n";
        
        build_and_check(&Y, M_ROW, N_COL);
        build_and_check(&refY, M_ROW, N_COL);

        double cycles_gemm_basic = measure_cycles(gemm_basic, X, W_dense, B, refY, M_ROW, N_COL, K_LEN);
        
        build_and_check(&Y, M_ROW, N_COL);
        double cycles_tsgemm_basic = measure_tcsc_cycles(tcsc_sgemm_basic, X, W_tsparse, B, Y, M_ROW, N_COL, K_LEN);
        
        build_and_check(&Y, M_ROW, N_COL);
        double cycles_tsgemm_opt = measure_tcsc_cycles(tcsc_sgemm_optimized, X, W_tsparse, B, Y, M_ROW, N_COL, K_LEN);

        build_and_check(&Y_prelu, M_ROW, N_COL);
        double cycles_prelu_basic = measure_tcsc_prelu_cycles(tcsc_sgemm_prelu_basic, X, W_tsparse, B, prelu_alpha, Y_prelu, M_ROW, N_COL, K_LEN);
        
        build_and_check(&Y_prelu_sep, M_ROW, N_COL);
        double cycles_prelu_sep = measure_tcsc_prelu_cycles(tcsc_sgemm_prelu_optimized_separate, X, W_tsparse, B, prelu_alpha, Y_prelu_sep, M_ROW, N_COL, K_LEN);
        
        build_and_check(&Y_prelu_otg, M_ROW, N_COL);
        double cycles_prelu_otg = measure_tcsc_prelu_cycles(tcsc_sgemm_prelu_optimized_onthego, X, W_tsparse, B, prelu_alpha, Y_prelu_otg, M_ROW, N_COL, K_LEN);

        // Calculate performance metrics
        double perf_gemm = (double)measured_flops_gemm / cycles_gemm_basic;
        double perf_basic = (double)measured_flops_basic / cycles_tsgemm_basic;
        double perf_opt = (double)measured_flops_opt / cycles_tsgemm_opt;
        double perf_prelu_basic = (double)measured_flops_prelu_basic / cycles_prelu_basic;
        double perf_prelu_sep = (double)measured_flops_prelu_sep / cycles_prelu_sep;
        double perf_prelu_otg = (double)measured_flops_prelu_otg / cycles_prelu_otg;

        print_results_table(cycles_gemm_basic, measured_flops_gemm, perf_gemm,
                           cycles_tsgemm_basic, measured_flops_basic, perf_basic,
                           cycles_tsgemm_opt, measured_flops_opt, perf_opt,
                           cycles_prelu_basic, measured_flops_prelu_basic, perf_prelu_basic,
                           cycles_prelu_sep, measured_flops_prelu_sep, perf_prelu_sep,
                           cycles_prelu_otg, measured_flops_prelu_otg, perf_prelu_otg);

        // Legacy output for compatibility
        printf(
            "GEMM         cycles=%.0f, flops=%lld, performance=%.4f\n",
            cycles_gemm_basic, measured_flops_gemm, perf_gemm
        );
        printf(
            "TCSC_basic   cycles=%.0f, flops=%lld, performance=%.4f\n", 
            cycles_tsgemm_basic, measured_flops_basic, perf_basic
        );
        printf(
            "TCSC_opt     cycles=%.0f, flops=%lld, performance=%.4f\n", 
            cycles_tsgemm_opt, measured_flops_opt, perf_opt
        );
        printf(
            "TCSC_PReLU_basic cycles=%.0f, flops=%lld, performance=%.4f\n", 
            cycles_prelu_basic, measured_flops_prelu_basic, perf_prelu_basic
        );
        printf(
            "TCSC_PReLU_sep   cycles=%.0f, flops=%lld, performance=%.4f\n", 
            cycles_prelu_sep, measured_flops_prelu_sep, perf_prelu_sep
        );
        printf(
            "TCSC_PReLU_otg   cycles=%.0f, flops=%lld, performance=%.4f\n", 
            cycles_prelu_otg, measured_flops_prelu_otg, perf_prelu_otg
        );

        // Cleanup
        free(Y); free(refY); free(Y_prelu); free(refY_prelu); 
        free(Y_prelu_sep); free(Y_prelu_otg);
        free(W_dense); free(X); free(B);
        tcsc_free(W_tsparse);
        
        overall_progress.update(test_idx + 1);
        
        if (test_idx < basicTestCases.size() - 1) {
            ProgressBar::show_thinking_animation("Preparing next test case", 500);
        }
    }
    
    overall_progress.finish();
    
    cout << "\n*** ALL BENCHMARKS COMPLETED! ***\n";
    cout << "[*] Detailed results saved in out.txt\n";
    cout << "[*] Check the performance improvements from PReLU optimizations!\n";
    cout << "[*] Compare 'separate' vs 'on-the-go' PReLU approaches\n";
    cout << "========================================================================\n";

    return 0;
}
EOF

echo "=== main.cpp updated with PReLU optimization testing! ==="
echo ""
echo "New features in main.cpp:"
echo "1. Tests both PReLU optimization approaches"
echo "2. Validates all PReLU versions against each other"
echo "3. Comprehensive performance comparison table"
echo "4. Shows which PReLU approach is faster"
echo "5. Legacy output format maintained for compatibility"
echo ""
echo "PReLU parameter set to: 0.2f (configurable)"
echo ""
echo "Ready to test! Run: ./build_and_run_m1.sh"