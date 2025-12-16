#!/bin/bash
# Script to analyze extra pairs found only in parallel execution
# Identifies which worker found each extra pair and traces why

set -e

INPUT_FILE="${1:-input/smalllines.in}"
NUM_FRAMES="${2:-1}"
NUM_WORKERS="${3:-4}"
EXAMPLE_PAIR="${4:-}"  # Optional: specific pair to trace (e.g., "2,8")

echo "=== Analyzing Extra Pairs (Only in Parallel) ==="
echo "Input file: $INPUT_FILE"
echo "Number of frames: $NUM_FRAMES"
echo "Number of workers: $NUM_WORKERS"
if [ -n "$EXAMPLE_PAIR" ]; then
  echo "Example pair to trace: $EXAMPLE_PAIR"
fi
echo ""

# Create temporary directories
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

SERIAL_OUT="$TMPDIR/serial.out"
PARALLEL_OUT="$TMPDIR/parallel.out"

echo "Running serial execution..."
CILK_NWORKERS=1 ./screensaver -q $NUM_FRAMES "$INPUT_FILE" 2>&1 | tee "$SERIAL_OUT" > /dev/null

echo "Running parallel execution..."
CILK_NWORKERS=$NUM_WORKERS ./screensaver -q $NUM_FRAMES "$INPUT_FILE" 2>&1 | tee "$PARALLEL_OUT" > /dev/null

echo ""
echo "=== Extracting and Comparing Pairs ==="

# Extract pairs for frame 1
FRAME=1

# Serial pairs - normalize and sort properly
grep "DEBUG_PAIR_ADDED.*FRAME=$FRAME" "$SERIAL_OUT" | \
  sed 's/.*PAIR=\([0-9]*\),\([0-9]*\).*/\1 \2/' | \
  awk '{if ($1 < $2) print sprintf("%010d,%010d", $1, $2); else print sprintf("%010d,%010d", $2, $1)}' | \
  sort -u | \
  awk -F, '{printf "%d,%d\n", $1+0, $2+0}' > "$TMPDIR/serial_frame_${FRAME}.txt"

# Parallel pairs with worker info
grep "DEBUG_PAIR_ADDED.*FRAME=$FRAME" "$PARALLEL_OUT" > "$TMPDIR/parallel_pairs_raw.txt"

# Extract normalized pairs - normalize and sort properly
grep "DEBUG_PAIR_ADDED.*FRAME=$FRAME" "$PARALLEL_OUT" | \
  sed 's/.*PAIR=\([0-9]*\),\([0-9]*\).*/\1 \2/' | \
  awk '{if ($1 < $2) print sprintf("%010d,%010d", $1, $2); else print sprintf("%010d,%010d", $2, $1)}' | \
  sort -u | \
  awk -F, '{printf "%d,%d\n", $1+0, $2+0}' > "$TMPDIR/parallel_frame_${FRAME}.txt"

# Find extra pairs (only in parallel) - use grep instead of comm to avoid sorting issues
grep -v -f "$TMPDIR/serial_frame_${FRAME}.txt" "$TMPDIR/parallel_frame_${FRAME}.txt" > "$TMPDIR/extra_pairs.txt" || true

EXTRA_COUNT=$(wc -l < "$TMPDIR/extra_pairs.txt" | tr -d ' ')
echo "Found $EXTRA_COUNT extra pairs (only in parallel)"
echo ""

if [ $EXTRA_COUNT -eq 0 ]; then
  echo "No extra pairs found!"
  exit 0
fi

echo "=== Extra Pairs by Worker ==="
echo ""

# For each extra pair, find which worker(s) found it
declare -A worker_counts
declare -A pair_workers

while IFS= read -r pair; do
  id1=$(echo $pair | cut -d, -f1)
  id2=$(echo $pair | cut -d, -f2)
  
  # Find all occurrences of this pair in parallel output (could be in either order)
  grep "DEBUG_PAIR_ADDED.*FRAME=$FRAME" "$PARALLEL_OUT" | \
    grep -E "PAIR=$id1,$id2|PAIR=$id2,$id1" | \
    sed 's/.*WORKER=\([0-9]*\).*/\1/' | \
    sort -u | while read worker; do
      if [ -n "$worker" ]; then
        worker_counts[$worker]=$((${worker_counts[$worker]:-0} + 1))
        pair_workers["$pair"]="${pair_workers[$pair]} $worker"
      fi
    done
