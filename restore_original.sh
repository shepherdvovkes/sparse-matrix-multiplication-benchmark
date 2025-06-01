#!/bin/bash

echo "=== Restoring Original Files ==="

# Function to restore a file if backup exists
restore_file() {
    local file="$1"
    local backup="${file}.backup"
    
    if [[ -f "$backup" ]]; then
        echo "Restoring $file from backup..."
        cp "$backup" "$file"
        rm "$backup"
        echo "✓ $file restored"
    else
        echo "⚠️  No backup found for $file"
    fi
}

# Restore all modified files
restore_file "main.cpp"
restore_file "sparse/tcsc.c"
restore_file "sparse/tcsc.h"
restore_file "papi/my_papi.c"
restore_file "papi/my_papi.h"

# Clean up build artifacts
echo "=== Cleaning Build Artifacts ==="
rm -f tcsc_benchmark
rm -f quick_test
rm -f out.txt
rm -f out.csv

echo "✅ All files restored to original state"
echo "You can now re-run patch_for_m1.sh if needed"