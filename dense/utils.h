#pragma once
#include <random>
#include <math.h>
#include "dense.h"

/*
 * Generate random numbers in [-1, +1)
 */
template<typename T>
void rands_dense(T *m, int rows, int cols) {
    std::random_device rd;
    std::mt19937 gen{rd()};
    std::uniform_real_distribution<T> dist(-1.0, 1.0);
    for (size_t i = 0; i < rows * cols; ++i)
        m[i] = dist(gen);
}

// TODO: OBSOLETE CODE TO BE DELETED
// /*
//  * Rounds float to n_digits-th digit after decimal point.
//  * This serves as helper to rands_sparse
//  */
// float round_nth(float val, int n_digits) {
//     float exp = pow(10, n_digits);
//     float near = roundf(val * exp) / exp;
//     return near;
// }

/*
 * Generates random numbers in {-1, 0 +1} with non-uniform probabilities
 * as defined by the parameter non_zero. Meaning
 *  prob(-1) = 1 / (2 * non_zero)
 *  prob(0)  = 1 - 1 / non_zero
 *  prob(+1) = 1 / (2 * non_zero)
 */
template<typename T>
void rands_sparse(T *m, int rows, int cols, int non_zero) {

    // probability({+1, -1}) = 1 / non_zero
    // probability({0})      = 1 - (1 / non_zero) = (non_zero - 1) / non_zero
    float p_zero, p_one;
    p_zero = (float) (non_zero - 1) / non_zero;
    p_one  = 1. / (2 * non_zero);

    // TODO: OBSOLETE CODE TO BE DELETED
    // // round to 4 digits after decimal point and multiply by 1e4 to get a whole
    // // number. This is used to adjust the probabilities of the discrete
    // // distribution.
    // // Example:
    // //  from probability to integer: 0.12344 -> 0.1234 -> 1234
    // //  (where the first -> is rounding and the second is mult by exp)
    // int n_digits = 4;
    // float exp = pow(10, n_digits);
    // p_zero = round_nth(p_zero, n_digits);
    // p_one = round_nth(p_one, n_digits);

    std::random_device rd;
    std::mt19937 gen{rd()};
    std::discrete_distribution<> dist({
        // probabilities for drawing the elements 0, 1, 2
        // represented as integers in the correct ratio to one another
        //(int) (p_one * exp), (int) (p_zero * exp), (int) (p_one * exp)
        p_one, p_zero, p_one
    });

    for (size_t i = 0; i < rows * cols; ++i)
        m[i] = dist(gen) - 1.0; // subtract offset to get elements in -1, 0, +1
}
