#ifndef MY_PAPI_H
#define MY_PAPI_H

void init_papi();
void start_flop_count();
long long stop_flop_count();
void destroy_papi();

#ifdef DISABLE_PAPI
void set_flop_count(long long flops);
#endif

#endif
