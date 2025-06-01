#ifdef DISABLE_PAPI
// Stub implementation for systems without PAPI (Mac M1)
#include <stdio.h>
#include "my_papi.h"

static long long flop_counter = 0;

void init_papi() {
    printf("PAPI disabled - using stub implementation for Mac M1\n");
}

void start_flop_count() {
    flop_counter = 0;
}

long long stop_flop_count() {
    return flop_counter;
}

void destroy_papi() {
    // Nothing to do
}

// Helper function to set FLOP count manually
void set_flop_count(long long flops) {
    flop_counter = flops;
}

#else
// Original PAPI implementation for Intel systems
#include <papi.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_papi.h"

void handle_error (int retval){
    printf("PAPI error %d: %s\n", retval, PAPI_strerror(retval));
    exit(1);
}

int EventSet = PAPI_NULL;

void init_papi(){
    int retval = PAPI_NULL;
	
    /* Initialize the PAPI library */
    retval = PAPI_library_init(PAPI_VER_CURRENT);
    if (retval != PAPI_VER_CURRENT){
        handle_error(retval);
    }

    /* Create an EventSet */
    retval = PAPI_create_eventset(&EventSet);
    if (retval != PAPI_OK){
        handle_error(retval);
    }

    /* Add Total Instructions Executed to our EventSet */
    retval = PAPI_add_event(EventSet, PAPI_SP_OPS);
    if (retval != PAPI_OK){
        handle_error(retval);
    }
}

void start_flop_count() {
    int retval = PAPI_start(EventSet);
    if (retval != PAPI_OK) {
        handle_error(retval);
    }
}

long long stop_flop_count() {
    long long flops = 0;
    int retval = PAPI_stop(EventSet, &flops);
    if (retval != PAPI_OK) {
        handle_error(retval);
    }

    retval = PAPI_reset(EventSet);
    return flops;
}

void destroy_papi(){
    PAPI_cleanup_eventset(EventSet);  
    PAPI_destroy_eventset(&EventSet);  
    PAPI_shutdown(); 
}
#endif
