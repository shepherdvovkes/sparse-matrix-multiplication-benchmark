#!/bin/bash

set -e  # Exit on any error

echo "=== Mac M1 Build and Run Script (Fixed) ==="

# Check if we're on macOS
if [[ $(uname) != "Darwin" ]]; then
    echo "Warning: This script is designed for macOS. You may need to adjust compiler flags."
fi

# Check if we're on Apple Silicon
if [[ $(uname -m) == "arm64" ]]; then
    echo "✓ Detected Apple Silicon (M1/M2/M3)"
    ARCH_FLAG="-mcpu=apple-m1"
else
    echo "✓ Detected Intel Mac"
    ARCH_FLAG="-march=native"
fi

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for required tools
echo "=== Checking build environment ==="

if ! command_exists g++; then
    echo "❌ g++ not found. Installing Xcode Command Line Tools..."
    xcode-select --install
    echo "Please run this script again after Xcode Command Line Tools installation completes."
    exit 1
else
    echo "✓ g++ found: $(g++ --version | head -n1)"
fi

if ! command_exists gcc; then
    echo "❌ gcc not found. Installing Xcode Command Line Tools..."
    xcode-select --install
    exit 1
else
    echo "✓ gcc found: $(gcc --version | head -n1)"
fi

# Check for PAPI (optional)
PAPI_AVAILABLE=false
if command_exists pkg-config && pkg-config --exists papi; then
    echo "✓ PAPI found via pkg-config"
    PAPI_AVAILABLE=true
elif [[ -f /opt/homebrew/lib/libpapi.dylib ]] || [[ -f /usr/local/lib/libpapi.dylib ]]; then
    echo "✓ PAPI library found"
    PAPI_AVAILABLE=true
elif [[ -f /opt/homebrew/include/papi.h ]] || [[ -f /usr/local/include/papi.h ]]; then
    echo "✓ PAPI headers found"
    PAPI_AVAILABLE=true
else
    echo "⚠️  PAPI not found. Using stub implementation."
    echo "   To install PAPI: brew install papi"
fi

# Backup original files if they exist
echo "=== Backing up original files ==="
if [[ -f main.cpp.backup ]]; then
    echo "✓ Backup already exists"
else
    cp main.cpp main.cpp.backup 2>/dev/null || echo "✓ No main.cpp to backup"
    echo "✓ Original files backed up"
fi

# Determine compilation flags
echo "=== Configuring build ==="

BASE_FLAGS_C="-O3 -ffast-math $ARCH_FLAG"
BASE_FLAGS_CXX="-O3 -ffast-math $ARCH_FLAG -std=c++17"
INCLUDE_FLAGS="-I."

if [[ "$PAPI_AVAILABLE" == true ]]; then
    echo "✓ Building with PAPI support"
    PAPI_FLAGS=""
    PAPI_LIBS="-lpapi"
    
    # Try to find PAPI paths
    if [[ -d /opt/homebrew/include ]]; then
        INCLUDE_FLAGS="$INCLUDE_FLAGS -I/opt/homebrew/include"
        PAPI_FLAGS="$PAPI_FLAGS -L/opt/homebrew/lib"
    elif [[ -d /usr/local/include ]]; then
        INCLUDE_FLAGS="$INCLUDE_FLAGS -I/usr/local/include"
        PAPI_FLAGS="$PAPI_FLAGS -L/usr/local/lib"
    fi
    
    COMPILE_PAPI_CMD="gcc $BASE_FLAGS_C $INCLUDE_FLAGS -c papi/my_papi.c -o papi/my_papi.o"
    LINK_PAPI_LIBS="papi/my_papi.o $PAPI_FLAGS $PAPI_LIBS"
else
    echo "✓ Building without PAPI (using stub implementation)"
    COMPILE_PAPI_CMD="gcc $BASE_FLAGS_C $INCLUDE_FLAGS -DDISABLE_PAPI -c papi/my_papi.c -o papi/my_papi.o"
    LINK_PAPI_LIBS="papi/my_papi.o"
