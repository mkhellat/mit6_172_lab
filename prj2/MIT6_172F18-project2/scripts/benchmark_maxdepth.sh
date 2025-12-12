#!/bin/bash
# Benchmark script to test different maxDepth values for quadtree
# Usage: ./benchmark_maxdepth.sh [input_file] [frames]

INPUT_FILE="${1:-input/beaver.in}"
FRAMES="${2:-100}"

echo "=========================================="
echo "Quadtree maxDepth Benchmark"
echo "=========================================="
echo "Input: $INPUT_FILE"
echo "Frames: $FRAMES"
echo ""

# Test different maxDepth values
for maxDepth in 5 8 10 12 15 18 20 25; do
  echo "Testing maxDepth=$maxDepth..."
  
  # Time the execution
  start_time=$(date +%s.%N)
  QUADTREE_MAXDEPTH=$maxDepth ./screensaver -q "$FRAMES" "$INPUT_FILE" > /tmp/qt_${maxDepth}.out 2>&1
  end_time=$(date +%s.%N)
  
  elapsed=$(echo "$end_time - $start_time" | bc)
  collisions=$(grep -oP '\d+ Line-Line Collisions' /tmp/qt_${maxDepth}.out | grep -oP '^\d+' || echo "0")
  
  printf "  Time: %.4fs\n" "$elapsed"
  echo "  Collisions: $collisions"
  echo ""
done

echo "=========================================="
echo "Benchmark complete"
echo "=========================================="
