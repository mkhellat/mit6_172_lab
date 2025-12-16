#!/bin/bash
# Script to trace a specific pair through serial and parallel execution
# Shows why it was found/missed in each execution

set -e

PAIR="${1:-2,8}"
INPUT_FILE="${2:-input/smalllines.in}"

echo "=== Tracing Pair $PAIR ==="
echo ""

id1=$(echo $PAIR | cut -d, -f1)
id2=$(echo $PAIR | cut -d, -f2)

# Normalize pair (ensure id1 < id2)
if [ $id1 -gt $id2 ]; then
  temp=$id1
  id1=$id2
  id2=$temp
fi
PAIR_NORM="$id1,$id2"

TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

echo "Running serial execution..."
CILK_NWORKERS=1 ./screensaver -q 1 "$INPUT_FILE" 2>&1 > "$TMPDIR/serial.out"

echo "Running parallel execution..."
CILK_NWORKERS=4 ./screensaver -q 1 "$INPUT_FILE" 2>&1 > "$TMPDIR/parallel.out"

echo ""
echo "=== Serial Execution Analysis ==="
SERIAL_FOUND=$(grep "DEBUG_PAIR_ADDED.*FRAME=1.*PAIR=$id1,$id2\|DEBUG_PAIR_ADDED.*FRAME=1.*PAIR=$id2,$id1" "$TMPDIR/serial.out" | wc -l)
if [ $SERIAL_FOUND -gt 0 ]; then
  echo "✅ Pair $PAIR_NORM FOUND in serial execution ($SERIAL_FOUND times)"
  echo ""
  echo "Details:"
  grep "DEBUG_PAIR_ADDED.*FRAME=1.*PAIR=$id1,$id2\|DEBUG_PAIR_ADDED.*FRAME=1.*PAIR=$id2,$id1" "$TMPDIR/serial.out" | head -3
else
  echo "❌ Pair $PAIR_NORM NOT FOUND in serial execution"
  echo ""
  echo "Why might it be missing?"
  echo "- Order check filtered it (line2_idx <= line1_idx)"
  echo "- seenPairs already marked it"
  echo "- Spatial query didn't find it"
  echo "- Bounds check filtered it"
fi

echo ""
echo "=== Parallel Execution Analysis ==="
PARALLEL_FOUND=$(grep "DEBUG_PAIR_ADDED.*FRAME=1.*PAIR=$id1,$id2\|DEBUG_PAIR_ADDED.*FRAME=1.*PAIR=$id2,$id1" "$TMPDIR/parallel.out" | wc -l)
if [ $PARALLEL_FOUND -gt 0 ]; then
  echo "✅ Pair $PAIR_NORM FOUND in parallel execution ($PARALLEL_FOUND times)"
  echo ""
  echo "Details:"
  grep "DEBUG_PAIR_ADDED.*FRAME=1.*PAIR=$id1,$id2\|DEBUG_PAIR_ADDED.*FRAME=1.*PAIR=$id2,$id1" "$TMPDIR/parallel.out" | head -5 | \
    awk '{
      for(i=1;i<=NF;i++) {
        if ($i ~ /^WORKER=/) {worker=substr($i,8)}
        if ($i ~ /^line1_idx=/) {line1_idx=substr($i,11)}
        if ($i ~ /^line2_idx=/) {line2_idx=substr($i,11)}
        if ($i ~ /^myIndex=/) {myIndex=substr($i,8)}
      }
      printf "  Worker: %s, line1_idx: %s, line2_idx: %s, myIndex: %s\n", worker, line1_idx, line2_idx, myIndex
    }'
  
  echo ""
  echo "Which worker(s) found it:"
  grep "DEBUG_PAIR_ADDED.*FRAME=1.*PAIR=$id1,$id2\|DEBUG_PAIR_ADDED.*FRAME=1.*PAIR=$id2,$id1" "$TMPDIR/parallel.out" | \
    sed 's/.*WORKER=\([0-9]*\).*/\1/' | sort -u | tr '\n' ' ' && echo ""
else
  echo "❌ Pair $PAIR_NORM NOT FOUND in parallel execution"
fi

echo ""
echo "=== Comparison ==="
if [ $SERIAL_FOUND -eq 0 ] && [ $PARALLEL_FOUND -gt 0 ]; then
  echo "🔍 EXTRA PAIR: Found in parallel but NOT in serial"
  echo ""
  echo "This is a logic flaw! Why did parallel find it?"
  echo ""
  echo "Possible reasons:"
  echo "1. Different processing order - line1 processed by worker before serial would process it"
  echo "2. Atomic seenPairs operation allowed it through (race condition)"
  echo "3. Order check was removed/not working in parallel"
  echo "4. Spatial query found it in parallel due to different traversal order"
  echo ""
  echo "To investigate:"
  echo "- Check which line indices were used (line1_idx, line2_idx)"
  echo "- Check if order check would have filtered it: line2_idx <= line1_idx?"
  echo "- Check if seenPairs was already set in serial but not in parallel"
  echo "- Check spatial query results for both line1 and line2"
elif [ $SERIAL_FOUND -gt 0 ] && [ $PARALLEL_FOUND -eq 0 ]; then
  echo "🔍 MISSING PAIR: Found in serial but NOT in parallel"
  echo ""
  echo "This is a correctness bug! Why did parallel miss it?"
elif [ $SERIAL_FOUND -gt 0 ] && [ $PARALLEL_FOUND -gt 0 ]; then
  echo "✅ Pair found in both (correct behavior)"
else
  echo "❌ Pair not found in either (might be invalid pair)"
fi

