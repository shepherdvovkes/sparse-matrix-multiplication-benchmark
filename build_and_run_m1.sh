#!/bin/bash

set -e  # Exit on any error

echo "=== Mac M1 Build and Run Script ==="

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

if ! command_exists make; then
    echo "❌ make not found. Installing Xcode Command Line Tools..."
    xcode-select --install
    exit 1
else
    echo "✓ make found"
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
    cp sparse/tcsc.c sparse/tcsc.c.backup 2>/dev/null || echo "✓ No tcsc.c to backup"
    cp sparse/tcsc.h sparse/tcsc.h.backup 2>/dev/null || echo "✓ No tcsc.h to backup"
    cp papi/my_papi.c papi/my_papi.c.backup 2>/dev/null || echo "✓ No my_papi.c to backup"
    echo "✓ Original files backed up"
fi

# Determine compilation flags
echo "=== Configuring build ==="

BASE_FLAGS="-O3 -ffast-math $ARCH_FLAG -std=c++17"
INCLUDE_FLAGS="-I."
SOURCE_FILES="main.cpp dense/dense.c sparse/tcsc.c"

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
    
    SOURCE_FILES="$SOURCE_FILES papi/my_papi.c"
    COMPILE_CMD="g++ $BASE_FLAGS $INCLUDE_FLAGS $PAPI_FLAGS $SOURCE_FILES $PAPI_LIBS -o tcsc_benchmark"
else
    echo "✓ Building without PAPI (using stub implementation)"
    COMPILE_CMD="g++ $BASE_FLAGS $INCLUDE_FLAGS -DDISABLE_PAPI $SOURCE_FILES papi/my_papi.c -o tcsc_benchmark"
fi

echo "Build command: $COMPILE_CMD"

# Clean previous builds
echo "=== Cleaning previous builds ==="
rm -f tcsc_benchmark
rm -f out.txt

# Compile
echo "=== Compiling ==="
if eval $COMPILE_CMD; then
    echo "✅ Compilation successful!"
else
    echo "❌ Compilation failed!"
    echo ""
    echo "Troubleshooting tips:"
    echo "1. Make sure Xcode Command Line Tools are installed: xcode-select --install"
    echo "2. If PAPI errors occur, try: brew install papi"
    echo "3. Check that all source files exist in the expected directories"
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
EOF

# Compile and run quick test
if [[ "$PAPI_AVAILABLE" == true ]]; then
    QUICK_TEST_CMD="g++ $BASE_FLAGS $INCLUDE_FLAGS $PAPI_FLAGS quick_test.cpp dense/dense.c sparse/tcsc.c papi/my_papi.c $PAPI_LIBS -o quick_test"
else
    QUICK_TEST_CMD="g++ $BASE_FLAGS $INCLUDE_FLAGS -DDISABLE_PAPI quick_test.cpp dense/dense.c sparse/tcsc.c papi/my_papi.c -o quick_test"
fi

if eval $QUICK_TEST_CMD && ./quick_test; then
    echo "✅ Validation test passed!"
    rm -f quick_test quick_test.cpp
else
    echo "❌ Validation test failed!"
    echo "Check your TCSC implementation for errors."
    rm -f quick_test quick_test.cpp
    exit 1
fi

# Run the benchmark
echo "=== Running benchmark ==="
echo "This may take several minutes..."
echo "Output will be saved to out.txt"

# Set CPU performance mode if possible (macOS specific)
if command_exists sudo; then
    echo "Attempting to set performance mode..."
    sudo pmset -a perfmode 1 2>/dev/null || echo "Could not set performance mode (requires admin)"
fi

# Run benchmark and capture output
echo "Starting benchmark run..."
if ./tcsc_benchmark | tee out.txt; then
    echo "✅ Benchmark completed successfully!"
    echo ""
    echo "=== Results Summary ==="
    echo "Output saved to: out.txt"
    echo ""
    echo "Key performance metrics:"
    grep -E "(TCSC_opt|Speedup)" out.txt | tail -10 || echo "No performance metrics found"
    echo ""
    echo "=== Next Steps ==="
    echo "1. Analyze the performance improvements in out.txt"
    echo "2. Run ./parse-out2csv.sh > out.csv to convert to CSV format"
    echo "3. Use python3 performance.py to generate performance plots"
    echo "4. Compare TCSC_basic vs TCSC_opt performance gains"
else
    echo "❌ Benchmark failed!"
    echo "Check out.txt for error details"
    exit 1
fi

# Restore performance mode
if command_exists sudo; then
    sudo pmset -a perfmode 0 2>/dev/null || echo "Could not restore power mode"
fi

echo ""
echo "🎉 Build and benchmark completed successfully!"
echo "Check out.txt for detailed results."