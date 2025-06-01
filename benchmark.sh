#!/bin/bash

if [[ $# -ne 1  ]]; then
    echo "usage: benchmark.sh program" >&2
    exit 2
fi

prog="$1"

# all of these step are taken from
# https://easyperf.net/blog/2019/08/02/Perf-measurement-environment-on-Linux#3-set-scaling_governor-to-performance

# disable intel turbo boost
echo '* disabling intel turbo boost'
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
# check with cat
echo 'no_turbo:' $(cat /sys/devices/system/cpu/intel_pstate/no_turbo)

# set scaling_governor to 'performance'
echo '* setting scaling_governor: performance'
for file in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo performance | sudo tee $file | sed 's/performance/./g' | tr '\n' ' '
done
echo ''

# disable hyper threading
echo '* disabling hyper threading'
for file in $(find /sys/devices/system/cpu -name online | grep -E 'cpu[0-9]+'); do
    echo 0 | sudo tee $file | sed 's/0/./g' | tr '\n' ' '
done
echo ''
# check with lscpu | grep list
lscpu | grep list

# run benchmark
sudo nice -n -50 taskset -c 0 ./"$prog" | tee out.txt

# disable intel turbo boost
echo '* enabling intel turbo boost'
echo 0 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
# check with cat
echo 'no_turbo:' $(cat /sys/devices/system/cpu/intel_pstate/no_turbo)

# disable hyper threading
echo '* enabling hyper threading'
for file in $(find /sys/devices/system/cpu -name online | grep -E 'cpu[0-9]+'); do
    echo 1 | sudo tee $file | sed 's/0/./g' | tr '\n' ' '
done
echo ''
# check with lscpu | grep list
lscpu | grep list

# set scaling_governor to 'powersafe'
echo '* setting scaling_governor: powersafe'
for file in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo powersafe | sudo tee $file | sed 's/performance/./g' | tr '\n' ' '
done
echo ''
