#pragma once

#include "dense/dense.h"

// function pointer for (sparse) gemm computations
typedef void (*gemm_func)(
    const dense_t X, const void *W, const dense_t B, dense_t Y,
    int M, int N, int K
);
// function pointer for (sparse) gemm prelu computations
typedef void (*prelu_func)(
    const dense_t X, const void *W, const dense_t B, float a, dense_t Y,
    int M, int N, int K
);

template <typename FuncType>
void add_func(FuncType f, std::string name);
