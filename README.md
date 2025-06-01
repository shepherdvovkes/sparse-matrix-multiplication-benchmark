# Sparse Ternary Matrix Multiplication

## Workflow

This workflow was tested successfully on linux.

1. Compile code for your machine, e.g. `g++ -O3 -ffast-math -march=native main.cpp dense/dense.c sparse/bcsr.c papi/my_papi.c sparse/tcsc.c -lpapi`
2. Run the benchmark, e.g.  `./benchmark.sh run`
3. Convert output stored in `out.txt` to csv format, e.g. `./parse-out2csv.sh > out.csv`
4. Plot the performance, e.g. `python3 performance.py`