fi

# Individual compilation commands
COMPILE_DENSE_CMD="gcc $BASE_FLAGS_C $INCLUDE_FLAGS -c dense/dense.c -o dense/dense.o"
COMPILE_TCSC_CMD="gcc $BASE_FLAGS_C $INCLUDE_FLAGS -c sparse/tcsc.c -o sparse/tcsc.o"
COMPILE_MAIN_CMD="g++ $BASE_FLAGS_CXX $INCLUDE_FLAGS -c main.cpp -o main.o"
LINK_CMD="g++ $BASE_FLAGS_CXX main.o dense/dense.o sparse/tcsc.o $LINK_PAPI_LIBS -o tcsc_benchmark"

echo "Compile commands:"
echo "  Dense: $COMPILE_DENSE_CMD"
echo "  TCSC:  $COMPILE_TCSC_CMD"
echo "  PAPI:  $COMPILE_PAPI_CMD"
echo "  Main:  $COMPILE_MAIN_CMD"
echo "  Link:  $LINK_CMD"

# Clean previous builds
echo "=== Cleaning previous builds ==="
rm -f tcsc_benchmark
rm -f out.txt
rm -f *.o dense/*.o sparse/*.o papi/*.o

# Compile step by step
echo "=== Compiling ==="

echo "Compiling dense.c..."
if eval $COMPILE_DENSE_CMD; then
    echo "✓ dense.c compiled successfully"
else
    echo "❌ Failed to compile dense.c"
    exit 1
fi

echo "Compiling tcsc.c..."
if eval $COMPILE_TCSC_CMD; then
    echo "✓ tcsc.c compiled successfully"
else
    echo "❌ Failed to compile tcsc.c"
    exit 1
fi

echo "Compiling my_papi.c..."
if eval $COMPILE_PAPI_CMD; then
    echo "✓ my_papi.c compiled successfully"
else
    echo "❌ Failed to compile my_papi.c"
    exit 1
fi

echo "Compiling main.cpp..."
if eval $COMPILE_MAIN_CMD; then
    echo "✓ main.cpp compiled successfully"
else
    echo "❌ Failed to compile main.cpp"
    exit 1
fi

echo "Linking..."
if eval $LINK_CMD; then
    echo "✅ Compilation successful!"
else
    echo "❌ Linking failed!"
    exit 1
fi

# Verify binary
if [[ ! -f tcsc_benchmark ]]; then
    echo "❌ Binary not created!"
    exit 1
fi

echo "✓ Binary created: tcsc_benchmark"
echo "Binary size: $(du -h tcsc_benchmark | cut -f1)"

# Run quick validation test
echo "=== Running validation test ==="
echo "Testing with small matrices to verify correctness..."

# Create a simple test
cat > quick_test.cpp << 'EOF'
#include <iostream>
#include "dense/dense.h"
#include "sparse/tcsc.h"
#include "papi/my_papi.h"

int main() {
    init_papi();
    
    // Small test case
    int M = 2, K = 4, N = 4;
    
    dense_t X = init_rand_dense(M, K);
    dense_t W_dense = init_rand_sparse(K, N, 2);
    dense_t B = init_rand_dense(N, 1);
    dense_t Y1 = (dense_t)aligned_alloc(32, M * N * sizeof(dense_elem_t));
    dense_t Y2 = (dense_t)aligned_alloc(32, M * N * sizeof(dense_elem_t));
    
    tcsc_t *W_sparse = tcsc_from_dense(W_dense, K, N);
    
    gemm_basic(X, W_dense, B, Y1, M, N, K);
    tcsc_sgemm_basic(X, W_sparse, B, Y2, M, N, K);
    
    bool passed = compare(Y1, Y2, M, N);
    
    std::cout << "Quick validation test: " << (passed ? "PASSED" : "FAILED") << std::endl;
    
    free(X); free(W_dense); free(B); free(Y1); free(Y2);
    tcsc_free(W_sparse);
    
    return passed ? 0 : 1;
}
