#ifdef __x86_64__
#include "tsc_x86.h"
#endif

#ifdef __aarch64__
#include "vct_arm.h"

#ifdef PMU
#include "kperf.h"
#endif
#endif

template<typename... Args>
double measure_cycles(void (*func)(Args...), Args... args) {
    int i, num_runs = NUM_RUNS;
    double cycles = 0.;
#ifdef __x86_64__
    myInt64 start, end;
#endif
#ifdef __aarch64__
    TIMESTAMP start, end;
#endif

#ifdef DO_WARMUP_BEFORE_MEASURING
    // Warm-up phase: we determine a number of executions that allows
    // the code to be executed for at least CYCLES_REQUIRED cycles.
    // This helps excluding timing overhead when measuring small runtimes.
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
            func(args...);
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

    // Actual performance measurements repeated REP times.
    // We simply store all results and compute medians during post-processing.
    double total_cycles = 0.;
    for (int j = 0; j < REP; j++) {
#ifdef __x86_64__
        start = start_tsc();
#endif
#ifdef __aarch64__
        start = start_vct();
#endif
        for (i = 0; i < num_runs; ++i) {
            func(args...);
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
