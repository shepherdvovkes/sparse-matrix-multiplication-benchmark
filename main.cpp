/*
 * This file is part of the Sparse Ternary Matrix Multiplication Project
 * of the Advanced Systems Lab course 2025 at ETH Zurich.
 *
 * Modified for TCSC testing on Mac M1 with Progress Bars!
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
    return (long long)M * (W->n_elem_pos + W->n_elem_neg) * 2 + (long long)M * N;
}

// Enhanced timing function with progress tracking
double measure_tcsc_cycles_with_progress(
    void (*func)(const dense_t, const tcsc_t*, const dense_t, dense_t, int, int, int),
    const dense_t X, const tcsc_t* W, const dense_t B, dense_t Y,
    int M, int N, int K, const string& func_name) {
    
    int i, num_runs = NUM_RUNS;
    double cycles = 0.;
    
    cout << "\n*** Measuring " << func_name << " performance..." << endl;
    
#ifdef __x86_64__
    myInt64 start, end;
#endif
#ifdef __aarch64__
    TIMESTAMP start, end;
#endif

#ifdef DO_WARMUP_BEFORE_MEASURING
    ProgressBar::show_thinking_animation("Warming up the CPU", 1000);
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
    
    cout << ">> Warmup complete! Running " << num_runs << " iterations per measurement." << endl;
#endif

    // Progress bar for actual measurements
    ProgressBar progress(REP, "[*] " + func_name + " measurements");
    
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
        
        progress.update(j + 1);
        this_thread::sleep_for(chrono::milliseconds(10)); // Small delay for visual effect
    }
    
    progress.finish();
    total_cycles /= REP;
    return total_cycles;
}

// Enhanced GEMM timing with progress
double measure_gemm_cycles_with_progress(
    void (*func)(const dense_t, const dense_t, const dense_t, dense_t, int, int, int),
    const dense_t X, const dense_t W, const dense_t B, dense_t Y,
    int M, int N, int K, const string& func_name) {
    
    cout << "\n*** Measuring " << func_name << " performance..." << endl;
    
    // Use the original measure_cycles but add some visual flair
    ProgressBar::show_thinking_animation("Setting up dense matrix multiplication", 800);
    
    double result = measure_cycles(func, X, W, B, Y, M, N, K);
    
    cout << "*** " << func_name << " measurement complete!" << endl;
    return result;
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
    cout << ">> Testing sparse matrix multiplication with C optimizations on Mac M1\n";
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
                         double cycles_opt, long long flops_opt, double perf_opt) {
    
    cout << "\n[*] PERFORMANCE RESULTS:\n";
    cout << "+---------------+-------------+-------------+-------------+\n";
    cout << "|   Algorithm   |   Cycles    |    FLOPs    | Performance |\n";
    cout << "+---------------+-------------+-------------+-------------+\n";
    cout << "| Dense GEMM    |" << setw(12) << (long long)cycles_gemm << " |" 
         << setw(12) << flops_gemm << " |" << setw(11) << fixed << setprecision(4) << perf_gemm << " |\n";
    cout << "| TCSC Basic    |" << setw(12) << (long long)cycles_basic << " |" 
         << setw(12) << flops_basic << " |" << setw(11) << fixed << setprecision(4) << perf_basic << " |\n";
    cout << "| TCSC Optimized|" << setw(12) << (long long)cycles_opt << " |" 
         << setw(12) << flops_opt << " |" << setw(11) << fixed << setprecision(4) << perf_opt << " |\n";
    cout << "+---------------+-------------+-------------+-------------+\n";
    
    double speedup_basic = cycles_gemm / cycles_basic;
    double speedup_opt_vs_basic = cycles_basic / cycles_opt;
    double speedup_overall = cycles_gemm / cycles_opt;
    
    cout << "\n[*] SPEEDUP ANALYSIS:\n";
    cout << "  [1] TCSC Basic vs Dense:     " << fixed << setprecision(2) << speedup_basic << "x faster\n";
    cout << "  [2] Optimized vs Basic:      " << fixed << setprecision(2) << speedup_opt_vs_basic << "x faster\n";
    cout << "  [3] Overall Optimization:    " << fixed << setprecision(2) << speedup_overall << "x faster\n";
    
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
        {   1, 4096, 16384},
        { 256,  512,  2048},
        { 256, 1024,  4096},
        { 256, 2048,  8192},
    };

    ProgressBar overall_progress(basicTestCases.size(), "[*] Overall Benchmark Progress");
    
    for (size_t test_idx = 0; test_idx < basicTestCases.size(); test_idx++) {
        const auto& [M_ROW, K_LEN, N_COL] = basicTestCases[test_idx];
        
        print_test_case_header(test_idx + 1, basicTestCases.size(), M_ROW, K_LEN, N_COL);
        
        ProgressBar::show_thinking_animation("Allocating and initializing matrices", 1000);

        dense_elem_t *Y, *refY;
        const dense_t W_dense = init_rand_sparse(K_LEN, N_COL, 2);
        const dense_t X = init_rand_dense(M_ROW, K_LEN);
        const dense_t B = init_rand_dense(N_COL, 1);

        build_and_check(&Y, M_ROW, N_COL);
        build_and_check(&refY, M_ROW, N_COL);

        ProgressBar::show_thinking_animation("Converting to TCSC sparse format", 800);
        tcsc_t *W_tsparse = tcsc_from_dense(W_dense, K_LEN, N_COL);

        // Calculate FLOP counts
        long long flops_gemm_basic = 2LL * M_ROW * N_COL * K_LEN + M_ROW * N_COL;
        long long flops_sparse = calculate_sparse_flops(W_tsparse, M_ROW, N_COL);

        cout << "[*] Matrix info: " << W_tsparse->n_elem_pos + W_tsparse->n_elem_neg 
             << " non-zeros out of " << K_LEN * N_COL << " elements\n";

        // Validation phase
        cout << "\n[*] Running validation tests...\n";
        
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

        cout << "[OK] All validation tests passed!\n";

        // Performance measurements
        cout << "\n[*] Starting performance measurements...\n";
        
        build_and_check(&Y, M_ROW, N_COL);
        build_and_check(&refY, M_ROW, N_COL);

        double cycles_gemm_basic = measure_gemm_cycles_with_progress(
            gemm_basic, X, W_dense, B, refY, M_ROW, N_COL, K_LEN, "Dense GEMM");
        
        build_and_check(&Y, M_ROW, N_COL);
        double cycles_tsgemm_basic = measure_tcsc_cycles_with_progress(
            tcsc_sgemm_basic, X, W_tsparse, B, Y, M_ROW, N_COL, K_LEN, "TCSC Basic");
        
        build_and_check(&Y, M_ROW, N_COL);
        double cycles_tsgemm_opt = measure_tcsc_cycles_with_progress(
            tcsc_sgemm_optimized, X, W_tsparse, B, Y, M_ROW, N_COL, K_LEN, "TCSC Optimized");

        // Calculate performance metrics
        double perf_gemm = (double)measured_flops_gemm / cycles_gemm_basic;
        double perf_basic = (double)measured_flops_basic / cycles_tsgemm_basic;
        double perf_opt = (double)measured_flops_opt / cycles_tsgemm_opt;

        print_results_table(cycles_gemm_basic, measured_flops_gemm, perf_gemm,
                           cycles_tsgemm_basic, measured_flops_basic, perf_basic,
                           cycles_tsgemm_opt, measured_flops_opt, perf_opt);

        // Legacy output for compatibility
        printf(
            "GEMM       cycles=%.0f, flops=%lld, performance=%.4f\n",
            cycles_gemm_basic, measured_flops_gemm, perf_gemm
        );
        printf(
            "TCSC_basic cycles=%.0f, flops=%lld, performance=%.4f\n", 
            cycles_tsgemm_basic, measured_flops_basic, perf_basic
        );
        printf(
            "TCSC_opt   cycles=%.0f, flops=%lld, performance=%.4f\n", 
            cycles_tsgemm_opt, measured_flops_opt, perf_opt
        );

        // Cleanup
        free(Y); free(refY); free(W_dense); free(X); free(B);
        tcsc_free(W_tsparse);
        
        overall_progress.update(test_idx + 1);
        
        if (test_idx < basicTestCases.size() - 1) {
            ProgressBar::show_thinking_animation("Preparing next test case", 500);
        }
    }
    
    overall_progress.finish();
    
    cout << "\n*** ALL BENCHMARKS COMPLETED! ***\n";
    cout << "[*] Detailed results saved in out.txt\n";
    cout << "[*] Check the performance improvements from C optimizations!\n";
    cout << "========================================================================\n";

    return 0;
}
