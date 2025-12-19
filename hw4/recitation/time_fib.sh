#!/bin/bash
# Timing script for fib program with different Cilk worker counts

if [ $# -lt 1 ]; then
    echo "Usage: $0 <num_workers> [n]"
    echo "Example: $0 1 45"
    exit 1
fi

WORKERS=$1
N=${2:-45}

echo "=== Testing with $WORKERS Cilk worker(s) ==="
export CILK_NWORKERS=$WORKERS

# Use date to measure time
START=$(date +%s.%N)
./fib $N
END=$(date +%s.%N)

ELAPSED=$(echo "$END - $START" | bc)
echo "Elapsed time: ${ELAPSED} seconds"