done < "$TMPDIR/extra_pairs.txt"

# Show worker statistics
echo "Worker distribution of extra pairs:"
for worker in $(printf '%s\n' "${!worker_counts[@]}" | sort -n); do
  count=${worker_counts[$worker]}
  echo "  Worker $worker: $count extra pairs"
done
echo ""

# Show first 20 extra pairs with their workers
echo "=== First 20 Extra Pairs with Worker Info ==="
head -20 "$TMPDIR/extra_pairs.txt" | while read pair; do
  id1=$(echo $pair | cut -d, -f1)
  id2=$(echo $pair | cut -d, -f2)
  
  echo "Pair $pair:"
  # Find worker and full details - new format: FRAME=1 WORKER=X PAIR=id1,id2 line1_idx=X line2_idx=X myIndex=X
  grep "DEBUG_PAIR_ADDED.*FRAME=$FRAME" "$PARALLEL_OUT" | \
    grep -E "PAIR=$id1,$id2|PAIR=$id2,$id1" | head -1 | \
    sed 's/DEBUG_PAIR_ADDED: //' | \
    awk '{
      for(i=1;i<=NF;i++) {
        if ($i ~ /^WORKER=/) {worker=substr($i,8)}
        if ($i ~ /^line1_idx=/) {line1_idx=substr($i,11)}
        if ($i ~ /^line2_idx=/) {line2_idx=substr($i,11)}
        if ($i ~ /^myIndex=/) {myIndex=substr($i,8)}
      }
      printf "  Worker: %s, line1_idx: %s, line2_idx: %s, myIndex: %s\n", worker, line1_idx, line2_idx, myIndex
    }'
done
echo ""

# If example pair specified, do detailed trace
if [ -n "$EXAMPLE_PAIR" ]; then
  id1=$(echo $EXAMPLE_PAIR | cut -d, -f1)
  id2=$(echo $EXAMPLE_PAIR | cut -d, -f2)
  
  echo "=== Detailed Trace for Pair $EXAMPLE_PAIR ==="
  echo ""
  
  # Check if it's in serial
  if grep -q "^$id1,$id2$\|^$id2,$id1$" "$TMPDIR/serial_frame_${FRAME}.txt"; then
    echo "Pair $EXAMPLE_PAIR is in SERIAL execution"
  else
    echo "Pair $EXAMPLE_PAIR is NOT in serial execution (extra pair)"
  fi
  
  # Check if it's in parallel
  if grep -q "^$id1,$id2$\|^$id2,$id1$" "$TMPDIR/parallel_frame_${FRAME}.txt"; then
    echo "Pair $EXAMPLE_PAIR is in PARALLEL execution"
    echo ""
    echo "Parallel execution details:"
    grep "DEBUG_PAIR_ADDED.*FRAME=$FRAME" "$PARALLEL_OUT" | \
      grep -E "PAIR=$id1,$id2|PAIR=$id2,$id1" | head -5
  else
    echo "Pair $EXAMPLE_PAIR is NOT in parallel execution"
  fi
  
  echo ""
  echo "=== Why was this pair found in parallel but not serial? ==="
  echo ""
  echo "Possible reasons:"
  echo "1. Different processing order in parallel (line1 processed by different worker)"
  echo "2. Atomic operation on seenPairs allowed it through in parallel"
  echo "3. Order-dependent check was skipped in parallel"
  echo "4. Spatial query found it in parallel but not serial"
  echo ""
  echo "To investigate further, check:"
  echo "- Which line indices (line1_idx, line2_idx) were used"
  echo "- Which worker processed line1"
  echo "- Whether seenPairs atomic operation succeeded"
  echo "- Whether order check would have filtered it in serial"
fi

# Show summary
echo ""
echo "=== Summary ==="
echo "Total extra pairs: $EXTRA_COUNT"
echo "These pairs are found in parallel but NOT in serial"
echo "This suggests a logic flaw where parallel finds pairs that should be filtered"
echo ""
echo "Next steps:"
echo "1. Pick a specific extra pair (e.g., from the list above)"
echo "2. Trace why serial didn't find it (check order, seenPairs, etc.)"
echo "3. Trace why parallel did find it (check worker, atomic operations)"
echo "4. Identify the logic flaw"

