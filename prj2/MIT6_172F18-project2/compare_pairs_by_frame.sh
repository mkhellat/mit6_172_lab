#!/bin/bash
# Script to compare candidate pairs frame by frame between serial and parallel execution
# Finds the first frame where pairs differ and identifies responsible worker

set -e

INPUT_FILE="${1:-input/smalllines.in}"
NUM_FRAMES="${2:-10}"
NUM_WORKERS="${3:-4}"

echo "=== Comparing Candidate Pairs Frame by Frame ==="
echo "Input file: $INPUT_FILE"
echo "Number of frames: $NUM_FRAMES"
echo "Number of workers: $NUM_WORKERS"
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
echo "=== Extracting pairs per frame ==="

# Extract pairs for each frame
for frame in $(seq 1 $NUM_FRAMES); do
  echo "Processing frame $frame..."
  
  # Extract pairs for this frame from serial
  # Format: convert to "id1,id2" with id1 < id2, then sort numerically
  grep "DEBUG_PAIR_ADDED.*FRAME=$frame" "$SERIAL_OUT" | \
    sed 's/.*PAIR=\([0-9]*\),\([0-9]*\).*/\1 \2/' | \
    awk '{if ($1 < $2) print $1","$2; else print $2","$1}' | \
    sort -t, -k1,1n -k2,2n -u > "$TMPDIR/serial_frame_${frame}.txt"
  
  # Extract pairs for this frame from parallel
  grep "DEBUG_PAIR_ADDED.*FRAME=$frame" "$PARALLEL_OUT" | \
    sed 's/.*PAIR=\([0-9]*\),\([0-9]*\).*/\1 \2/' | \
    awk '{if ($1 < $2) print $1","$2; else print $2","$1}' | \
    sort -t, -k1,1n -k2,2n -u > "$TMPDIR/parallel_frame_${frame}.txt"
  
  # Count pairs
  serial_count=$(wc -l < "$TMPDIR/serial_frame_${frame}.txt" | tr -d ' ')
  parallel_count=$(wc -l < "$TMPDIR/parallel_frame_${frame}.txt" | tr -d ' ')
  
  echo "  Frame $frame: Serial=$serial_count pairs, Parallel=$parallel_count pairs"
  
  # Check if they differ
  if ! cmp -s "$TMPDIR/serial_frame_${frame}.txt" "$TMPDIR/parallel_frame_${frame}.txt"; then
    echo ""
    echo "*** MISMATCH FOUND IN FRAME $frame ***"
    echo ""
    echo "Pairs only in serial:"
    comm -23 "$TMPDIR/serial_frame_${frame}.txt" "$TMPDIR/parallel_frame_${frame}.txt" | head -20
    if [ $(comm -23 "$TMPDIR/serial_frame_${frame}.txt" "$TMPDIR/parallel_frame_${frame}.txt" | wc -l) -gt 20 ]; then
      echo "... (more pairs)"
    fi
    
    echo ""
    echo "Pairs only in parallel:"
    comm -13 "$TMPDIR/serial_frame_${frame}.txt" "$TMPDIR/parallel_frame_${frame}.txt" | head -20
    if [ $(comm -13 "$TMPDIR/serial_frame_${frame}.txt" "$TMPDIR/parallel_frame_${frame}.txt" | wc -l) -gt 20 ]; then
      echo "... (more pairs)"
    fi
    
    echo ""
    echo "=== Detailed Analysis for Frame $frame ==="
    echo "Serial pairs file: $TMPDIR/serial_frame_${frame}.txt"
    echo "Parallel pairs file: $TMPDIR/parallel_frame_${frame}.txt"
    
    # Find which worker added the extra pairs
    echo ""
    echo "Finding which worker added pairs only in parallel..."
    grep "DEBUG_PAIR_ADDED.*FRAME=$frame" "$PARALLEL_OUT" | \
      sed 's/.*PAIR=\([0-9]*\),\([0-9]*\).*/\1,\2/' | \
      while read pair; do
        if ! grep -q "^$pair$" "$TMPDIR/serial_frame_${frame}.txt"; then
          # This pair is only in parallel - find the full log line
          grep "DEBUG_PAIR_ADDED.*FRAME=$frame.*PAIR=$pair" "$PARALLEL_OUT" | head -1
        fi
      done | head -10
    
    echo ""
    echo "Finding which pairs from serial are missing in parallel..."
    comm -23 "$TMPDIR/serial_frame_${frame}.txt" "$TMPDIR/parallel_frame_${frame}.txt" | \
      head -10 | while read pair; do
        echo "Missing pair: $pair"
        # Try to find if it was attempted but skipped
        id1=$(echo $pair | cut -d, -f1)
        id2=$(echo $pair | cut -d, -f2)
        grep "FRAME=$frame" "$PARALLEL_OUT" | grep -E "(WARNING.*$id1.*$id2|WARNING.*$id2.*$id1|skipped.*$id1|skipped.*$id2)" | head -3
      done
    
    echo ""
    echo "=== First mismatch found in frame $frame - stopping analysis ==="
    exit 0
  fi
done

echo ""
echo "=== All frames match! ==="

